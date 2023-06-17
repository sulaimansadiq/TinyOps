/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/micro/kernels/depthwise_conv.h"

#include "CMSIS/NN/Include/arm_nnfunctions.h"
#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/common.h"
#include "tensorflow/lite/kernels/internal/quantization_util.h"
#include "tensorflow/lite/kernels/internal/reference/depthwiseconv_float.h"
#include "tensorflow/lite/kernels/internal/reference/depthwiseconv_uint8.h"
#include "tensorflow/lite/kernels/internal/reference/integer_ops/depthwise_conv.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/kernels/padding.h"
#include "tensorflow/lite/micro/kernels/conv.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"

#include "overlay_fw.h"

#ifdef OVERLAY_FW
extern "C" {
#include "dmacopying.h"
#include "main.h"
}

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef  htim10;

extern volatile DMACopyQue DMAQue[NUM_DMA_STREAMS_USED];

extern volatile PartitionedExecutionState partitioned_exec_state;

extern volatile TfLiteEvalTensor oc_input_tensors[OP_MAX_INPUT_SIZE];
//extern volatile uint8_t oc_input_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
//extern volatile int oc_input_tensors_dims_data[OP_MAX_INPUT_SIZE][4];
//extern volatile TfLiteIntArray oc_input_tensors_dims_array_0;
//extern volatile TfLiteIntArray oc_input_tensors_dims_array_1;

extern volatile TfLiteEvalTensor oc_input2_tensors[OP_MAX_INPUT_SIZE];
//extern volatile uint8_t oc_input2_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
//extern volatile int oc_input2_tensors_dims_data[OP_MAX_INPUT_SIZE][4];
//extern volatile TfLiteIntArray oc_input2_tensors_dims_array_0;
//extern volatile TfLiteIntArray oc_input2_tensors_dims_array_1;

#ifdef OVERLAY_BIASES
extern volatile TfLiteEvalTensor oc_input3_tensor;
//extern volatile uint8_t oc_input3_tensor_buffer[BIAS_TENSOR_BUFFER_SIZE];
//extern volatile int oc_input3_tensor_dims_data[MAX_NUM_DIMS];
//extern volatile TfLiteIntArray oc_input3_tensor_dims_array;
#endif

extern volatile TfLiteEvalTensor oc_output_tensors;
//extern uint8_t oc_output_tensors_buffers[TENSOR_BUFFER_SIZE];
//extern volatile int oc_output_tensors_dims_data[4];
//extern volatile TfLiteIntArray oc_output_tensors_dims_array;

#ifdef OVERLAY_QUANT_PARAMS
extern volatile uint8_t * quant_out_multiplier;
extern volatile uint8_t * quant_out_shift;
//extern volatile uint8_t quant_out_multiplier[QUANT_PARAMS_BUFFER_SIZE+128];
//extern volatile uint8_t quant_out_shift[QUANT_PARAMS_BUFFER_SIZE+128];
#endif

extern uint32_t dma_waiting;
uint32_t dma_waiting_depconv = 0;
uint32_t dma_waiting_depconv_p0 = 0;

#ifdef DEBUG_DUMP_DMA_WAIT_DEPCONV
uint32_t debug_dump_dma_waiting[800];
uint32_t debug_dump_dma_waiting_depconv[800];
uint32_t debug_dump_dma_waiting_depconv_idx = 0;
#endif

extern	TinyOpsBuffSizes	tinyOpsBuffSizes;
#endif

namespace tflite {
namespace {

struct OpData {
  OpDataConv reference_op_data;

  // Index to buffer for optimizations if applicable.
  int buffer_idx;
};

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  OpData* data = static_cast<OpData*>(node->user_data);
  const auto& params =
      *(reinterpret_cast<TfLiteDepthwiseConvParams*>(node->builtin_data));

  const TfLiteTensor* input =
      GetInput(context, node, kDepthwiseConvInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  const TfLiteTensor* filter =
      GetInput(context, node, kDepthwiseConvWeightsTensor);
  TF_LITE_ENSURE(context, filter != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kDepthwiseConvOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  const TfLiteType data_type = input->type;
  int input_width = SizeOfDimension(input, 2);
  int input_height = SizeOfDimension(input, 1);
  int filter_width = SizeOfDimension(filter, 2);
  int filter_height = SizeOfDimension(filter, 1);
  int output_width = SizeOfDimension(output, 2);
  int output_height = SizeOfDimension(output, 1);

  if (input->type == kTfLiteInt8) {
    TF_LITE_ENSURE_EQ(context, filter->quantization.type,
                      kTfLiteAffineQuantization);

    // All per-channel quantized tensors need valid zero point and scale arrays.
    const auto* affine_quantization =
        reinterpret_cast<TfLiteAffineQuantization*>(
            filter->quantization.params);
    TF_LITE_ENSURE(context, affine_quantization);
    TF_LITE_ENSURE(context, affine_quantization->scale);
    TF_LITE_ENSURE(context, affine_quantization->zero_point);
    TF_LITE_ENSURE(
        context, affine_quantization->scale->size == 1 ||
                     affine_quantization->scale->size ==
                         filter->dims->data[kDepthwiseConvQuantizedDimension]);
    TF_LITE_ENSURE_EQ(context, affine_quantization->scale->size,
                      affine_quantization->zero_point->size);
  }

  // Allocate memory for per-channel quantization parameters
  const int num_channels = filter->dims->data[kDepthwiseConvQuantizedDimension];

  data->reference_op_data.per_channel_output_multiplier =
      reinterpret_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));
  data->reference_op_data.per_channel_output_shift =
      reinterpret_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));

  TF_LITE_ENSURE_STATUS(CalculateOpDataDepthwiseConv(
      context, node, params, input_width, input_height, filter_width,
      filter_height, output_width, output_height, data_type,
      &data->reference_op_data));

  if (input->type == kTfLiteInt8) {
    RuntimeShape input_shape = GetTensorShape(input);
    RuntimeShape output_shape = GetTensorShape(output);
    RuntimeShape filter_shape = GetTensorShape(filter);
    TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
    TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
    TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);

    const int batch_size = MatchingDim(input_shape, 0, output_shape, 0);
    const int output_depth = MatchingDim(output_shape, 3, filter_shape, 3);
    TFLITE_DCHECK_EQ(batch_size, 1); /* Only batch = 1 is supported */

    cmsis_nn_dims input_dims;
    input_dims.n = batch_size;
    input_dims.h = input_height;
    input_dims.w = input_width;
    input_dims.c = input_shape.Dims(3);

    cmsis_nn_dims filter_dims;
    filter_dims.n = 1;
    filter_dims.h = filter_height;
    filter_dims.w = filter_width;
    filter_dims.c = output_depth;

    cmsis_nn_dims output_dims;
    output_dims.n = batch_size;
    output_dims.h = output_height;
    output_dims.w = output_width;
    output_dims.c = output_depth;

    cmsis_nn_dw_conv_params dw_conv_params;
    dw_conv_params.padding.h = data->reference_op_data.padding.height;
    dw_conv_params.padding.w = data->reference_op_data.padding.width;

    const int32_t buf_size = arm_depthwise_conv_wrapper_s8_get_buffer_size(
        &dw_conv_params, &input_dims, &filter_dims, &output_dims);

	#ifdef OVERLAY_FW
	char uart_buf[100];
	int uart_buf_len;

//	uart_buf_len = sprintf(uart_buf, "DepConv Scratch Buffer size %i\n", buf_size);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	if (buf_size > tinyOpsBuffSizes.partitionIm2colSize) {

//		  uart_buf_len = sprintf(uart_buf, "DepConv Scratch Buffer too small for buffer size %u\n", buf_size);
//		  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

//		  return kTfLiteError;
	}
	#endif

    if (buf_size > 0) {
      TF_LITE_ENSURE_STATUS(context->RequestScratchBufferInArena(
          context, buf_size, &data->buffer_idx));
    } else {
      data->buffer_idx = -1;
    }
  }
  return kTfLiteOk;
}

void EvalQuantizedPerChannel(TfLiteContext* context, TfLiteNode* node,
                             const TfLiteDepthwiseConvParams& params,
                             const OpData& data, const TfLiteEvalTensor* input,
                             const TfLiteEvalTensor* filter,
                             const TfLiteEvalTensor* bias,
                             TfLiteEvalTensor* output) {
  cmsis_nn_dw_conv_params dw_conv_params;
  dw_conv_params.dilation.h = params.dilation_height_factor;
  dw_conv_params.dilation.w = params.dilation_width_factor;
  // Call to reference implementation can be removed when dilation is supported
  // in the optimized implementations.
  if (1 == dw_conv_params.dilation.h && 1 == dw_conv_params.dilation.w) {
    dw_conv_params.input_offset = -data.reference_op_data.input_zero_point;
    dw_conv_params.output_offset = data.reference_op_data.output_zero_point;
    dw_conv_params.stride.h = params.stride_height;
    dw_conv_params.stride.w = params.stride_width;
    dw_conv_params.padding.h = data.reference_op_data.padding.height;
    dw_conv_params.padding.w = data.reference_op_data.padding.width;
    // TODO(b/130439627): Use calculated value for clamping.
    dw_conv_params.activation.min = std::numeric_limits<int8_t>::min();
    dw_conv_params.activation.max = std::numeric_limits<int8_t>::max();
    dw_conv_params.ch_mult = params.depth_multiplier;

    cmsis_nn_per_channel_quant_params quant_params;
    quant_params.multiplier =
        data.reference_op_data.per_channel_output_multiplier;
    quant_params.shift = data.reference_op_data.per_channel_output_shift;

    RuntimeShape filter_shape = tflite::micro::GetTensorShape(filter);
    RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
    RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
    RuntimeShape bias_shape = tflite::micro::GetTensorShape(bias);

    TFLITE_DCHECK_LE(dw_conv_params.activation.min,
                     dw_conv_params.activation.max);

    const int batch_size = MatchingDim(input_shape, 0, output_shape, 0);
    const int output_depth = MatchingDim(filter_shape, 3, output_shape, 3);

    if (tflite::micro::GetTensorData<int8_t>(bias)) {
      TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);
    }

    cmsis_nn_dims input_dims;
    input_dims.n = batch_size;
    input_dims.h = input_shape.Dims(1);
    input_dims.w = input_shape.Dims(2);
    input_dims.c = input_shape.Dims(3);

    cmsis_nn_dims filter_dims;
    filter_dims.n = filter_shape.Dims(0);
    filter_dims.h = filter_shape.Dims(1);
    filter_dims.w = filter_shape.Dims(2);
    filter_dims.c = output_depth;

    cmsis_nn_dims bias_dims;
    bias_dims.n = 1;
    bias_dims.h = 1;
    bias_dims.w = 1;
    bias_dims.c = output_depth;

    cmsis_nn_dims output_dims;
    output_dims.n = batch_size;
    output_dims.h = output_shape.Dims(1);
    output_dims.w = output_shape.Dims(2);
    output_dims.c = output_depth;

    cmsis_nn_context ctx;
    ctx.buf = nullptr;
    /* 'size' is unused */
    ctx.size = 0;

    if (data.buffer_idx > -1) {
      ctx.buf = context->GetScratchBuffer(context, data.buffer_idx);
    }

    TFLITE_DCHECK_EQ(
        arm_depthwise_conv_wrapper_s8(
            &ctx, &dw_conv_params, &quant_params, &input_dims,
            tflite::micro::GetTensorData<int8_t>(input), &filter_dims,
            tflite::micro::GetTensorData<int8_t>(filter), &bias_dims,
            tflite::micro::GetTensorData<int32_t>(bias), &output_dims,
            tflite::micro::GetTensorData<int8_t>(output)),
        ARM_MATH_SUCCESS);
  } else {
    reference_integer_ops::DepthwiseConvPerChannel(
        DepthwiseConvParamsQuantized(params, data.reference_op_data),
        data.reference_op_data.per_channel_output_multiplier,
        data.reference_op_data.per_channel_output_shift,
        tflite::micro::GetTensorShape(input),
        tflite::micro::GetTensorData<int8_t>(input),
        tflite::micro::GetTensorShape(filter),
        tflite::micro::GetTensorData<int8_t>(filter),
        tflite::micro::GetTensorShape(bias),
        tflite::micro::GetTensorData<int32_t>(bias),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<int8_t>(output));
  }
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
	TFLITE_DCHECK(node->user_data != nullptr);
	TFLITE_DCHECK(node->builtin_data != nullptr);

	const auto& params =
	  *(reinterpret_cast<TfLiteDepthwiseConvParams*>(node->builtin_data));
	#ifdef OVERLAY_FW
	OpData& data = *(static_cast<OpData*>(node->user_data));
	#else
	const OpData& data = *(static_cast<OpData*>(node->user_data));
	#endif

	TfLiteEvalTensor* output =
	  tflite::micro::GetEvalOutput(context, node, kDepthwiseConvOutputTensor);
	const TfLiteEvalTensor* input =
	  tflite::micro::GetEvalInput(context, node, kDepthwiseConvInputTensor);
	const TfLiteEvalTensor* filter =
	  tflite::micro::GetEvalInput(context, node, kDepthwiseConvWeightsTensor);
	const TfLiteEvalTensor* bias =
	  (NumInputs(node) == 3)
		  ? tflite::micro::GetEvalInput(context, node, kDepthwiseConvBiasTensor)
		  : nullptr;

	#ifdef OVERLAY_FW

	uint16_t timer_val;
	int uart_buf_len;
	char uart_buf[50];

	cmsis_nn_dw_conv_params conv_params;
    conv_params.padding.h = data.reference_op_data.padding.height;

    volatile PartitionInfo * op_part_info		= &partitioned_exec_state.ops_parts_info[partitioned_exec_state.currentOp];
    volatile PartitionInfo * next_op_part_info	= &partitioned_exec_state.ops_parts_info[partitioned_exec_state.currentOp+1];

//	OVERLAY_CopyInData(&DMAQue, op_part_info, 0, input, &oc_input_tensors[partitioned_exec_state.pingPong]);

	uint32_t nextOpFirstPartCopied			= 0;
	uint32_t output_tensor_produced_bytes	= 0;

//	StartTransfer(&DmaHandle);

	#ifdef OVERLAY_FILTERS
	uint32_t filterTensorSize	= filter->dims->data[0] * filter->dims->data[1] * filter->dims->data[2] * filter->dims->data[3];

	if ( filterTensorSize < tinyOpsBuffSizes.partitionFilterSize ) {
		filter = (TfLiteEvalTensor *)&oc_input2_tensors[0];
//		DMA_CopyTensor(&DMAQue[1], op_part_info->filter, &oc_input2_tensors[0]);
	}
	else {
//		uart_buf_len = sprintf(uart_buf, "Bigger Filter\n");
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
	}
	#endif

	#ifdef OVERLAY_BIASES
	if ( bias ) {
		uint32_t biasTensorSize	= bias->dims->data[0] * 4;
//		uint32_t biasTensorSize	= bias->dims->data[0] * 8;

		if ( biasTensorSize < tinyOpsBuffSizes.biasSize ) {
			bias = (TfLiteEvalTensor *)&oc_input3_tensor;
//			DMA_CopyTensor(&DMAQue[0], op_part_info->bias, &oc_input3_tensor);
//			bias = &oc_input2_tensors[1];
//			DMA_CopyTensor(&DMAQue, op_part_info->bias, &oc_input2_tensors[1]);
		}
		else {
//			uart_buf_len = sprintf(uart_buf, "Bigger Filter\n");
//			HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
		}
	}
	#endif

	#ifdef OVERLAY_QUANT_PARAMS
	const int num_channels = filter->dims->data[kDepthwiseConvQuantizedDimension];

	int32_t * local_per_channel_output_multiplier;
	int32_t * local_per_channel_output_shift;

	if ( (num_channels * sizeof(int32_t)) < tinyOpsBuffSizes.quantParamsOutMultiplierSize ) {
//	if ( (num_channels * sizeof(int32_t)) < (QUANT_PARAMS_BUFFER_SIZE-100*4) ) {
		local_per_channel_output_multiplier	= data.reference_op_data.per_channel_output_multiplier;
		local_per_channel_output_shift		= data.reference_op_data.per_channel_output_shift;

//		DMA_Copy_Que_Req_Put( &DMAQue[0], (uint8_t *)data.reference_op_data.per_channel_output_multiplier, (uint8_t *) quant_out_multiplier, num_channels * sizeof(int32_t) );
//		data.reference_op_data.per_channel_output_multiplier = (int32_t *)quant_out_multiplier;
//
//		DMA_Copy_Que_Req_Put( &DMAQue[0], (uint8_t *)data.reference_op_data.per_channel_output_shift, (uint8_t *) quant_out_shift, num_channels * sizeof(int32_t) );
//		data.reference_op_data.per_channel_output_shift	= (int32_t *)quant_out_shift;

		DMA_Copy_Que_Req_Put( &DMAQue[0], (uint8_t *)data.reference_op_data.per_channel_output_multiplier, (uint8_t *) &quant_out_multiplier[64], num_channels * sizeof(int32_t) );
		data.reference_op_data.per_channel_output_multiplier = (int32_t *)&quant_out_multiplier[64];

		DMA_Copy_Que_Req_Put( &DMAQue[0], (uint8_t *)data.reference_op_data.per_channel_output_shift, (uint8_t *) &quant_out_shift[64], num_channels * sizeof(int32_t) );
		data.reference_op_data.per_channel_output_shift	= (int32_t *)&quant_out_shift[64];
	}
	#endif

	while ( (DMAQue[1].readIdx != DMAQue[1].writeIdx) );

	// start pipeline
	for ( int32_t p = 0; p < op_part_info->numParts; p++ ) {

	  partitioned_exec_state.currentPart = p;

	  uint16_t timer_val;
	  int uart_buf_len;
	  char uart_buf[50];

//	  timer_val = __HAL_TIM_GET_COUNTER(&htim10);

	  // wait till input data copied
	  while ( (DMAQue[0].readIdx != DMAQue[0].writeIdx) );

//	  while ( (DMAQue[0].readIdx != DMAQue[0].writeIdx) && (DMAQue[1].readIdx != DMAQue[1].writeIdx) ); //{
//	  while ( DMAQue[0].readIdx != DMAQue[0].writeIdx ) {
//		  if (p != 0)
//			  dma_waiting_depconv++;
//		  else
//			  dma_waiting_depconv_p0++;
//	  }

	  // get next input and setup dma before processing copied in input
	  if ( p < (op_part_info->numParts-1) )
		  OVERLAY_CopyInData(&DMAQue[0], op_part_info, p+1, &oc_input_tensors[!partitioned_exec_state.pingPong], &oc_input2_tensors[!partitioned_exec_state.pingPong]);
	  else { // get first part of next operation
		  if (next_op_part_info->input1) {
			  uint32_t startingRow			= next_op_part_info->partitionStart[0];
			  uint32_t endingRow			= next_op_part_info->partitionEnd[0];
			  uint32_t bytes_in_partition	= (endingRow-startingRow)* next_op_part_info->input1->dims->data[0] * next_op_part_info->input1->dims->data[2] * next_op_part_info->input1->dims->data[3];

			  if ( output_tensor_produced_bytes >= bytes_in_partition && !next_op_part_info->input2 ) {
				  OVERLAY_CopyInData(&DMAQue[0], next_op_part_info, 0, &oc_input_tensors[!partitioned_exec_state.pingPong], &oc_input2_tensors[!partitioned_exec_state.pingPong]);
				  nextOpFirstPartCopied = 1;
			  }
		  }
	  }

//	  if ( p < (op_part_info->numParts-1) ) {
//		  OVERLAY_CopyInData(&DMAQue, op_part_info, p+1, input, &oc_input_tensors[!partitioned_exec_state.pingPong]);
//	  }

	  // setup ouput tensor and params for operation
	  data.reference_op_data.padding.height = op_part_info->op_part_data[p][OP_DATA_CONV_PAD_IDX];

	  oc_output_tensors.dims->data[1]	= op_part_info->out_partitioned_dim[p];

	  oc_output_tensors.type			= output->type;
	  oc_output_tensors.data.int8		= &output->data.int8[output_tensor_produced_bytes];
	  oc_output_tensors.dims->size		= output->dims->size;
	  oc_output_tensors.dims->data[0]	= output->dims->data[0];
	  oc_output_tensors.dims->data[2]	= output->dims->data[2];
	  oc_output_tensors.dims->data[3]	= output->dims->data[3];

	  output_tensor_produced_bytes += oc_output_tensors.dims->data[0] * oc_output_tensors.dims->data[1] * oc_output_tensors.dims->data[2] * oc_output_tensors.dims->data[3];

	  TfLiteStatus ret_status;

	  // execute operation on partition
		switch (oc_input_tensors[partitioned_exec_state.pingPong].type) {  // Already know in/out types are same.
		  case kTfLiteFloat32: {
			tflite::reference_ops::DepthwiseConv(
			  DepthwiseConvParamsFloat(params, data.reference_op_data),
			  tflite::micro::GetTensorShape((TfLiteEvalTensor *)&oc_input_tensors[partitioned_exec_state.pingPong]),
			  tflite::micro::GetTensorData<float>((TfLiteEvalTensor *)&oc_input_tensors[partitioned_exec_state.pingPong]),
			  tflite::micro::GetTensorShape(filter),
			  tflite::micro::GetTensorData<float>(filter),
			  tflite::micro::GetTensorShape(bias),
			  tflite::micro::GetTensorData<float>(bias),
			  tflite::micro::GetTensorShape(output),
			  tflite::micro::GetTensorData<float>(output));
			break;
		  }
		  case kTfLiteInt8:
//			EvalQuantizedPerChannel(context, node, params, data, &oc_input_tensors[partitioned_exec_state.pingPong], filter, bias,
//									&oc_output_tensors);
			EvalQuantizedPerChannel(context, node, params, data,
									(TfLiteEvalTensor *)&oc_input_tensors[partitioned_exec_state.pingPong],
									filter,
									bias,
									(TfLiteEvalTensor *)&oc_output_tensors);
			break;
		  default:
			TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
								 TfLiteTypeGetName(input->type), input->type);
			return kTfLiteError;
		}

		  if ( !( p < (op_part_info->numParts-1) ) && !nextOpFirstPartCopied && next_op_part_info->input1 )
			  OVERLAY_CopyInData(&DMAQue[0], next_op_part_info, 0, &oc_input_tensors[!partitioned_exec_state.pingPong], &oc_input2_tensors[!partitioned_exec_state.pingPong]);

	  // switch buffer that was being copied in in background
	  partitioned_exec_state.pingPong = !partitioned_exec_state.pingPong;
	}

	// restore padding params that we messed around with
	data.reference_op_data.padding.height = conv_params.padding.h;
	#ifdef OVERLAY_QUANT_PARAMS
	data.reference_op_data.per_channel_output_multiplier	= local_per_channel_output_multiplier;
	data.reference_op_data.per_channel_output_shift			= local_per_channel_output_shift;
	#endif

	#ifdef OVERLAY_FILTERS
	const TfLiteEvalTensor * next_op_filter = next_op_part_info->filter;

	if ( next_op_filter ) {
		uint32_t next_op_filterTensorSize	=	next_op_filter->dims->data[0] *
												next_op_filter->dims->data[1] *
												next_op_filter->dims->data[2] *
												next_op_filter->dims->data[3];

		if ( next_op_filterTensorSize < tinyOpsBuffSizes.partitionFilterSize )
			DMA_CopyTensor(&DMAQue[1], next_op_filter, &oc_input2_tensors[0]);
	}
	#endif

	#ifdef OVERLAY_BIASES
	const TfLiteEvalTensor * next_op_bias = next_op_part_info->bias;

	if ( next_op_bias ) {
		uint32_t next_op_biasTensorSize		=	next_op_bias->dims->data[0] * 4;
		if ( next_op_biasTensorSize < tinyOpsBuffSizes.biasSize )
			DMA_CopyTensor(&DMAQue[1], next_op_bias, &oc_input3_tensor);
	}
	#endif

	#else
	switch (input->type) {  // Already know in/out types are same.
	  case kTfLiteFloat32: {
		tflite::reference_ops::DepthwiseConv(
		  DepthwiseConvParamsFloat(params, data.reference_op_data),
		  tflite::micro::GetTensorShape(input),
		  tflite::micro::GetTensorData<float>(input),
		  tflite::micro::GetTensorShape(filter),
		  tflite::micro::GetTensorData<float>(filter),
		  tflite::micro::GetTensorShape(bias),
		  tflite::micro::GetTensorData<float>(bias),
		  tflite::micro::GetTensorShape(output),
		  tflite::micro::GetTensorData<float>(output));
		break;
	  }
	  case kTfLiteInt8:
		EvalQuantizedPerChannel(context, node, params, data, input, filter, bias,
		      				  output);
		break;
	  default:
		TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
							 TfLiteTypeGetName(input->type), input->type);
	    return kTfLiteError;
    }
	#endif

  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration Register_DEPTHWISE_CONV_2D() {
  return {/*init=*/Init,
          /*free=*/nullptr,
          /*prepare=*/Prepare,
          /*invoke=*/Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite
