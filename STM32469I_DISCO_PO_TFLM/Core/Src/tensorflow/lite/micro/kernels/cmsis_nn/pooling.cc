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
#include "tensorflow/lite/kernels/internal/reference/pooling.h"

#include "CMSIS/NN/Include/arm_nnfunctions.h"
#include "flatbuffers/base.h"  // from @flatbuffers
#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/kernels/internal/reference/integer_ops/pooling.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/kernels/padding.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"

#include "overlay_fw.h"

#ifdef OVERLAY_FW
extern "C" {
#include "dmacopying.h"
#include "main.h"
}

extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef  htim10;
extern volatile uint8_t * debugPrintPtr;

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

extern uint32_t dma_waiting;
uint32_t dma_waiting_pool = 0;
uint32_t dma_waiting_pool_p0 = 0;
#endif

namespace tflite {
namespace ops {
namespace micro {
namespace pooling {

namespace {

constexpr int kInputTensor = 0;
constexpr int kOutputTensor = 0;

struct OpData {
  TfLitePaddingValues padding;
  // Index to buffer for optimizations if applicable.
  int buffer_idx;

  int32_t activation_min;
  int32_t activation_max;
  float activation_min_f32;
  float activation_max_f32;
};

TfLiteStatus CalculateOpData(TfLiteContext* context,
                             const TfLitePoolParams* params,
                             const TfLiteTensor* input, TfLiteTensor* output,
                             OpData* data) {
  // input: batch, height, width, channel
  int height = SizeOfDimension(input, 1);
  int width = SizeOfDimension(input, 2);

  int out_height, out_width;

  data->padding = ComputePaddingHeightWidth(
      params->stride_height, params->stride_width,
      /*dilation_rate_height=*/1,
      /*dilation_rate_width=*/1, height, width, params->filter_height,
      params->filter_width, params->padding, &out_height, &out_width);

  if (input->type == kTfLiteFloat32) {
    CalculateActivationRange(params->activation, &data->activation_min_f32,
                             &data->activation_max_f32);
  } else {
    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->activation_min,
        &data->activation_max));
    TFLITE_DCHECK_LE(data->activation_min, data->activation_max);
  }

  // Set buffer index to a reset value
  data->buffer_idx = -1;

  return kTfLiteOk;
}

void AverageEvalFloat(const TfLiteContext* context, const TfLiteNode* node,
                      const TfLitePoolParams* params, const OpData& data,
                      const TfLiteEvalTensor* input, TfLiteEvalTensor* output) {
  float activation_min, activation_max;
  CalculateActivationRange(params->activation, &activation_min,
                           &activation_max);

  PoolParams op_params;
  op_params.stride_height = params->stride_height;
  op_params.stride_width = params->stride_width;
  op_params.filter_height = params->filter_height;
  op_params.filter_width = params->filter_width;
  op_params.padding_values.height = data.padding.height;
  op_params.padding_values.width = data.padding.width;
  op_params.float_activation_min = activation_min;
  op_params.float_activation_max = activation_max;
  reference_ops::AveragePool(op_params, tflite::micro::GetTensorShape(input),
                             tflite::micro::GetTensorData<float>(input),
                             tflite::micro::GetTensorShape(output),
                             tflite::micro::GetTensorData<float>(output));
}

void AverageEvalQuantized(TfLiteContext* context, const TfLiteNode* node,
                          const TfLitePoolParams* params, const OpData& data,
                          const TfLiteEvalTensor* input,
                          TfLiteEvalTensor* output) {
  TFLITE_DCHECK(input->type == kTfLiteUInt8 || input->type == kTfLiteInt8);

  if (input->type == kTfLiteUInt8) {
    PoolParams op_params;
    op_params.stride_height = params->stride_height;
    op_params.stride_width = params->stride_width;
    op_params.filter_height = params->filter_height;
    op_params.filter_width = params->filter_width;
    op_params.padding_values.height = data.padding.height;
    op_params.padding_values.width = data.padding.width;
    op_params.quantized_activation_min = data.activation_min;
    op_params.quantized_activation_max = data.activation_max;

    reference_ops::AveragePool(op_params, tflite::micro::GetTensorShape(input),
                               tflite::micro::GetTensorData<uint8_t>(input),
                               tflite::micro::GetTensorShape(output),
                               tflite::micro::GetTensorData<uint8_t>(output));
  } else {
    RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
    TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);

    RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
    TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);

    const int depth = MatchingDim(input_shape, 3, output_shape, 3);

    cmsis_nn_dims input_dims;
    input_dims.n = 1;
    input_dims.h = input_shape.Dims(1);
    input_dims.w = input_shape.Dims(2);
    input_dims.c = depth;

    cmsis_nn_dims output_dims;
    output_dims.n = 1;
    output_dims.h = output_shape.Dims(1);
    output_dims.w = output_shape.Dims(2);
    output_dims.c = depth;

    cmsis_nn_pool_params pool_params;
    pool_params.stride.h = params->stride_height;
    pool_params.stride.w = params->stride_width;
    pool_params.padding.h = data.padding.height;
    pool_params.padding.w = data.padding.width;
    pool_params.activation.min = data.activation_min;
    pool_params.activation.max = data.activation_max;

    cmsis_nn_dims filter_dims;
    filter_dims.n = 1;
    filter_dims.h = params->filter_height;
    filter_dims.w = params->filter_width;
    filter_dims.c = 1;

    cmsis_nn_context ctx;
    ctx.buf = nullptr;
    ctx.size = 0;
    if (data.buffer_idx > -1) {
      ctx.buf = context->GetScratchBuffer(context, data.buffer_idx);
    }

    TFLITE_DCHECK_EQ(
        arm_avgpool_s8(&ctx, &pool_params, &input_dims,
                       tflite::micro::GetTensorData<int8_t>(input),
                       &filter_dims, &output_dims,
                       tflite::micro::GetTensorData<int8_t>(output)),
        ARM_MATH_SUCCESS);
  }
}

void MaxEvalFloat(TfLiteContext* context, TfLiteNode* node,
                  TfLitePoolParams* params, const OpData& data,
                  const TfLiteEvalTensor* input, TfLiteEvalTensor* output) {
  float activation_min, activation_max;
  CalculateActivationRange(params->activation, &activation_min,
                           &activation_max);
  tflite::PoolParams op_params;
  op_params.stride_height = params->stride_height;
  op_params.stride_width = params->stride_width;
  op_params.filter_height = params->filter_height;
  op_params.filter_width = params->filter_width;
  op_params.padding_values.height = data.padding.height;
  op_params.padding_values.width = data.padding.width;
  op_params.float_activation_min = data.activation_min_f32;
  op_params.float_activation_max = data.activation_max_f32;
  reference_ops::MaxPool(op_params, tflite::micro::GetTensorShape(input),
                         tflite::micro::GetTensorData<float>(input),
                         tflite::micro::GetTensorShape(output),
                         tflite::micro::GetTensorData<float>(output));
}

void MaxEvalQuantizedUInt8(TfLiteContext* context, TfLiteNode* node,
                           TfLitePoolParams* params, const OpData& data,
                           const TfLiteEvalTensor* input,
                           TfLiteEvalTensor* output) {
  tflite::PoolParams op_params;
  op_params.stride_height = params->stride_height;
  op_params.stride_width = params->stride_width;
  op_params.filter_height = params->filter_height;
  op_params.filter_width = params->filter_width;
  op_params.padding_values.height = data.padding.height;
  op_params.padding_values.width = data.padding.width;
  op_params.quantized_activation_min = data.activation_min;
  op_params.quantized_activation_max = data.activation_max;
  reference_ops::MaxPool(op_params, tflite::micro::GetTensorShape(input),
                         tflite::micro::GetTensorData<uint8_t>(input),
                         tflite::micro::GetTensorShape(output),
                         tflite::micro::GetTensorData<uint8_t>(output));
}

TfLiteStatus MaxEvalInt8(TfLiteContext* context, const TfLiteNode* node,
                         const TfLitePoolParams* params, const OpData& data,
                         const TfLiteEvalTensor* input,
                         TfLiteEvalTensor* output) {
  RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
  RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
  const int depth = MatchingDim(input_shape, 3, output_shape, 3);

  cmsis_nn_dims input_dims;
  input_dims.n = 1;
  input_dims.h = input_shape.Dims(1);
  input_dims.w = input_shape.Dims(2);
  input_dims.c = depth;

  cmsis_nn_dims output_dims;
  output_dims.n = 1;
  output_dims.h = output_shape.Dims(1);
  output_dims.w = output_shape.Dims(2);
  output_dims.c = depth;

  cmsis_nn_pool_params pool_params;
  pool_params.stride.h = params->stride_height;
  pool_params.stride.w = params->stride_width;
  pool_params.padding.h = data.padding.height;
  pool_params.padding.w = data.padding.width;
  pool_params.activation.min = data.activation_min;
  pool_params.activation.max = data.activation_max;

  cmsis_nn_dims filter_dims;
  filter_dims.n = 1;
  filter_dims.h = params->filter_height;
  filter_dims.w = params->filter_width;
  filter_dims.c = 1;

  cmsis_nn_context ctx;
  ctx.buf = nullptr;
  ctx.size = 0;
  if (data.buffer_idx > -1) {
    ctx.buf = context->GetScratchBuffer(context, data.buffer_idx);
  }

  TFLITE_DCHECK_EQ(
      arm_max_pool_s8(&ctx, &pool_params, &input_dims,
                      tflite::micro::GetTensorData<int8_t>(input), &filter_dims,
                      &output_dims,
                      tflite::micro::GetTensorData<int8_t>(output)),
      ARM_MATH_SUCCESS);

  return kTfLiteOk;
}

}  // namespace

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus MaxPrepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  OpData* data = static_cast<OpData*>(node->user_data);
  auto* params = reinterpret_cast<TfLitePoolParams*>(node->builtin_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  TF_LITE_ENSURE_STATUS(CalculateOpData(context, params, input, output, data));

  return kTfLiteOk;
}

TfLiteStatus AveragePrepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  OpData* data = static_cast<OpData*>(node->user_data);
  auto* params = reinterpret_cast<TfLitePoolParams*>(node->builtin_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TF_LITE_ENSURE_STATUS(CalculateOpData(context, params, input, output, data));

  if (input->type == kTfLiteInt8) {
    RuntimeShape input_shape = GetTensorShape(input);
    TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);

    RuntimeShape output_shape = GetTensorShape(output);
    TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);

    const int depth = MatchingDim(input_shape, 3, output_shape, 3);
    const int output_width = output_shape.Dims(2);

    const int32_t buffer_size =
        arm_avgpool_s8_get_buffer_size(output_width, depth);

    if (buffer_size > 0) {
      TF_LITE_ENSURE_STATUS(context->RequestScratchBufferInArena(
          context, buffer_size, &data->buffer_idx));
    } else {
      data->buffer_idx = -1;
    }
  }
  return kTfLiteOk;
}

TfLiteStatus AverageEval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLitePoolParams*>(node->builtin_data);

  #ifdef OVERLAY_FW
  TFLITE_DCHECK(node->user_data != nullptr);
  OpData& data = *(static_cast<OpData*>(node->user_data));
  #else
  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));
  #endif

  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kInputTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  #ifdef OVERLAY_FW
  cmsis_nn_pool_params pool_params;
  pool_params.padding.h = data.padding.height;

  volatile PartitionInfo * op_part_info		= &partitioned_exec_state.ops_parts_info[partitioned_exec_state.currentOp];
  volatile PartitionInfo * next_op_part_info	= &partitioned_exec_state.ops_parts_info[partitioned_exec_state.currentOp+1];

//  OVERLAY_CopyInData(&DMAQue, op_part_info, 0, input, &oc_input_tensors[partitioned_exec_state.pingPong]);

	uint32_t nextOpFirstPartCopied			= 0;
	uint32_t output_tensor_produced_bytes	= 0;

//	StartTransfer(&DmaHandle);
	while ( (DMAQue[1].readIdx != DMAQue[1].writeIdx) );

	#ifdef OVERLAY_FILTERS
	const TfLiteEvalTensor * next_op_filter = next_op_part_info->filter;

	if ( next_op_filter ) {
		uint32_t next_op_filterTensorSize	=	next_op_filter->dims->data[0] *
												next_op_filter->dims->data[1] *
												next_op_filter->dims->data[2] *
												next_op_filter->dims->data[3];
		if ( next_op_filterTensorSize < INPUT2_TENSOR_BUFFER_0_SIZE )
			DMA_CopyTensor(&DMAQue[1], next_op_filter, &oc_input2_tensors[0]);
	}
	#endif

	#ifdef OVERLAY_BIASES
	const TfLiteEvalTensor * next_op_bias = next_op_part_info->bias;

	if ( next_op_bias ) {
		uint32_t next_op_biasTensorSize		=	next_op_bias->dims->data[0] * 4;
		if ( next_op_biasTensorSize < BIAS_TENSOR_BUFFER_SIZE )
			DMA_CopyTensor(&DMAQue[1], next_op_bias, &oc_input3_tensor);
	}
	#endif


	// start pipeline
	for ( int32_t p = 0; p < op_part_info->numParts; p++ ) {

	  partitioned_exec_state.currentPart = p;

	  uint16_t timer_val;
	  int uart_buf_len;
//	  char debugPrintPtr[50];

//	  timer_val = HAL_GetTick();

	  // wait till input data copied
	  while ( (DMAQue[0].readIdx != DMAQue[0].writeIdx) );

//	  while ( (DMAQue[0].readIdx != DMAQue[0].writeIdx) && (DMAQue[1].readIdx != DMAQue[1].writeIdx) ) {
//	  while ( DMAQue[0].readIdx != DMAQue[0].writeIdx ) {
//		  if (p != 0)
//			  dma_waiting_pool++;
//		  else
//			  dma_waiting_pool_p0++;
//	  }

//		uart_buf_len = sprintf((char *)debugPrintPtr, "dma_waiting_pool: %u\n", dma_waiting_pool);
//		HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
//		debugPrintPtr += 32;
//		uart_buf_len = sprintf((char *)debugPrintPtr, "dma_wait: %u\n", dma_waiting);
//		HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
//		debugPrintPtr += 32;

//	  timer_val = HAL_GetTick() - timer_val;
//	  uart_buf_len = sprintf((char *)debugPrintPtr, "conv_dma_wait: %u us \r\n", timer_val);
//	  HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
//		debugPrintPtr += 32;

//	  timer_val = HAL_GetTick();

//	  SCB_InvalidateDCache_by_Addr( (uint32_t*) ( ( (uint32_t)&oc_input_tensors_buffers[partitioned_exec_state.pingPong][0] ) & ~(uint32_t)0x1F), bytes_in_partition + 32 );
//
//	  timer_val = HAL_GetTick() - timer_val;
//	  uart_buf_len = sprintf((char *)debugPrintPtr, "conv_cache invalid: %u us \r\n", timer_val);
//	  HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
//		debugPrintPtr += 32;

		// get next input and setup dma before processing copied in input
		if ( p < (op_part_info->numParts-1) )
			OVERLAY_CopyInData(&DMAQue[0], op_part_info, p+1, &oc_input_tensors[!partitioned_exec_state.pingPong], &oc_input2_tensors[!partitioned_exec_state.pingPong]);
		else { // get first part of next operation
			  if (next_op_part_info->input1) {
				  uint32_t startingRow			= next_op_part_info->partitionStart[0];
				  uint32_t endingRow			= next_op_part_info->partitionEnd[0];
				  uint32_t bytes_in_partition	= (endingRow-startingRow)* next_op_part_info->input1->dims->data[0] * next_op_part_info->input1->dims->data[2] * next_op_part_info->input1->dims->data[3];

				  if ( output_tensor_produced_bytes >= bytes_in_partition ) {
					  OVERLAY_CopyInData(&DMAQue[0], next_op_part_info, 0, &oc_input_tensors[!partitioned_exec_state.pingPong], &oc_input2_tensors[!partitioned_exec_state.pingPong]);
					  nextOpFirstPartCopied = 1;
				  }
			  }
		}
//	  // get next input and setup dma before processing copied in input
//	  if ( p < (op_part_info->numParts-1) ) {
//
//		  OVERLAY_CopyInData(&DMAQue, op_part_info, p+1, input, &oc_input_tensors[!partitioned_exec_state.pingPong]);
//
////		  StartTransfer(&DmaHandle);
//	  }

	  // setup ouput tensor and params for operation

	  data.padding.height				= op_part_info->op_part_data[p][OP_DATA_POOL_PAD_IDX];

	  oc_output_tensors.dims->data[1]	= op_part_info->out_partitioned_dim[p];

	  oc_output_tensors.type			= output->type;
	  oc_output_tensors.data.int8		= &output->data.int8[output_tensor_produced_bytes];
	  oc_output_tensors.dims->size		= output->dims->size;
	  oc_output_tensors.dims->data[0]	= output->dims->data[0];
	  oc_output_tensors.dims->data[2]	= output->dims->data[2];
	  oc_output_tensors.dims->data[3]	= output->dims->data[3];

	  output_tensor_produced_bytes += oc_output_tensors.dims->data[0] * oc_output_tensors.dims->data[1] * oc_output_tensors.dims->data[2] * oc_output_tensors.dims->data[3];

	  switch (input->type) {
	    case kTfLiteFloat32:
	      AverageEvalFloat(context, node, params, data, input, output);
	      break;
	    case kTfLiteUInt8:
	    case kTfLiteInt8:
	      AverageEvalQuantized(context, node, params, data, (TfLiteEvalTensor *)&oc_input_tensors[partitioned_exec_state.pingPong], (TfLiteEvalTensor *)&oc_output_tensors);
	      break;
	    default:
	      TF_LITE_KERNEL_LOG(context, "Input type %s is not currently supported",
	                         TfLiteTypeGetName(input->type));
	      return kTfLiteError;
	  }

	  if ( !( p < (op_part_info->numParts-1) ) && !nextOpFirstPartCopied && next_op_part_info->input1 )
		  OVERLAY_CopyInData(&DMAQue[0], next_op_part_info, 0, &oc_input_tensors[!partitioned_exec_state.pingPong], &oc_input2_tensors[!partitioned_exec_state.pingPong]);

	  // switch buffer that was being copied in in background
	  partitioned_exec_state.pingPong = !partitioned_exec_state.pingPong;
	  // restore padding params that we messed around with
	  data.padding.height = pool_params.padding.h;

	}

//	#ifdef OVERLAY_FILTERS
//	const TfLiteEvalTensor * next_op_filter = next_op_part_info->filter;
//
//	if ( next_op_filter ) {
//		uint32_t next_op_filterTensorSize	=	next_op_filter->dims->data[0] *
//												next_op_filter->dims->data[1] *
//												next_op_filter->dims->data[2] *
//												next_op_filter->dims->data[3];
//		if ( next_op_filterTensorSize < TENSOR_BUFFER_SIZE )
//			DMA_CopyTensor(&DMAQue[1], next_op_filter, &oc_input2_tensors[0]);
//	}
//	#endif

  #else
  // Inputs and outputs share the same type, guaranteed by the converter.
  switch (input->type) {
    case kTfLiteFloat32:
      AverageEvalFloat(context, node, params, data, input, output);
      break;
    case kTfLiteUInt8:
    case kTfLiteInt8:
      AverageEvalQuantized(context, node, params, data, input, output);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Input type %s is not currently supported",
                         TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
  #endif
  return kTfLiteOk;
}

TfLiteStatus MaxEval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLitePoolParams*>(node->builtin_data);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kInputTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  switch (input->type) {
    case kTfLiteFloat32:
      MaxEvalFloat(context, node, params, data, input, output);
      break;
    case kTfLiteUInt8:
      MaxEvalQuantizedUInt8(context, node, params, data, input, output);
      break;
    case kTfLiteInt8:
      MaxEvalInt8(context, node, params, data, input, output);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s not currently supported.",
                         TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}

}  // namespace pooling

TfLiteRegistration Register_AVERAGE_POOL_2D() {
  return {/*init=*/pooling::Init,
          /*free=*/nullptr,
          /*prepare=*/pooling::AveragePrepare,
          /*invoke=*/pooling::AverageEval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

TfLiteRegistration Register_MAX_POOL_2D() {
  return {/*init=*/pooling::Init,
          /*free=*/nullptr,
          /*prepare=*/pooling::MaxPrepare,
          /*invoke=*/pooling::MaxEval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace micro
}  // namespace ops
}  // namespace tflite
