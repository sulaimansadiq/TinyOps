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
#include "tensorflow/lite/micro/micro_interpreter.h"

#include "main.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>

#include "flatbuffers/flatbuffers.h"  // from @flatbuffers
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/core/api/error_reporter.h"
#include "tensorflow/lite/core/api/tensor_utils.h"
#include "tensorflow/lite/micro/memory_helpers.h"
#include "tensorflow/lite/micro/micro_allocator.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/schema/schema_generated.h"

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef  htim10;

#include "tensorflow/lite/micro/kernels/kernel_util.h"
#include "overlay_fw.h"

#ifdef OVERLAY_FW
extern volatile DMACopyQue DMAQue[NUM_DMA_STREAMS_USED];

extern volatile PartitionedExecutionState partitioned_exec_state;

extern volatile TfLiteEvalTensor oc_input_tensors[OP_MAX_INPUT_SIZE];
//extern volatile uint8_t oc_input_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
extern volatile uint8_t * oc_input_tensors_buffers[OP_MAX_INPUT_SIZE];
extern volatile int oc_input_tensors_dims_data[OP_MAX_INPUT_SIZE][MAX_NUM_DIMS];
extern volatile TfLiteIntArray oc_input_tensors_dims_array_0;
extern volatile TfLiteIntArray oc_input_tensors_dims_array_1;

extern volatile TfLiteEvalTensor oc_input2_tensors[OP_MAX_INPUT_SIZE];
//extern volatile uint8_t oc_input2_tensors_buffers_0[INPUT2_TENSOR_BUFFER_0_SIZE];
//extern volatile uint8_t oc_input2_tensors_buffers_1[INPUT2_TENSOR_BUFFER_1_SIZE];
extern volatile uint8_t * oc_input2_tensors_buffers_0;
extern volatile uint8_t * oc_input2_tensors_buffers_1;
//extern volatile uint8_t oc_input2_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
extern volatile int oc_input2_tensors_dims_data[OP_MAX_INPUT_SIZE][MAX_NUM_DIMS];
extern volatile TfLiteIntArray oc_input2_tensors_dims_array_0;
extern volatile TfLiteIntArray oc_input2_tensors_dims_array_1;

#ifdef OVERLAY_BIASES
extern volatile TfLiteEvalTensor oc_input3_tensor;
//extern volatile uint8_t oc_input3_tensor_buffer[BIAS_TENSOR_BUFFER_SIZE];
extern volatile uint8_t * oc_input3_tensor_buffer;
extern volatile int oc_input3_tensor_dims_data[MAX_NUM_DIMS];
extern volatile TfLiteIntArray oc_input3_tensor_dims_array;
#endif

extern volatile uint8_t * quant_out_multiplier;
extern volatile uint8_t * quant_out_shift;

extern uint8_t tinyops_arena[];

#ifdef OVERLAY_OUTPUT_TENSORS
extern volatile TfLiteEvalTensor oc_output2_tensors[OP_MAX_INPUT_SIZE];
extern volatile uint8_t oc_output2_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
extern volatile int oc_output2_tensors_dims_data[OP_MAX_INPUT_SIZE][MAX_NUM_DIMS];
extern volatile TfLiteIntArray oc_output2_tensors_dims_array_0;
extern volatile TfLiteIntArray oc_output2_tensors_dims_array_1;
#endif

#ifdef BENCHMARK_TINYOPS_STATS
extern	TinyOpsBuffSizes	tinyOpsBuffSizes;
extern	uint32_t			tinyOpsPartFactor;
#endif

extern volatile TfLiteEvalTensor oc_output_tensors;
//extern uint8_t oc_output_tensors_buffers[TENSOR_BUFFER_SIZE];
extern volatile int oc_output_tensors_dims_data[4];
extern volatile TfLiteIntArray oc_output_tensors_dims_array;

#define DEBUG_DUMP_DMA_WAIT_CONV
#ifdef DEBUG_DUMP_DMA_WAIT_CONV
extern uint32_t debug_dump_dma_waiting_convp0_time;
extern uint32_t debug_dump_dma_waiting_convp0[800];
extern uint32_t debug_dump_dma_waiting_conv[800];
extern uint32_t debug_dump_dma_waiting_conv_idx;
#endif

#endif

namespace tflite {
namespace {

#ifndef TF_LITE_STRIP_ERROR_STRINGS
const char* OpNameFromRegistration(const TfLiteRegistration* registration) {
  if (registration->builtin_code == BuiltinOperator_CUSTOM) {
    return registration->custom_name;
  } else {
    return EnumNameBuiltinOperator(BuiltinOperator(registration->builtin_code));
  }
}
#endif  // !defined(TF_LITE_STRIP_ERROR_STRINGS)

}  // namespace

namespace internal {

ContextHelper::ContextHelper(ErrorReporter* error_reporter,
                             MicroAllocator* allocator, const Model* model)
    : allocator_(allocator), error_reporter_(error_reporter), model_(model) {}

void* ContextHelper::AllocatePersistentBuffer(TfLiteContext* ctx,
                                              size_t bytes) {
  return reinterpret_cast<ContextHelper*>(ctx->impl_)
      ->allocator_->AllocatePersistentBuffer(bytes);
}

TfLiteStatus ContextHelper::RequestScratchBufferInArena(TfLiteContext* ctx,
                                                        size_t bytes,
                                                        int* buffer_idx) {
  ContextHelper* helper = reinterpret_cast<ContextHelper*>(ctx->impl_);
  return helper->allocator_->RequestScratchBufferInArena(bytes, buffer_idx);
}

void* ContextHelper::GetScratchBuffer(TfLiteContext* ctx, int buffer_idx) {
  ContextHelper* helper = reinterpret_cast<ContextHelper*>(ctx->impl_);
  ScratchBufferHandle* handle = helper->scratch_buffer_handles_ + buffer_idx;
  return handle->data;
}

void ContextHelper::ReportOpError(struct TfLiteContext* context,
                                  const char* format, ...) {
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  ContextHelper* helper = static_cast<ContextHelper*>(context->impl_);
  va_list args;
  va_start(args, format);
  TF_LITE_REPORT_ERROR(helper->error_reporter_, format, args);
  va_end(args);
#endif
}

TfLiteTensor* ContextHelper::GetTensor(const struct TfLiteContext* context,
                                       int tensor_idx) {
  ContextHelper* helper = static_cast<ContextHelper*>(context->impl_);
  return helper->allocator_->AllocateTempTfLiteTensor(
      helper->model_, helper->eval_tensors_, tensor_idx);
}

TfLiteEvalTensor* ContextHelper::GetEvalTensor(
    const struct TfLiteContext* context, int tensor_idx) {
  ContextHelper* helper = reinterpret_cast<ContextHelper*>(context->impl_);
  return &helper->eval_tensors_[tensor_idx];
}

void ContextHelper::SetTfLiteEvalTensors(TfLiteEvalTensor* eval_tensors) {
  eval_tensors_ = eval_tensors;
}

void ContextHelper::SetScratchBufferHandles(
    ScratchBufferHandle* scratch_buffer_handles) {
  scratch_buffer_handles_ = scratch_buffer_handles;
}

}  // namespace internal

MicroInterpreter::MicroInterpreter(const Model* model,
                                   const MicroOpResolver& op_resolver,
                                   uint8_t* tensor_arena,
                                   size_t tensor_arena_size,
                                   ErrorReporter* error_reporter,
                                   MicroProfiler* profiler)
    : model_(model),
      op_resolver_(op_resolver),
      error_reporter_(error_reporter),
      allocator_(*MicroAllocator::Create(tensor_arena, tensor_arena_size,
                                         error_reporter)),
      tensors_allocated_(false),
      initialization_status_(kTfLiteError),
      eval_tensors_(nullptr),
      context_helper_(error_reporter_, &allocator_, model),
      input_tensors_(nullptr),
      output_tensors_(nullptr) {
  Init(profiler);
}

MicroInterpreter::MicroInterpreter(const Model* model,
                                   const MicroOpResolver& op_resolver,
                                   MicroAllocator* allocator,
                                   ErrorReporter* error_reporter,
                                   MicroProfiler* profiler)
    : model_(model),
      op_resolver_(op_resolver),
      error_reporter_(error_reporter),
      allocator_(*allocator),
      tensors_allocated_(false),
      initialization_status_(kTfLiteError),
      eval_tensors_(nullptr),
      context_helper_(error_reporter_, &allocator_, model),
      input_tensors_(nullptr),
      output_tensors_(nullptr) {
  Init(profiler);
}

MicroInterpreter::~MicroInterpreter() {
  if (node_and_registrations_ != nullptr) {
    for (size_t i = 0; i < subgraph_->operators()->size(); ++i) {
      TfLiteNode* node = &(node_and_registrations_[i].node);
      const TfLiteRegistration* registration =
          node_and_registrations_[i].registration;
      // registration is allocated outside the interpreter, so double check to
      // make sure it's not nullptr;
      if (registration != nullptr && registration->free != nullptr) {
        registration->free(&context_, node->user_data);
      }
    }
  }
}

void MicroInterpreter::Init(MicroProfiler* profiler) {
  const flatbuffers::Vector<flatbuffers::Offset<SubGraph>>* subgraphs =
      model_->subgraphs();
  if (subgraphs->size() != 1) {
    TF_LITE_REPORT_ERROR(error_reporter_,
                         "Only 1 subgraph is currently supported.\n");
    initialization_status_ = kTfLiteError;
    return;
  }
  subgraph_ = (*subgraphs)[0];

  context_.impl_ = static_cast<void*>(&context_helper_);
  context_.ReportError = context_helper_.ReportOpError;
  context_.GetTensor = context_helper_.GetTensor;
  context_.GetEvalTensor = context_helper_.GetEvalTensor;
  context_.recommended_num_threads = 1;
  context_.profiler = profiler;

  initialization_status_ = kTfLiteOk;
}

TfLiteStatus MicroInterpreter::AllocateTensors() {

  int uart_buf_len;
  char uart_buf[50];

  if (allocator_.StartModelAllocation(model_, op_resolver_,
                                      &node_and_registrations_,
                                      &eval_tensors_) != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter_,
                         "Failed starting model allocation.\n");
    initialization_status_ = kTfLiteError;
    return kTfLiteError;
  }

  // Update the pointer now that TfLiteEvalTensor allocation has completed on
  // the context helper.
  // TODO(b/16157777): This call would not be needed if ContextHelper rolled
  // into the interpreter.
  context_helper_.SetTfLiteEvalTensors(eval_tensors_);
  context_.tensors_size = subgraph_->tensors()->size();

  // Only allow AllocatePersistentBuffer in Init stage.
  context_.AllocatePersistentBuffer = context_helper_.AllocatePersistentBuffer;
  context_.RequestScratchBufferInArena = nullptr;
  context_.GetScratchBuffer = nullptr;

  for (size_t i = 0; i < subgraph_->operators()->size(); ++i) {
    auto* node = &(node_and_registrations_[i].node);
    auto* registration = node_and_registrations_[i].registration;
    size_t init_data_size;
    const char* init_data;
    if (registration->builtin_code == BuiltinOperator_CUSTOM) {
      init_data = reinterpret_cast<const char*>(node->custom_initial_data);
      init_data_size = node->custom_initial_data_size;
    } else {
      init_data = reinterpret_cast<const char*>(node->builtin_data);
      init_data_size = 0;
    }
    if (registration->init) {
      node->user_data =
          registration->init(&context_, init_data, init_data_size);
    }
  }

//	uart_buf_len = sprintf(uart_buf, "1 \n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	// Both AllocatePersistentBuffer and RequestScratchBufferInArena is
  // available in Prepare stage.
  context_.RequestScratchBufferInArena =
      context_helper_.RequestScratchBufferInArena;
  for (size_t i = 0; i < subgraph_->operators()->size(); ++i) {
//	TF_LITE_REPORT_ERROR(error_reporter_, "loop iter: %u", i);
//	uart_buf_len = sprintf(uart_buf, "%u \n", i);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
    auto* node = &(node_and_registrations_[i].node);
    auto* registration = node_and_registrations_[i].registration;
    if (registration->prepare) {
      TfLiteStatus prepare_status = registration->prepare(&context_, node);
      if (prepare_status != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(
            error_reporter_,
            "Node %s (number %df) failed to prepare with status %d",
            OpNameFromRegistration(registration), i, prepare_status);
        return kTfLiteError;
      }
    }
    allocator_.FinishPrepareNodeAllocations(/*node_id=*/i);
  }

//	uart_buf_len = sprintf(uart_buf, "2 \n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

  // Prepare is done, we're ready for Invoke. Memory allocation is no longer
  // allowed. Kernels can only fetch scratch buffers via GetScratchBuffer.
  context_.AllocatePersistentBuffer = nullptr;
  context_.RequestScratchBufferInArena = nullptr;
  context_.GetScratchBuffer = context_helper_.GetScratchBuffer;

  TF_LITE_ENSURE_OK(&context_,
                    allocator_.FinishModelAllocation(model_, eval_tensors_,
                                                     &scratch_buffer_handles_));
  // TODO(b/16157777): Remove this when ContextHelper is rolled into this class.
  context_helper_.SetScratchBufferHandles(scratch_buffer_handles_);

  // TODO(b/162311891): Drop these allocations when the interpreter supports
  // handling buffers from TfLiteEvalTensor.
  input_tensors_ =
      reinterpret_cast<TfLiteTensor**>(allocator_.AllocatePersistentBuffer(
          sizeof(TfLiteTensor*) * inputs_size()));
  if (input_tensors_ == nullptr) {
    TF_LITE_REPORT_ERROR(
        error_reporter_,
        "Failed to allocate memory for context->input_tensors_, "
        "%d bytes required",
        sizeof(TfLiteTensor*) * inputs_size());
    return kTfLiteError;
  }

//	uart_buf_len = sprintf(uart_buf, "3 \n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

  for (size_t i = 0; i < inputs_size(); ++i) {
    input_tensors_[i] = allocator_.AllocatePersistentTfLiteTensor(
        model_, eval_tensors_, inputs().Get(i));
    if (input_tensors_[i] == nullptr) {
      TF_LITE_REPORT_ERROR(error_reporter_,
                           "Failed to initialize input tensor %d", i);
      return kTfLiteError;
    }
  }

  // TODO(b/162311891): Drop these allocations when the interpreter supports
  // handling buffers from TfLiteEvalTensor.
  output_tensors_ =
      reinterpret_cast<TfLiteTensor**>(allocator_.AllocatePersistentBuffer(
          sizeof(TfLiteTensor*) * outputs_size()));
  if (output_tensors_ == nullptr) {
    TF_LITE_REPORT_ERROR(
        error_reporter_,
        "Failed to allocate memory for context->output_tensors_, "
        "%d bytes required",
        sizeof(TfLiteTensor*) * outputs_size());
    return kTfLiteError;
  }

  for (size_t i = 0; i < outputs_size(); ++i) {
    output_tensors_[i] = allocator_.AllocatePersistentTfLiteTensor(
        model_, eval_tensors_, outputs().Get(i));
    if (output_tensors_[i] == nullptr) {
      TF_LITE_REPORT_ERROR(error_reporter_,
                           "Failed to initialize output tensor %d", i);
      return kTfLiteError;
    }
  }

	TF_LITE_ENSURE_STATUS(ResetVariableTensors());

	tensors_allocated_ = true;

	#ifdef OVERLAY_FW
	oc_input_tensors[0].dims		= (TfLiteIntArray *)&oc_input_tensors_dims_array_0;
	oc_input_tensors[1].dims		= (TfLiteIntArray *)&oc_input_tensors_dims_array_1;

	oc_input2_tensors[0].dims		= (TfLiteIntArray *)&oc_input2_tensors_dims_array_0;
	oc_input2_tensors[1].dims		= (TfLiteIntArray *)&oc_input2_tensors_dims_array_1;

	#ifdef OVERLAY_BIASES
	oc_input3_tensor.dims			= (TfLiteIntArray *)&oc_input3_tensor_dims_array;
	#endif

	oc_output_tensors.dims			= (TfLiteIntArray *)&oc_output_tensors_dims_array;

	#ifdef OVERLAY_OUTPUT_TENSORS
	oc_output2_tensors[0].data.data	= (void *)&oc_output2_tensors_buffers[0][0];
	oc_output2_tensors[1].data.data	= (void *)&oc_output2_tensors_buffers[1][0];
	oc_output2_tensors[0].dims		= (TfLiteIntArray *)&oc_output2_tensors_dims_array_0;
	oc_output2_tensors[1].dims		= (TfLiteIntArray *)&oc_output2_tensors_dims_array_1;
	#endif

	partitioned_exec_state.totalOps = subgraph_->operators()->size();
	partitioned_exec_state.currentOp = FIRST_OPERATION_STATE_VAL;
	partitioned_exec_state.currentPart = FIRST_PART_STATE_VAL;

	partitioned_exec_state.nodes_and_regs_ = node_and_registrations_;

	partitioned_exec_state.pingPong = 0;

//	uart_buf_len = sprintf(uart_buf, "debug_dump_dma_waiting_conv: %X us \r\n", debug_dump_dma_waiting_conv);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	uart_buf_len = sprintf(uart_buf, "debug_dump_dma_waiting_convp0: %X us \r\n", debug_dump_dma_waiting_convp0);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	for (uint32_t i = 0; i<partitioned_exec_state.totalOps; i++) {

		partitioned_exec_state.ops_parts_info[i].input1	= 0;
		partitioned_exec_state.ops_parts_info[i].input2	= 0;
		partitioned_exec_state.ops_parts_info[i].filter	= 0;
		partitioned_exec_state.ops_parts_info[i].bias	= 0;

//		uart_buf_len = sprintf(uart_buf, "parting op: %u us \r\n", i);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

		auto * node = &(node_and_registrations_[i].node);
		auto * registration = (node_and_registrations_[i].registration);

//		PartitionOp( &context_, node, registration, &partitioned_exec_state.ops_parts_info[i] );

//		if ( registration->builtin_code == BuiltinOperator_CONV_2D ||
//			 registration->builtin_code == BuiltinOperator_DEPTHWISE_CONV_2D )
//			PartitionConvOp( &context_, node, registration, &partitioned_exec_state.ops_parts_info[i], PARTITION_FACTOR );
//		else if ( registration->builtin_code == BuiltinOperator_PAD )
//			PartitionPadOp( &context_, node, registration, &partitioned_exec_state.ops_parts_info[i], PARTITION_FACTOR );
//		else if ( registration->builtin_code == BuiltinOperator_ADD )
//			PartitionAddOp( &context_, node, registration, &partitioned_exec_state.ops_parts_info[i], PARTITION_FACTOR );
//		else if ( registration->builtin_code == BuiltinOperator_AVERAGE_POOL_2D )
//			PartitionPoolOp( &context_, node, registration, &partitioned_exec_state.ops_parts_info[i], PARTITION_FACTOR );
	}

	if ( PartitionOp( &context_, node_and_registrations_, &partitioned_exec_state ) ) {

		uint32_t offset = 32;

		oc_input_tensors_buffers[0] = &tinyops_arena[offset];	// this should already by aligned to 32 bytes
		oc_input_tensors_buffers[0] = (uint8_t *)( (uintptr_t)oc_input_tensors_buffers[0] & ~(uintptr_t)(0x0F) );
		offset += tinyOpsBuffSizes.partitionBuff1Size + MEM_ALIGNMENT_PAINT;

		oc_input_tensors_buffers[1] = &tinyops_arena[offset];
		oc_input_tensors_buffers[1] = (uint8_t *)( (uintptr_t)oc_input_tensors_buffers[1] & ~(uintptr_t)(0x0F) );
		offset += tinyOpsBuffSizes.partitionBuff2Size + MEM_ALIGNMENT_PAINT;

		oc_input2_tensors_buffers_0 = &tinyops_arena[offset];
		oc_input2_tensors_buffers_0 = (uint8_t *)( (uintptr_t)oc_input2_tensors_buffers_0 & ~(uintptr_t)(0x0F) );
		offset += tinyOpsBuffSizes.partitionFilterSize + MEM_ALIGNMENT_PAINT;

		oc_input2_tensors_buffers_1 = &tinyops_arena[offset];
		oc_input2_tensors_buffers_1 = (uint8_t *)( (uintptr_t)oc_input2_tensors_buffers_1 & ~(uintptr_t)(0x0F) );
		offset += tinyOpsBuffSizes.partitionIm2colSize + MEM_ALIGNMENT_PAINT;

		#ifdef OVERLAY_BIASES
		oc_input3_tensor_buffer = &tinyops_arena[offset];
		oc_input3_tensor_buffer = (uint8_t *)( (uintptr_t)oc_input3_tensor_buffer & ~(uintptr_t)(0x0F) );
		offset += tinyOpsBuffSizes.biasSize + MEM_ALIGNMENT_PAINT;
		#endif

		#ifdef OVERLAY_QUANT_PARAMS
		quant_out_multiplier = &tinyops_arena[offset];
		quant_out_multiplier = (uint8_t *)( (uintptr_t)quant_out_multiplier & ~(uintptr_t)(0x0F) );
		offset += tinyOpsBuffSizes.quantParamsOutMultiplierSize + MEM_ALIGNMENT_PAINT;

		quant_out_shift = &tinyops_arena[offset];
		quant_out_shift = (uint8_t *)( (uintptr_t)quant_out_shift & ~(uintptr_t)(0x0F) );
		offset += tinyOpsBuffSizes.quantParamsOutShiftSize + MEM_ALIGNMENT_PAINT;
		#endif

		oc_input_tensors[0].data.data	= (void *)oc_input_tensors_buffers[0];
		oc_input_tensors[1].data.data	= (void *)oc_input_tensors_buffers[1];

		oc_input2_tensors[0].data.data	= (void *)oc_input2_tensors_buffers_0;
		oc_input2_tensors[1].data.data	= (void *)oc_input2_tensors_buffers_1;

		#ifdef OVERLAY_BIASES
		oc_input3_tensor.data.data		= (void *)oc_input3_tensor_buffer;
		#endif
	}
	else
		return kTfLiteError;

	OVERLAY_GetMaxSizes( &context_, node_and_registrations_, &partitioned_exec_state );

	#endif

	// function to compute sizes needed

	return kTfLiteOk;
}

TfLiteStatus MicroInterpreter::Invoke() {

  uint16_t timer_val;
  int uart_buf_len;
  char uart_buf[50];

  if (initialization_status_ != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter_,
                         "Invoke() called after initialization failed\n");
    return kTfLiteError;
  }

  // Ensure tensors are allocated before the interpreter is invoked to avoid
  // difficult to debug segfaults.
  if (!tensors_allocated_) {
    TF_LITE_ENSURE_OK(&context_, AllocateTensors());
  }

//	uart_buf_len = sprintf(uart_buf, "operator->size %u \r\n", subgraph_->operators()->size());
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	#ifdef OVERLAY_FW
	partitioned_exec_state.currentOp = FIRST_OPERATION_STATE_VAL;
	partitioned_exec_state.currentPart = FIRST_PART_STATE_VAL;
	partitioned_exec_state.pingPong = 0;

  	volatile PartitionInfo * op_part_info = &partitioned_exec_state.ops_parts_info[FIRST_OPERATION_STATE_VAL];
	OVERLAY_CopyInData(&DMAQue[0], op_part_info, FIRST_PART_STATE_VAL, &oc_input_tensors[partitioned_exec_state.pingPong], &oc_input2_tensors[partitioned_exec_state.pingPong]);

	#ifdef OVERLAY_FILTERS
	const TfLiteEvalTensor * op_filter = op_part_info->filter;

	if ( op_filter ) {
		uint32_t op_filterTensorSize	=	op_filter->dims->data[0] *
											op_filter->dims->data[1] *
											op_filter->dims->data[2] *
											op_filter->dims->data[3];

		if ( op_filterTensorSize < INPUT2_TENSOR_BUFFER_0_SIZE )
			DMA_CopyTensor(&DMAQue[0], op_filter, &oc_input2_tensors[0]);
	}
	#endif

	#ifdef OVERLAY_BIASES
	const TfLiteEvalTensor * op_bias = op_part_info->bias;

	if ( op_bias ) {
		uint32_t op_biasTensorSize		=	op_bias->dims->data[0] * 4;
		if ( op_biasTensorSize < BIAS_TENSOR_BUFFER_SIZE )
			DMA_CopyTensor(&DMAQue[0], op_bias, &oc_input3_tensor);
	}
	#endif

//	if ( op_part_info->filter )
//		DMA_CopyTensor( &DMAQue[1], op_part_info->filter, &oc_input2_tensors[0] );
//	if ( op_part_info->bias )
//		DMA_CopyTensor( &DMAQue[1], op_part_info->bias,  &oc_input2_tensors[1] );
	#endif

	for (size_t i = 0; i < subgraph_->operators()->size(); ++i) {

//	uart_buf_len = sprintf(uart_buf, "operator %u \r\n", i);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	timer_val = __HAL_TIM_GET_COUNTER(&htim10);

    auto* node = &(node_and_registrations_[i].node);
    auto* registration = node_and_registrations_[i].registration;

// This ifdef is needed (even though ScopedMicroProfiler itself is a no-op with
// -DTF_LITE_STRIP_ERROR_STRINGS) because the function OpNameFromRegistration is
// only defined for builds with the error strings.
#if !defined(TF_LITE_STRIP_ERROR_STRINGS)
    ScopedMicroProfiler scoped_profiler(
        OpNameFromRegistration(registration),
        reinterpret_cast<MicroProfiler*>(context_.profiler));
#endif

	#ifdef OVERLAY_FW
    partitioned_exec_state.processed_dims[0] = 0;
    partitioned_exec_state.processed_dims[1] = 0;
    partitioned_exec_state.processed_dims[2] = 0;
    partitioned_exec_state.processed_dims[3] = 0;

  if (i == 120 )
	  uint32_t kk = 34;
    #endif

    TFLITE_DCHECK(registration->invoke);
    TfLiteStatus invoke_status = registration->invoke(&context_, node);

	#ifdef OVERLAY_FW
    partitioned_exec_state.currentOp++;
    partitioned_exec_state.currentOp = partitioned_exec_state.currentOp % partitioned_exec_state.totalOps;
	#endif
    // All TfLiteTensor structs used in the kernel are allocated from temp
    // memory in the allocator. This creates a chain of allocations in the
    // temp section. The call below resets the chain of allocations to
    // prepare for the next call.
    allocator_.ResetTempAllocations();

    if (invoke_status == kTfLiteError) {
    	uart_buf_len = sprintf(uart_buf, "kTfLiteError \r\n");
    	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

      TF_LITE_REPORT_ERROR(
          error_reporter_,
          "Node %s (number %d) failed to invoke with status %d",
          OpNameFromRegistration(registration), i, invoke_status);
      return kTfLiteError;
    } else if (invoke_status != kTfLiteOk) {
    	uart_buf_len = sprintf(uart_buf, "!= kTfLiteOk \r\n");
    	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

      return invoke_status;
    }

    timer_val = __HAL_TIM_GET_COUNTER(&htim10) - timer_val;

//    if ( registration->builtin_code == BuiltinOperator_PAD ) {
//		uart_buf_len = sprintf(uart_buf, "%u/10ms \r\n", timer_val);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//    }

  }

//	#ifdef OVERLAY_FW
//	uart_buf_len = sprintf(uart_buf, "debug_dump_dma_waiting_convp0_time: %X us \r\n", debug_dump_dma_waiting_convp0_time);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	#endif

	return kTfLiteOk;
}

TfLiteTensor* MicroInterpreter::input(size_t index) {
  const size_t length = inputs_size();
  if (index >= length) {
    TF_LITE_REPORT_ERROR(error_reporter_,
                         "Input index %d out of range (length is %d)", index,
                         length);
    return nullptr;
  }
  return input_tensors_[index];
}

TfLiteTensor* MicroInterpreter::output(size_t index) {
  const size_t length = outputs_size();
  if (index >= length) {
    TF_LITE_REPORT_ERROR(error_reporter_,
                         "Output index %d out of range (length is %d)", index,
                         length);
    return nullptr;
  }
  return output_tensors_[index];
}

TfLiteTensor* MicroInterpreter::tensor(size_t index) {
  const size_t length = tensors_size();
  if (index >= length) {
    TF_LITE_REPORT_ERROR(error_reporter_,
                         "Tensor index %d out of range (length is %d)", index,
                         length);
    return nullptr;
  }
  return allocator_.AllocatePersistentTfLiteTensor(model_, eval_tensors_,
                                                   index);
}

TfLiteStatus MicroInterpreter::ResetVariableTensors() {
  for (size_t i = 0; i < subgraph_->tensors()->size(); ++i) {
    auto* tensor = subgraph_->tensors()->Get(i);
    if (tensor->is_variable()) {
      size_t buffer_size;
      TF_LITE_ENSURE_STATUS(
          TfLiteEvalTensorByteLength(&eval_tensors_[i], &buffer_size));

      int value = 0;
      if (tensor->type() == tflite::TensorType_INT8) {
        value = tensor->quantization()->zero_point()->Get(0);
      }
      memset(eval_tensors_[i].data.raw, value, buffer_size);
    }
  }

  return kTfLiteOk;
}

}  // namespace tflite
