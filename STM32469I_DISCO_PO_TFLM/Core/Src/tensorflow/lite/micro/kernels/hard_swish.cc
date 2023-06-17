/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/lite/kernels/internal/reference/hard_swish.h"

#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/common.h"
#include "tensorflow/lite/kernels/internal/quantization_util.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/kernels/internal/types.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/kernels/op_macros.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"
#include "tensorflow/lite/micro/micro_utils.h"

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
#endif

namespace tflite {
namespace ops {
namespace micro {
namespace hard_swish {

constexpr int kInputTensor = 0;
constexpr int kOutputTensor = 0;

void* HardSwishInit(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(HardSwishParams));
}

TfLiteStatus HardSwishPrepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  if (input->type == kTfLiteUInt8 || input->type == kTfLiteInt8) {
    HardSwishParams* params = static_cast<HardSwishParams*>(node->user_data);

    params->input_zero_point = input->params.zero_point;
    params->output_zero_point = output->params.zero_point;

    const float input_scale = input->params.scale;
    const float hires_input_scale = (1.0f / 128.0f) * input_scale;
    const float reluish_scale = 3.0f / 32768.0f;
    const float output_scale = output->params.scale;

    const double output_multiplier =
        static_cast<double>(hires_input_scale / output_scale);
    int32_t output_multiplier_fixedpoint_int32;
    QuantizeMultiplier(output_multiplier, &output_multiplier_fixedpoint_int32,
                       &params->output_multiplier_exponent);
    DownScaleInt32ToInt16Multiplier(
        output_multiplier_fixedpoint_int32,
        &params->output_multiplier_fixedpoint_int16);

    TF_LITE_ENSURE(context, params->output_multiplier_exponent <= 0);

    const double reluish_multiplier =
        static_cast<double>(hires_input_scale / reluish_scale);
    int32_t reluish_multiplier_fixedpoint_int32;
    QuantizeMultiplier(reluish_multiplier, &reluish_multiplier_fixedpoint_int32,
                       &params->reluish_multiplier_exponent);
    DownScaleInt32ToInt16Multiplier(
        reluish_multiplier_fixedpoint_int32,
        &params->reluish_multiplier_fixedpoint_int16);
  }

  return kTfLiteOk;
}

TfLiteStatus HardSwishEval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kInputTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);
  HardSwishParams* params = static_cast<HardSwishParams*>(node->user_data);

	#ifdef OVERLAY_FW
	volatile PartitionInfo * op_part_info		= &partitioned_exec_state.ops_parts_info[partitioned_exec_state.currentOp];
	volatile PartitionInfo * next_op_part_info	= &partitioned_exec_state.ops_parts_info[partitioned_exec_state.currentOp+1];

	//	OVERLAY_CopyInData(&DMAQue, op_part_info, 0, input1, &oc_input_tensors[partitioned_exec_state.pingPong], input2, &oc_input2_tensors[partitioned_exec_state.pingPong]);

	uint32_t nextOpFirstPartCopied			= 0;
	uint32_t output_tensor_produced_bytes	= 0;

	while ( (DMAQue[1].readIdx != DMAQue[1].writeIdx) );

	// start pipeline
	for ( int32_t p = 0; p < op_part_info->numParts; p++ ) {

		partitioned_exec_state.currentPart = p;

		int uart_buf_len;
	//		char debugPrintPtr[50];

		// wait till input data copied
		while ( (DMAQue[0].readIdx != DMAQue[0].writeIdx) );
		while ( (DMAQue[1].readIdx != DMAQue[1].writeIdx) );

	//		while ( (DMAQue[0].readIdx != DMAQue[0].writeIdx) && (DMAQue[1].readIdx != DMAQue[1].writeIdx) ); //{
	//		while ( DMAQue[0].readIdx != DMAQue[0].writeIdx ) {
	//			if (p != 0)
	//				dma_waiting_add++;
	//			else
	//				dma_waiting_add_p0++;
	//		}

	//		uart_buf_len = sprintf((char *)debugPrintPtr, "dma_waiting_add: %u\n", dma_waiting_add);
	//		HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
	//		debugPrintPtr += 32;
	//		uart_buf_len = sprintf((char *)debugPrintPtr, "dma_wait: %u\n", dma_waiting);
	//		HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
	//		debugPrintPtr += 32;

	//		SCB_InvalidateDCache_by_Addr( (uint32_t*) ( ( (uint32_t)&oc_input_tensors_buffers[partitioned_exec_state.pingPong][0] ) & ~(uint32_t)0x1F), bytes_in_partition + 32 );

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
	//		if ( p < (op_part_info->numParts-1) ) {
	//
	//			OVERLAY_CopyInData(&DMAQue, op_part_info, p+1, input1, &oc_input_tensors[!partitioned_exec_state.pingPong], input2, &oc_input2_tensors[!partitioned_exec_state.pingPong]);
	//		}

		oc_output_tensors.dims->data[1]	= op_part_info->out_partitioned_dim[p];

		oc_output_tensors.type			= output->type;
		oc_output_tensors.data.int8		= &output->data.int8[output_tensor_produced_bytes];
		oc_output_tensors.dims->size	= output->dims->size;
		oc_output_tensors.dims->data[0]	= output->dims->data[0];
		oc_output_tensors.dims->data[2]	= output->dims->data[2];
		oc_output_tensors.dims->data[3]	= output->dims->data[3];

		output_tensor_produced_bytes += oc_output_tensors.dims->data[0] * oc_output_tensors.dims->data[1] * oc_output_tensors.dims->data[2] * oc_output_tensors.dims->data[3];

		switch (input->type) {
		case kTfLiteFloat32: {
		  tflite::reference_ops::HardSwish<float>(
			  tflite::micro::GetTensorShape(input),
			  tflite::micro::GetTensorData<float>(input),
			  tflite::micro::GetTensorShape(output),
			  tflite::micro::GetTensorData<float>(output));
		} break;
		case kTfLiteUInt8: {
		  tflite::reference_ops::HardSwish<uint8_t>(
			  *params, tflite::micro::GetTensorShape(input),
			  tflite::micro::GetTensorData<uint8_t>(input),
			  tflite::micro::GetTensorShape(output),
			  tflite::micro::GetTensorData<uint8_t>(output));
		} break;
		case kTfLiteInt8: {
		  tflite::reference_ops::HardSwish<int8_t>(
			  *params, tflite::micro::GetTensorShape(input),
			  tflite::micro::GetTensorData<int8_t>(input),
			  tflite::micro::GetTensorShape(output),
			  tflite::micro::GetTensorData<int8_t>(output));
		} break;
		default: {
		  TF_LITE_KERNEL_LOG(
			  context,
			  "Only float32/int8_t/uint8_t are supported currently, got %s",
			  TfLiteTypeGetName(input->type));
		  return kTfLiteError;
		}

		if ( !( p < (op_part_info->numParts-1) ) && !nextOpFirstPartCopied && next_op_part_info->input1 )
			OVERLAY_CopyInData(&DMAQue[0], next_op_part_info, 0, &oc_input_tensors[!partitioned_exec_state.pingPong], &oc_input2_tensors[!partitioned_exec_state.pingPong]);

		// switch buffer that was being copied in in background
		partitioned_exec_state.pingPong = !partitioned_exec_state.pingPong;

		}

	}

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
	const TfLiteEvalTensor * next_op_bias	= next_op_part_info->bias;

	if ( next_op_bias ) {
		uint32_t next_op_biasTensorSize		=	next_op_bias->dims->data[0] * 4;
		if ( next_op_biasTensorSize < BIAS_TENSOR_BUFFER_SIZE )
			DMA_CopyTensor(&DMAQue[1], next_op_bias, &oc_input3_tensor);
	}
	#endif

	#else

	switch (input->type) {
	case kTfLiteFloat32: {
	  tflite::reference_ops::HardSwish<float>(
		  tflite::micro::GetTensorShape(input),
		  tflite::micro::GetTensorData<float>(input),
		  tflite::micro::GetTensorShape(output),
		  tflite::micro::GetTensorData<float>(output));
	} break;
	case kTfLiteUInt8: {
	  tflite::reference_ops::HardSwish<uint8_t>(
		  *params, tflite::micro::GetTensorShape(input),
		  tflite::micro::GetTensorData<uint8_t>(input),
		  tflite::micro::GetTensorShape(output),
		  tflite::micro::GetTensorData<uint8_t>(output));
	} break;
	case kTfLiteInt8: {
	  tflite::reference_ops::HardSwish<int8_t>(
		  *params, tflite::micro::GetTensorShape(input),
		  tflite::micro::GetTensorData<int8_t>(input),
		  tflite::micro::GetTensorShape(output),
		  tflite::micro::GetTensorData<int8_t>(output));
	} break;
	default: {
	  TF_LITE_KERNEL_LOG(
		  context,
		  "Only float32/int8_t/uint8_t are supported currently, got %s",
		  TfLiteTypeGetName(input->type));
	  return kTfLiteError;
	}

	}

	#endif

	return kTfLiteOk;
}

}  // namespace hard_swish

TfLiteRegistration Register_HARD_SWISH() {
  return {/*init=*/hard_swish::HardSwishInit,
          /*free=*/nullptr,
          /*prepare=*/hard_swish::HardSwishPrepare,
          /*invoke=*/hard_swish::HardSwishEval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace micro
}  // namespace ops
}  // namespace tflite
