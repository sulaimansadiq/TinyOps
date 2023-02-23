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

#include "tensorflow/lite/kernels/internal/reference/mul.h"

#include "CMSIS/NN/Include/arm_nnfunctions.h"
#include "tensorflow/lite/kernels/internal/quantization_util.h"
#include "tensorflow/lite/kernels/internal/reference/integer_ops/mul.h"
#include "tensorflow/lite/kernels/internal/reference/process_broadcast_shapes.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"
#include "tensorflow/lite/micro/memory_helpers.h"

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
namespace mul {

constexpr int kInput1Tensor = 0;
constexpr int kInput2Tensor = 1;
constexpr int kOutputTensor = 0;

struct OpData {
  int32_t output_activation_min;
  int32_t output_activation_max;

  int32_t output_multiplier;
  int output_shift;

  // Cached tensor zero point values for quantized operations.
  int32_t input1_zero_point;
  int32_t input2_zero_point;
  int32_t output_zero_point;

  float output_activation_min_f32;
  float output_activation_max_f32;
};

TfLiteStatus CalculateOpData(TfLiteContext* context, TfLiteNode* node,
                             TfLiteMulParams* params, OpData* data) {
  const TfLiteTensor* input1 = GetInput(context, node, kInput1Tensor);
  TF_LITE_ENSURE(context, input1 != nullptr);
  const TfLiteTensor* input2 = GetInput(context, node, kInput2Tensor);
  TF_LITE_ENSURE(context, input2 != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input2->type);

  if (output->type == kTfLiteUInt8 || output->type == kTfLiteInt8) {
    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->output_activation_min,
        &data->output_activation_max));

    double real_multiplier = static_cast<double>(input1->params.scale) *
                             static_cast<double>(input2->params.scale) /
                             static_cast<double>(output->params.scale);
    QuantizeMultiplier(real_multiplier, &data->output_multiplier,
                       &data->output_shift);

    data->input1_zero_point = input1->params.zero_point;
    data->input2_zero_point = input2->params.zero_point;
    data->output_zero_point = output->params.zero_point;
  } else {
    CalculateActivationRange(params->activation,
                             &data->output_activation_min_f32,
                             &data->output_activation_max_f32);
  }

  return kTfLiteOk;
}

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLiteMulParams*>(node->builtin_data);

  TFLITE_DCHECK(node->user_data != nullptr);
  OpData* data = static_cast<OpData*>(node->user_data);

  return CalculateOpData(context, node, params, data);
}

void EvalQuantized(TfLiteContext* context, TfLiteNode* node, const OpData& data,
                   const TfLiteEvalTensor* input1,
                   const TfLiteEvalTensor* input2, TfLiteEvalTensor* output) {
  tflite::ArithmeticParams op_params;
  op_params.quantized_activation_min = data.output_activation_min;
  op_params.quantized_activation_max = data.output_activation_max;
  op_params.input1_offset = -data.input1_zero_point;
  op_params.input2_offset = -data.input2_zero_point;
  op_params.output_offset = data.output_zero_point;
  op_params.output_multiplier = data.output_multiplier;
  op_params.output_shift = data.output_shift;

  bool need_broadcast = reference_ops::ProcessBroadcastShapes(
      tflite::micro::GetTensorShape(input1),
      tflite::micro::GetTensorShape(input2), &op_params);

#define TF_LITE_MUL(type, opname, dtype)                         \
  type::opname(op_params, tflite::micro::GetTensorShape(input1), \
               tflite::micro::GetTensorData<dtype>(input1),      \
               tflite::micro::GetTensorShape(input2),            \
               tflite::micro::GetTensorData<dtype>(input2),      \
               tflite::micro::GetTensorShape(output),            \
               tflite::micro::GetTensorData<dtype>(output));

  if (output->type == kTfLiteInt8) {
    if (need_broadcast) {
      TF_LITE_MUL(reference_integer_ops, BroadcastMul4DSlow, int8_t);
    } else {
      arm_elementwise_mul_s8(
          tflite::micro::GetTensorData<int8_t>(input1),
          tflite::micro::GetTensorData<int8_t>(input2), op_params.input1_offset,
          op_params.input2_offset, tflite::micro::GetTensorData<int8_t>(output),
          op_params.output_offset, op_params.output_multiplier,
          op_params.output_shift, op_params.quantized_activation_min,
          op_params.quantized_activation_max,
          MatchingElementsSize(tflite::micro::GetTensorShape(input1),
                               tflite::micro::GetTensorShape(input2),
                               tflite::micro::GetTensorShape(output)));
    }
  } else if (output->type == kTfLiteUInt8) {
    if (need_broadcast) {
      TF_LITE_MUL(reference_integer_ops, BroadcastMul4DSlow, uint8_t);
    } else {
      TF_LITE_MUL(reference_integer_ops, Mul, uint8_t);
    }
  }
#undef TF_LITE_MUL
}

void EvalFloat(TfLiteContext* context, TfLiteNode* node,
               TfLiteMulParams* params, const OpData& data,
               const TfLiteEvalTensor* input1, const TfLiteEvalTensor* input2,
               TfLiteEvalTensor* output) {
  tflite::ArithmeticParams op_params;
  op_params.float_activation_min = data.output_activation_min_f32;
  op_params.float_activation_max = data.output_activation_max_f32;

  bool need_broadcast = reference_ops::ProcessBroadcastShapes(
      tflite::micro::GetTensorShape(input1),
      tflite::micro::GetTensorShape(input2), &op_params);
#define TF_LITE_MUL(opname)                                               \
  reference_ops::opname(op_params, tflite::micro::GetTensorShape(input1), \
                        tflite::micro::GetTensorData<float>(input1),      \
                        tflite::micro::GetTensorShape(input2),            \
                        tflite::micro::GetTensorData<float>(input2),      \
                        tflite::micro::GetTensorShape(output),            \
                        tflite::micro::GetTensorData<float>(output));

  if (need_broadcast) {
    TF_LITE_MUL(BroadcastMul4DSlow);
  } else {
    TF_LITE_MUL(Mul);
  }
#undef TF_LITE_MUL
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLiteMulParams*>(node->builtin_data);

  const TfLiteEvalTensor* input1 =
      tflite::micro::GetEvalInput(context, node, kInput1Tensor);
  const TfLiteEvalTensor* input2 =
      tflite::micro::GetEvalInput(context, node, kInput2Tensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

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

		switch (input1->type) {
		case kTfLiteUInt8:
		case kTfLiteInt8:
		  EvalQuantized(context, node, data, input1, input2, output);
		  break;
		case kTfLiteFloat32:
		  EvalFloat(context, node, params, data, input1, input2, output);
		  break;
		default:
		  TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
							 TfLiteTypeGetName(input1->type), input1->type);
		  return kTfLiteError;
		}

		if ( !( p < (op_part_info->numParts-1) ) && !nextOpFirstPartCopied && next_op_part_info->input1 )
			OVERLAY_CopyInData(&DMAQue[0], next_op_part_info, 0, &oc_input_tensors[!partitioned_exec_state.pingPong], &oc_input2_tensors[!partitioned_exec_state.pingPong]);

		// switch buffer that was being copied in in background
		partitioned_exec_state.pingPong = !partitioned_exec_state.pingPong;

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

	switch (input1->type) {
	case kTfLiteUInt8:
	case kTfLiteInt8:
	  EvalQuantized(context, node, data, input1, input2, output);
	  break;
	case kTfLiteFloat32:
	  EvalFloat(context, node, params, data, input1, input2, output);
	  break;
	default:
	  TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
						 TfLiteTypeGetName(input1->type), input1->type);
	  return kTfLiteError;
	}
	#endif

  return kTfLiteOk;
}
}  // namespace mul

TfLiteRegistration Register_MUL() {
  return {/* Init=*/mul::Init,
          /* Free=*/nullptr,
          /* Prepare=*/mul::Prepare,
          /*invoke=*/mul::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace micro
}  // namespace ops
}  // namespace tflite
