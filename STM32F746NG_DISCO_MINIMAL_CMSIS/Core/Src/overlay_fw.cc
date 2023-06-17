#include "overlay_fw.h"

#ifdef OVERLAY_FW
#include "dmacopying.h"
#include "tensorflow/lite/micro/micro_interpreter.h"

#include "main.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>

#include "tensorflow/lite/micro/kernels/conv.h"

#include "CMSIS/NN/Include/arm_nn_types.h"
#include "CMSIS/NN/Include/arm_nnfunctions.h"
#include "tensorflow/lite/c/builtin_op_data.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/common.h"
#include "tensorflow/lite/kernels/internal/quantization_util.h"
#include "tensorflow/lite/kernels/internal/reference/conv.h"
#include "tensorflow/lite/kernels/internal/reference/integer_ops/conv.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/kernels/padding.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"

#include "tensorflow/lite/micro/kernels/fully_connected.h"

#include "tensorflow/lite/kernels/internal/reference/fully_connected.h"
#include "tensorflow/lite/kernels/internal/reference/integer_ops/fully_connected.h"

//extern "C" {
//#include "dmacopying.h"
//#include "main.h"
//}

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef  htim10;

extern TfLiteEvalTensor oc_input_tensors[OP_MAX_INPUT_SIZE];
extern TfLiteEvalTensor oc_input2_tensors[OP_MAX_INPUT_SIZE];

extern volatile DMACopyQue DMAQue[NUM_DMA_STREAMS_USED];

#ifdef BENCHMARK_TINYOPS_STATS
extern	TinyOpsBuffSizes	tinyOpsBuffSizes;
extern	uint32_t			tinyOpsPartFactor;
#endif

namespace tflite {

uint8_t MicroInterpreter::PartitionOp( const TfLiteContext * context, NodeAndRegistration * nodes_and_regs, volatile PartitionedExecutionState * exec_state ) {

	for ( uint32_t partFac = 2; partFac < MAX_PARTITION_FACTOR; partFac++ ) {

		tinyOpsPartFactor = partFac;

		// reset the partitioning state before trying out the next partition factor
		for ( int n = 0; n < MAX_NUM_OPERATIONS; n++) {

			exec_state->ops_parts_info[n].dims = 0;
			exec_state->ops_parts_info[n].input1 = 0;
			exec_state->ops_parts_info[n].input2 = 0;
			exec_state->ops_parts_info[n].filter = 0;
			exec_state->ops_parts_info[n].bias = 0;

			for ( int m = 0; m < MAX_PARTITION_FACTOR; m++ ) {
				exec_state->ops_parts_info[n].partSizes[m] = 0;
				exec_state->ops_parts_info[n].partitionStart[m] = 0;
				exec_state->ops_parts_info[n].partitionEnd[m] = 0;

				exec_state->ops_parts_info[n].inp_partitioned_dim[m] = 0;
				exec_state->ops_parts_info[n].out_partitioned_dim[m] = 0;

				exec_state->ops_parts_info[n].op_part_data[m][0] = 0;
				exec_state->ops_parts_info[n].op_part_data[m][1] = 0;
			}
		}

		for ( uint32_t i = 0; i < exec_state->totalOps; i++ ) {

			auto * node = &(nodes_and_regs[i].node);
			auto * registration = (nodes_and_regs[i].registration);

			if ( registration->builtin_code == BuiltinOperator_CONV_2D ||
				 registration->builtin_code == BuiltinOperator_DEPTHWISE_CONV_2D )
				PartitionConvOp( context, node, registration, &exec_state->ops_parts_info[i], partFac );
			else if ( registration->builtin_code == BuiltinOperator_PAD )
				PartitionPadOp( context, node, registration, &exec_state->ops_parts_info[i], partFac );
			else if ( registration->builtin_code == BuiltinOperator_ADD )
				PartitionAddOp( context, node, registration, &exec_state->ops_parts_info[i], partFac );
			else if ( registration->builtin_code == BuiltinOperator_AVERAGE_POOL_2D )
				PartitionPoolOp( context, node, registration, &exec_state->ops_parts_info[i], partFac );
			else if ( registration->builtin_code == BuiltinOperator_MUL )
				PartitionMulOp( context, node, registration, &exec_state->ops_parts_info[i],  partFac );
			else if ( registration->builtin_code == BuiltinOperator_HARD_SWISH )
				PartitionHardSwishOp(context, node, registration, &exec_state->ops_parts_info[i], partFac );

		}

		#define DYNAMIC_PARTITION_FACTOR
		#ifdef DYNAMIC_PARTITION_FACTOR
		uint32_t NW_maxPartitionSize = Partition_GetMaxSize( context, nodes_and_regs, exec_state );	// get max buffer size

		OVERLAY_GetMaxSizes( context, nodes_and_regs, exec_state );

		for ( uint32_t i = 0; i < exec_state->totalOps; i++ ) {		// now go find smallest partition factor for ops, to maximise buffer spaceusage

			auto * node = &(nodes_and_regs[i].node);
			auto * registration = (nodes_and_regs[i].registration);

			const TfLiteEvalTensor * input1	= 0;

			if ( exec_state->ops_parts_info[i].numParts == 2 )				// partition factor for this op already small enough
				continue;

			for ( uint32_t j = 2; j < (partFac+1); j++ ) {

				if ( registration->builtin_code == BuiltinOperator_CONV_2D ||
					 registration->builtin_code == BuiltinOperator_DEPTHWISE_CONV_2D ) {
					PartitionConvOp( context, node, registration, &exec_state->ops_parts_info[i], j );
					input1	= tflite::micro::GetEvalInput(context, node, kConvInputTensor);
				}
				else if ( registration->builtin_code == BuiltinOperator_PAD ) {
					PartitionPadOp( context, node, registration, &exec_state->ops_parts_info[i], j );
					input1 = tflite::micro::GetEvalInput(context, node, 0);
				}
				else if ( registration->builtin_code == BuiltinOperator_ADD ) {
					PartitionAddOp( context, node, registration, &exec_state->ops_parts_info[i], j );
					input1 = tflite::micro::GetEvalInput(context, node, 0);
				}
				else if ( registration->builtin_code == BuiltinOperator_AVERAGE_POOL_2D ) {
					PartitionPoolOp( context, node, registration, &exec_state->ops_parts_info[i], j );
					input1 = tflite::micro::GetEvalInput(context, node, 0);
				}
				else if ( registration->builtin_code == BuiltinOperator_MUL ) {
					PartitionMulOp( context, node, registration, &exec_state->ops_parts_info[i],  j );
					input1 = tflite::micro::GetEvalInput(context, node, 0);
				}
				else if ( registration->builtin_code == BuiltinOperator_HARD_SWISH ) {
					PartitionHardSwishOp(context, node, registration, &exec_state->ops_parts_info[i], j );
					input1 = tflite::micro::GetEvalInput(context, node, 0);
				}

				uint32_t partitionSize, maxPartitionSizeOp, maxPartitionSizeOpPart;
				uint32_t maxPartitionSize = 0;

				for ( int j = 0; j < exec_state->ops_parts_info[i].numParts; j++ ) {
					partitionSize = 1;
					for ( int k = 0; k < input1->dims->size; k++ ) {
						if ( k != DIM_IDX_HEIGHT )
							partitionSize = partitionSize * input1->dims->data[k];
					}
					partitionSize = partitionSize * (exec_state->ops_parts_info[i].partitionEnd[j] - exec_state->ops_parts_info[i].partitionStart[j]);
					if ( partitionSize > maxPartitionSize ) {
						maxPartitionSize			= partitionSize;
						maxPartitionSizeOp			= i;
						maxPartitionSizeOpPart		= j;
					}
				}

				if ( maxPartitionSize <= NW_maxPartitionSize )
					break;

			}
		}
		#endif

		int32_t tinyops_sram_usage = MEM_ALIGNMENT_PAINT;

		tinyops_sram_usage += tinyOpsBuffSizes.partitionBuff1Size + MEM_ALIGNMENT_PAINT;
		tinyops_sram_usage += tinyOpsBuffSizes.partitionBuff2Size + MEM_ALIGNMENT_PAINT;
		tinyops_sram_usage += tinyOpsBuffSizes.partitionFilterSize + MEM_ALIGNMENT_PAINT;
		tinyops_sram_usage += tinyOpsBuffSizes.partitionIm2colSize + MEM_ALIGNMENT_PAINT;
		tinyops_sram_usage += tinyOpsBuffSizes.biasSize + MEM_ALIGNMENT_PAINT;
		tinyops_sram_usage += tinyOpsBuffSizes.quantParamsOutMultiplierSize + MEM_ALIGNMENT_PAINT;
		tinyops_sram_usage += tinyOpsBuffSizes.quantParamsOutShiftSize + MEM_ALIGNMENT_PAINT;

//		if (tinyops_sram_usage < PLATFORM_AVAILABLE_SRAM) {
//			tinyOpsBuffSizes.partitionFilterSize += (PLATFORM_AVAILABLE_SRAM - tinyops_sram_usage);
//			tinyops_sram_usage += (PLATFORM_AVAILABLE_SRAM - tinyops_sram_usage);
//		}

//		tinyOpsBuffSizes.partitionFilterSize = PLATFORM_AVAILABLE_SRAM - 20000 - tinyops_sram_usage - MEM_ALIGNMENT_PAINT;
//		tinyops_sram_usage += tinyOpsBuffSizes.partitionFilterSize;

		if (tinyops_sram_usage <= PLATFORM_AVAILABLE_SRAM)	// subtract 1000 for 32byte alignment of memory pages
			return 1;

	}

	return 0;

}

uint32_t MicroInterpreter::Partition_GetMaxSize( const TfLiteContext * context, NodeAndRegistration * nodes_and_regs, volatile PartitionedExecutionState * exec_state ) {

	int uart_buf_len;
	char uart_buf[50];

	uint32_t maxPartitionSize = 0;

	uint32_t maxPartitionSizeOp;
	uint32_t maxPartitionSizeOpPart;

	for ( int i = 0; i < exec_state->totalOps; i++ ) {

		auto * node = &(nodes_and_regs[i].node);
		auto * reg = (nodes_and_regs[i].registration);

		volatile PartitionInfo * op_part_info = &exec_state->ops_parts_info[i];

		if ( op_part_info->numParts != 0 ) {

			const TfLiteEvalTensor * input1	= 0;
			uint32_t partitionSize			= 1;

			if ( reg->builtin_code == BuiltinOperator_CONV_2D )
				input1	= tflite::micro::GetEvalInput(context, node, kConvInputTensor);
			else if ( reg->builtin_code == BuiltinOperator_DEPTHWISE_CONV_2D )
				input1	= tflite::micro::GetEvalInput(context, node, kConvInputTensor);
			else if ( reg->builtin_code == BuiltinOperator_AVERAGE_POOL_2D )
				input1 = tflite::micro::GetEvalInput(context, node, 0);
			else if ( reg->builtin_code == BuiltinOperator_PAD )
				input1 = tflite::micro::GetEvalInput(context, node, 0);
			else if ( reg->builtin_code == BuiltinOperator_ADD )
				input1 = tflite::micro::GetEvalInput(context, node, 0);
			else if ( reg->builtin_code == BuiltinOperator_MUL )
				input1 = tflite::micro::GetEvalInput(context, node, 0);
			else if ( reg->builtin_code == BuiltinOperator_HARD_SWISH )
				input1 = tflite::micro::GetEvalInput(context, node, 0);

			for ( int j = 0; j < op_part_info->numParts; j++ ) {
				partitionSize = 1;
				for ( int k = 0; k < input1->dims->size; k++ ) {
					if ( k != DIM_IDX_HEIGHT )
						partitionSize = partitionSize * input1->dims->data[k];
				}
				partitionSize = partitionSize * (op_part_info->partitionEnd[j] - op_part_info->partitionStart[j]);
				if ( partitionSize > maxPartitionSize ) {
					maxPartitionSize			= partitionSize;
					maxPartitionSizeOp			= i;
					maxPartitionSizeOpPart		= j;
				}
			}
		}
	}

//	uart_buf_len = sprintf(uart_buf, "Max Partition Sizes:\n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
//	uart_buf_len = sprintf(uart_buf, "%u, %u: %u\n", maxPartitionSizeOp, maxPartitionSizeOpPart, maxPartitionSize);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	return maxPartitionSize;

}

void MicroInterpreter::PartitionConvOp( const TfLiteContext * context, TfLiteNode * node, const TfLiteRegistration * registration, volatile PartitionInfo * op_partition_info, uint32_t partition_factor ) {

	int uart_buf_len;
	char uart_buf[50];

	struct OpData {
	  OpDataConv reference_op_data;

	  // Index to buffer for optimizations if applicable.
	  int buffer_idx;
	};

	const TfLiteEvalTensor* input	=	tflite::micro::GetEvalInput(context, node, kConvInputTensor);
	const TfLiteEvalTensor* filter	=	tflite::micro::GetEvalInput(context, node, kConvWeightsTensor);
	const TfLiteEvalTensor* bias	=	tflite::micro::GetEvalInput(context, node, kConvBiasTensor);
	//(NumInputs(node) == 3) ? tflite::micro::GetEvalInput(context, node, kDepthwiseConvBiasTensor) : nullptr;

	const TfLiteEvalTensor * output	=	tflite::micro::GetEvalOutput(context, node, kConvOutputTensor);

	op_partition_info->input1		= input;
	op_partition_info->filter		= filter;
	op_partition_info->bias			= bias;

	int stride_h;

	TFLITE_DCHECK(node->builtin_data != nullptr);
	if ( registration->builtin_code == BuiltinOperator_CONV_2D ) {
		const auto& params = *(reinterpret_cast<TfLiteConvParams*>(node->builtin_data));
		stride_h = params.stride_height;
	}
	if ( registration->builtin_code == BuiltinOperator_DEPTHWISE_CONV_2D ) {
		const auto& params = *(reinterpret_cast<TfLiteDepthwiseConvParams*>(node->builtin_data));
		stride_h = params.stride_height;
	}

	TFLITE_DCHECK(node->user_data != nullptr);
	OpData& data = *(static_cast<OpData*>(node->user_data));

	// get conv parameters needed to calculate partitioning
	int padding_h = data.reference_op_data.padding.height;

	int input_height	= input->dims->data[DIM_IDX_HEIGHT];
	int filter_height	= filter->dims->data[DIM_IDX_HEIGHT];
	int output_height	= output->dims->data[DIM_IDX_HEIGHT];

	// reset variables holding partitioning state
	op_partition_info->dims = input->dims;

	for (uint32_t j = 0; j < partition_factor; j++) {

		op_partition_info->partSizes[j] = 0;
		op_partition_info->inp_partitioned_dim[j] = 0;
		op_partition_info->out_partitioned_dim[j] = 0;

	}
	op_partition_info->numParts = 0;

	// calculate size of partitions according to scheme where every part is multiple of stride and any leftover rows go to last partition
	int k = 0, partsHeightsSum = 0;

	op_partition_info->partSizes[0] += (padding_h % stride_h);				// padding included in size of 1st partition?
	partsHeightsSum += (padding_h % stride_h);

	if (filter_height >= input_height) {

		op_partition_info->partSizes[0] = input_height;
		op_partition_info->numParts = 1;

	}
	else {

		while ( (input_height - partsHeightsSum) >= stride_h ) {				// while >= #stride rows still to be assigned to partitions

			op_partition_info->partSizes[k] += stride_h;						// add # stride rows to partitions one by one
			partsHeightsSum += stride_h;

			k++;																// move to add rows to next partition
			if ( op_partition_info->numParts != partition_factor )				// increment till we reach max number of partitions possible
				op_partition_info->numParts = k;
			if ( k == partition_factor )										// increment with roll-over
				k = 0;

		}

	op_partition_info->partSizes[op_partition_info->numParts-1] += (input_height % partsHeightsSum);	// add leftover rows to last partition

	}

//	uart_buf_len = sprintf(uart_buf, "partSizes done\n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	// get size of tensors in partitions including extra rows due to receptive field overflow

	uint32_t startRow = 0, endRow;
	uint8_t i = 0;

	uint32_t outputRowsProduced = 0;

	for ( i = 0; i < op_partition_info->numParts; i++ ) {

//		uart_buf_len = sprintf(uart_buf, "partNum: %u\n", i);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

		// calculate tensor dimensions of input tensor
		uint32_t padRows = 0;

		uint32_t part_size				= op_partition_info->partSizes[i];

		uint32_t numRecFieldStarts		= part_size/stride_h + 1;				// deals with odd numbered partition sizes

		uint32_t numOutRowsLeft			= output_height - outputRowsProduced;

		if ( numOutRowsLeft == 0 ) {											// if all rows produced

			op_partition_info->numParts = i;									// then this part is useless. discard this and later parts and set previous as last part
			break;																//
		}

		uint32_t lastRecFieldStartRow	= part_size - (part_size % stride_h) - ( (part_size % stride_h) == 0 ) * stride_h + startRow;
		uint32_t lastRecFieldEndRow		= lastRecFieldStartRow + filter_height;

		int32_t overflowRows			= lastRecFieldEndRow - input_height;

		if ( overflowRows > 0 ) {												// if receptive field flows over image

//			uart_buf_len = sprintf(uart_buf, "overflowed: %u\n", overflowRows);
//			HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

			endRow	= input_height;												// than clamp ending row to end of image

			if ( numOutRowsLeft <= numRecFieldStarts ){							// if all remaining rows are calculated from this partition

				uint32_t lastValidRecFieldStartRow	= (numOutRowsLeft - 1) * stride_h + startRow;
				uint32_t lastValidRecFieldEndRow	= lastValidRecFieldStartRow + filter_height;

				padRows = lastValidRecFieldEndRow - input_height;				// pad according to how many receptive fields needed for remaining out rows
			}
			else {																// else if not all remaining rows are calculated from this partition

				padRows = lastRecFieldEndRow - input_height;					// then padd so that all receptive fields are used
			}


		}
		else
			endRow = lastRecFieldEndRow;

//		uart_buf_len = sprintf(uart_buf, "post overflow\n");
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

		op_partition_info->inp_partitioned_dim[i] = endRow - startRow;			// rest of dimensions are unpartitioned and same as input, previously assigned at start

		op_partition_info->partitionStart[i]	= startRow;						// to help with size of data copy computation
		op_partition_info->partitionEnd[i]		= endRow;

		if (filter_height >= input_height) {
			op_partition_info->out_partitioned_dim[0]					 = (uint32_t)output_height;
			op_partition_info->op_part_data[0][OP_DATA_CONV_PAD_IDX]	=  (uint32_t)padding_h;

		}
		else {

			if ( i == 0 ) {
				padRows	+= padding_h;
				op_partition_info->op_part_data[i][OP_DATA_CONV_PAD_IDX]	=  (uint32_t)padding_h;
			}

			int out_size = ( op_partition_info->inp_partitioned_dim[i] + padRows - filter_height)/stride_h + 1;
			op_partition_info->out_partitioned_dim[i] = (uint32_t)out_size;

			outputRowsProduced += out_size;

	//			if ( (i+1) < op_partition_info->numParts )
			startRow = startRow + op_partition_info->partSizes[i];

		}

	}

//	uart_buf_len = sprintf(uart_buf, "num_parts: %u\n", op_partition_info->numParts);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

//		return 1;

}

void MicroInterpreter::PartitionPoolOp( const TfLiteContext * context, TfLiteNode * node, const TfLiteRegistration * registration, volatile PartitionInfo * op_partition_info, uint32_t partition_factor ) {

	int uart_buf_len;
	char uart_buf[50];

	struct OpData {
	  TfLitePaddingValues padding;
	  // Index to buffer for optimizations if applicable.
	  int buffer_idx;

	  int32_t activation_min;
	  int32_t activation_max;
	  float activation_min_f32;
	  float activation_max_f32;
	};

	const TfLiteEvalTensor* input	=	tflite::micro::GetEvalInput(context, node, 0);

	op_partition_info->input1		= input;

	TFLITE_DCHECK(node->builtin_data != nullptr);
	auto* params = reinterpret_cast<TfLitePoolParams*>(node->builtin_data);

	TFLITE_DCHECK(node->user_data != nullptr);
	const OpData& data = *(static_cast<const OpData*>(node->user_data));

	// get avg pooling parameters needed to calculate partitioning
	int stride_h		=	params->stride_height;
	int padding_h		=	data.padding.height;
	int filter_height	= params->filter_height;

	int input_height	= input->dims->data[DIM_IDX_HEIGHT];

	// reset variables holding partitioning state
	op_partition_info->dims = input->dims;

	for (uint32_t j = 0; j < partition_factor; j++) {

		op_partition_info->partSizes[j] = 0;
		op_partition_info->inp_partitioned_dim[j] = 0;
		op_partition_info->out_partitioned_dim[j] = 0;

	}
	op_partition_info->numParts = 0;

	// calculate size of partitions according to scheme where every part is multiple of stride and any leftover rows go to last partition
	int k = 0, partsHeightsSum = 0;

	while ( (input_height - partsHeightsSum) >= stride_h ) {							// while >= #stride rows still to be assigned to partitions

		op_partition_info->partSizes[k] += stride_h;				// add # stride rows to partitions one by one
		partsHeightsSum += stride_h;

		k++;																			// move to add rows to next partition
		if ( op_partition_info->numParts != partition_factor )							// increment till we reach max number of partitions possible
			op_partition_info->numParts = k;
		if ( k == partition_factor )														// increment with roll-over
			k = 0;

	}

	op_partition_info->partSizes[op_partition_info->numParts-1] += (input_height % partsHeightsSum);	// add leftover rows to last partition

//	uart_buf_len = sprintf(uart_buf, "partSizes done\n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	// get size of tensors in partitions including extra rows due to receptive field overflow

	uint32_t startRow = 0, endRow;
	uint8_t i = 0;

	for ( i = 0; i < op_partition_info->numParts; i++ ) {

//		uart_buf_len = sprintf(uart_buf, "partNum: %u\n", i);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

		// calculate tensor dimensions of input tensor
		uint32_t padRows = 0;

		uint32_t part_size				= op_partition_info->partSizes[i];

		uint32_t lastRecFieldStartRow	= part_size - (part_size % stride_h) - ( (part_size % stride_h) == 0 ) * stride_h + startRow;
		uint32_t lastRecFieldEndRow		= lastRecFieldStartRow + filter_height;
		int32_t overflowRows			= lastRecFieldEndRow - input_height;

		if ( overflowRows > 0 ) {												// if receptive field flows over image

//			uart_buf_len = sprintf(uart_buf, "overflowed: %u\n", overflowRows);
//			HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

			endRow	= input_height;												// than clamp ending row to end of image
			padRows	+= (overflowRows < padding_h) ? overflowRows : padding_h;	// add padding according to how many rows overflowed

			int32_t firstRecFieldLowerPadReq = startRow + filter_height - input_height;	// calculate padding required for first receptive field
			if ( firstRecFieldLowerPadReq > padding_h ) {							// if req more than padding_h, kernel cannot be centered in this partition
//				uart_buf_len = sprintf(uart_buf, "firstRecFieldLowerPadReq > padding_h\n");
//				HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

				op_partition_info->numParts = i;								// then this part is useless. discard this and later parts and set previous as last part
				break;															//
			}

			if ( (padding_h == 0) && ((endRow-startRow) < filter_height) ) {		// if no padding and no complete receptive fields in this part
//				uart_buf_len = sprintf(uart_buf, "padding_h == 0 && (endRow-startRow) < filter_height\n");
//				HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
//				uart_buf_len = sprintf(uart_buf, "endRow: %u\n", endRow);
//				HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
//				uart_buf_len = sprintf(uart_buf, "startRow: %u\n", startRow);
//				HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
//				uart_buf_len = sprintf(uart_buf, "filter_height: %u\n", filter_height);
//				HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

				op_partition_info->numParts = i;								// then this part is useless. discard this and later parts and set previous as last part
				break;															// impossible for first part to be useless
			}
		}
		else
			endRow = lastRecFieldEndRow;

//		uart_buf_len = sprintf(uart_buf, "post overflow\n");
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

		op_partition_info->inp_partitioned_dim[i] = endRow - startRow;		// rest of dimensions are unpartitioned and same as input, previously assigned at start

		op_partition_info->partitionStart[i]	= startRow;								// to help with size of data copy computation
		op_partition_info->partitionEnd[i]		= endRow;

		if ( i == 0 ) {
			padRows	+= padding_h;
			op_partition_info->op_part_data[i][OP_DATA_POOL_PAD_IDX]	=  (uint32_t)padding_h;
		}

		int out_size = ( op_partition_info->inp_partitioned_dim[i] + padRows - filter_height)/stride_h + 1;
		op_partition_info->out_partitioned_dim[i] = (uint32_t)out_size;

//			if ( (i+1) < op_partition_info->numParts )
		startRow = startRow + op_partition_info->partSizes[i];

	}

//	uart_buf_len = sprintf(uart_buf, "num_parts: %u\n", op_partition_info->numParts);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

//		return 1;

}

void MicroInterpreter::PartitionPadOp( const TfLiteContext * context, TfLiteNode * node, const TfLiteRegistration * registration, volatile PartitionInfo * op_partition_info, uint32_t partition_factor ) {

	int uart_buf_len;
	char uart_buf[50];

	struct OpData {
	  PadParams params;
	  int32_t output_zero_point;
	};

	TFLITE_DCHECK(node->user_data != nullptr);
	const OpData* data = static_cast<const OpData*>(node->user_data);

	const TfLiteEvalTensor* input			=	tflite::micro::GetEvalInput(context, node, /*index=*/0);
	const TfLiteEvalTensor* constant_values	=	NumInputs(node) == 3 ? tflite::micro::GetEvalInput(context, node, /*index=*/2) : nullptr;
	TfLiteEvalTensor* output				=	tflite::micro::GetEvalOutput(context, node, /*index=*/0);

	op_partition_info->input1		= input;

	int input_height		= input->dims->data[DIM_IDX_HEIGHT];
    int32_t h_top_padding	= data->params.left_padding[DIM_IDX_HEIGHT];
    int32_t h_bot_padding	= data->params.right_padding[DIM_IDX_HEIGHT];

	// reset variables holding partitioning state
	op_partition_info->dims = input->dims;

	for (uint32_t j = 0; j < partition_factor; j++) {

		op_partition_info->partSizes[j] = 0;
		op_partition_info->inp_partitioned_dim[j] = 0;
		op_partition_info->out_partitioned_dim[j] = 0;

	}
	op_partition_info->numParts = 0;

	// calculate size of partitions by incrementing each partition one at a time till all rows consumed
	int k = 0, partsHeightsSum = 0;

	while ( (input_height - partsHeightsSum) > 0 ) {				// while >0 rows still to be assigned to partitions

		op_partition_info->partSizes[k] += 1;				// add # stride rows to partitions one by one
		partsHeightsSum += 1;

		k++;																			// move to add rows to next partition
		if ( op_partition_info->numParts != partition_factor )							// increment till we reach max number of partitions possible
			op_partition_info->numParts = k;
		if ( k == partition_factor )														// increment with roll-over
			k = 0;

	}

	uint32_t startRow = 0, endRow;
	uint8_t i = 0;

	for ( i = 0; i < op_partition_info->numParts; i++ ) {

//		uart_buf_len = sprintf(uart_buf, "partNum: %u\n", i);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
		uint32_t padRows = 0;

		// calculate tensor dimensions of input tensor
		uint32_t part_size				= op_partition_info->partSizes[i];

		endRow = startRow + part_size;

		op_partition_info->inp_partitioned_dim[i] = endRow - startRow;		// rest of dimensions are unpartitioned and same as input, previously assigned at start

		op_partition_info->partitionStart[i]	= startRow;								// to help with size of data copy computation
		op_partition_info->partitionEnd[i]		= endRow;

		op_partition_info->op_part_data[i][OP_DATA_PAD_H_TOP_PAD_IDX]	= 0;
		op_partition_info->op_part_data[i][OP_DATA_PAD_H_BOT_PAD_IDX]	= 0;

		if ( i == 0 ) {
			op_partition_info->op_part_data[i][OP_DATA_PAD_H_TOP_PAD_IDX]	= (uint32_t)h_top_padding;
			padRows += h_top_padding;
		}

		if ( i == (op_partition_info->numParts-1) ) {
			op_partition_info->op_part_data[i][OP_DATA_PAD_H_BOT_PAD_IDX]	= (uint32_t)h_bot_padding;
			padRows += h_bot_padding;
		}

		int out_size = op_partition_info->inp_partitioned_dim[i] + padRows;
		op_partition_info->out_partitioned_dim[i] = (uint32_t)out_size;

//			if ( (i+1) < op_partition_info->numParts )
		startRow = startRow + op_partition_info->partSizes[i];

	}

//	uart_buf_len = sprintf(uart_buf, "num_parts: %u\n", op_partition_info->numParts);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

}

void MicroInterpreter::PartitionAddOp( const TfLiteContext * context, TfLiteNode * node, const TfLiteRegistration * registration, volatile PartitionInfo * op_partition_info, uint32_t partition_factor ) {

	int uart_buf_len;
	char uart_buf[50];

	const TfLiteEvalTensor* input1	=	tflite::micro::GetEvalInput(context, node, 0);
	const TfLiteEvalTensor* input2	=	tflite::micro::GetEvalInput(context, node, 1);

	op_partition_info->input1		= input1;
	op_partition_info->input2		= input2;

	int input_height		= input1->dims->data[DIM_IDX_HEIGHT];

	// reset variables holding partitioning state
	op_partition_info->dims = input1->dims;

	for (uint32_t j = 0; j < partition_factor; j++) {

		op_partition_info->partSizes[j]				= 0;
		op_partition_info->inp_partitioned_dim[j]	= 0;
		op_partition_info->out_partitioned_dim[j]	= 0;
		op_partition_info->partitionStart[j]		= 0;								// to help with size of data copy computation
		op_partition_info->partitionEnd[j]			= 0;

	}
	op_partition_info->numParts = 0;

	// calculate size of partitions by incrementing each partition one at a time till all rows consumed
	int k = 0, partsHeightsSum = 0;

	while ( (input_height - partsHeightsSum) > 0 ) {				// while >0 rows still to be assigned to partitions

		op_partition_info->partSizes[k] += 1;				// add # stride rows to partitions one by one
		partsHeightsSum += 1;

		k++;																			// move to add rows to next partition
		if ( op_partition_info->numParts != partition_factor )							// increment till we reach max number of partitions possible
			op_partition_info->numParts = k;
		if ( k == partition_factor )														// increment with roll-over
			k = 0;

	}

	uint32_t startRow = 0, endRow;
	uint8_t i = 0;

	for ( i = 0; i < op_partition_info->numParts; i++ ) {

//		uart_buf_len = sprintf(uart_buf, "partNum: %u\n", i);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
		// calculate tensor dimensions of input tensor
		uint32_t part_size				= op_partition_info->partSizes[i];
		endRow = startRow + part_size;

		op_partition_info->inp_partitioned_dim[i] = endRow - startRow;		// rest of dimensions are unpartitioned and same as input, previously assigned at start

		op_partition_info->partitionStart[i]	= startRow;								// to help with size of data copy computation
		op_partition_info->partitionEnd[i]		= endRow;

		int out_size = op_partition_info->inp_partitioned_dim[i];
		op_partition_info->out_partitioned_dim[i] = (uint32_t)out_size;

		startRow = startRow + op_partition_info->partSizes[i];

	}

//	uart_buf_len = sprintf(uart_buf, "num_parts: %u\n", op_partition_info->numParts);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

}

void MicroInterpreter::PartitionMulOp( const TfLiteContext * context, TfLiteNode * node, const TfLiteRegistration * registration, volatile PartitionInfo * op_partition_info, uint32_t partition_factor ) {

	int uart_buf_len;
	char uart_buf[50];

	const TfLiteEvalTensor* input1	=	tflite::micro::GetEvalInput(context, node, 0);
	const TfLiteEvalTensor* input2	=	tflite::micro::GetEvalInput(context, node, 1);

	op_partition_info->input1		= input1;
	op_partition_info->input2		= input2;

	int input_height		= input1->dims->data[DIM_IDX_HEIGHT];

	// reset variables holding partitioning state
	op_partition_info->dims = input1->dims;

	for (uint32_t j = 0; j < partition_factor; j++) {

		op_partition_info->partSizes[j]				= 0;
		op_partition_info->inp_partitioned_dim[j]	= 0;
		op_partition_info->out_partitioned_dim[j]	= 0;
		op_partition_info->partitionStart[j]		= 0;								// to help with size of data copy computation
		op_partition_info->partitionEnd[j]			= 0;

	}
	op_partition_info->numParts = 0;

	// calculate size of partitions by incrementing each partition one at a time till all rows consumed
	int k = 0, partsHeightsSum = 0;

	while ( (input_height - partsHeightsSum) > 0 ) {				// while >0 rows still to be assigned to partitions

		op_partition_info->partSizes[k] += 1;				// add # stride rows to partitions one by one
		partsHeightsSum += 1;

		k++;																			// move to add rows to next partition
		if ( op_partition_info->numParts != partition_factor )							// increment till we reach max number of partitions possible
			op_partition_info->numParts = k;
		if ( k == partition_factor )														// increment with roll-over
			k = 0;

	}

	uint32_t startRow = 0, endRow;
	uint8_t i = 0;

	for ( i = 0; i < op_partition_info->numParts; i++ ) {

//		uart_buf_len = sprintf((char *)debugPrintPtr, "partNum: %u\n", i);
//		HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
//		debugPrintPtr += 32;

		// calculate tensor dimensions of input tensor
		uint32_t part_size				= op_partition_info->partSizes[i];
		endRow = startRow + part_size;

		op_partition_info->inp_partitioned_dim[i] = endRow - startRow;		// rest of dimensions are unpartitioned and same as input, previously assigned at start

		op_partition_info->partitionStart[i]	= startRow;								// to help with size of data copy computation
		op_partition_info->partitionEnd[i]		= endRow;

		int out_size = op_partition_info->inp_partitioned_dim[i];
		op_partition_info->out_partitioned_dim[i] = (uint32_t)out_size;

		startRow = startRow + op_partition_info->partSizes[i];

	}

//	uart_buf_len = sprintf((char *)debugPrintPtr, "num_parts: %u\n", op_partition_info->numParts);
//	HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
//	debugPrintPtr += 32;

}

void MicroInterpreter::PartitionHardSwishOp( const TfLiteContext * context, TfLiteNode * node, const TfLiteRegistration * registration, volatile PartitionInfo * op_partition_info, uint32_t partition_factor ) {

	int uart_buf_len;
	char uart_buf[50];

	const TfLiteEvalTensor* input1	=	tflite::micro::GetEvalInput(context, node, 0);

	op_partition_info->input1		= input1;

	int input_height		= input1->dims->data[DIM_IDX_HEIGHT];

	// reset variables holding partitioning state
	op_partition_info->dims = input1->dims;

	for (uint32_t j = 0; j < partition_factor; j++) {

		op_partition_info->partSizes[j]				= 0;
		op_partition_info->inp_partitioned_dim[j]	= 0;
		op_partition_info->out_partitioned_dim[j]	= 0;
		op_partition_info->partitionStart[j]		= 0;								// to help with size of data copy computation
		op_partition_info->partitionEnd[j]			= 0;

	}
	op_partition_info->numParts = 0;

	// calculate size of partitions by incrementing each partition one at a time till all rows consumed
	int k = 0, partsHeightsSum = 0;

	while ( (input_height - partsHeightsSum) > 0 ) {				// while >0 rows still to be assigned to partitions

		op_partition_info->partSizes[k] += 1;				// add # stride rows to partitions one by one
		partsHeightsSum += 1;

		k++;																			// move to add rows to next partition
		if ( op_partition_info->numParts != partition_factor )							// increment till we reach max number of partitions possible
			op_partition_info->numParts = k;
		if ( k == partition_factor )														// increment with roll-over
			k = 0;

	}

	uint32_t startRow = 0, endRow;
	uint8_t i = 0;

	for ( i = 0; i < op_partition_info->numParts; i++ ) {

//		uart_buf_len = sprintf((char *)debugPrintPtr, "partNum: %u\n", i);
//		HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
//		debugPrintPtr += 32;

		// calculate tensor dimensions of input tensor
		uint32_t part_size				= op_partition_info->partSizes[i];
		endRow = startRow + part_size;

		op_partition_info->inp_partitioned_dim[i] = endRow - startRow;		// rest of dimensions are unpartitioned and same as input, previously assigned at start

		op_partition_info->partitionStart[i]	= startRow;								// to help with size of data copy computation
		op_partition_info->partitionEnd[i]		= endRow;

		int out_size = op_partition_info->inp_partitioned_dim[i];
		op_partition_info->out_partitioned_dim[i] = (uint32_t)out_size;

		startRow = startRow + op_partition_info->partSizes[i];

	}

//	uart_buf_len = sprintf((char *)debugPrintPtr, "num_parts: %u\n", op_partition_info->numParts);
//	HAL_UART_Transmit(&huart3, (uint8_t *)(char *)debugPrintPtr, uart_buf_len, 100);
//	debugPrintPtr += 32;

}

void MicroInterpreter::OVERLAY_GetMaxSizes( const TfLiteContext * context, NodeAndRegistration * nodes_and_regs, volatile PartitionedExecutionState * exec_state ) {

	int uart_buf_len;
	char uart_buf[50];

	uint32_t maxPartitionSize[MAX_RECEPTIVE_FIELD_LIST_SIZE]			= {0, 0, 0, 0, 0};
	uint32_t maxFilterSize[MAX_RECEPTIVE_FIELD_LIST_SIZE]				= {0, 0, 0, 0, 0};
	uint32_t maxReceptiveFieldSize[MAX_RECEPTIVE_FIELD_LIST_SIZE]		= {0, 0, 0, 0, 0};
	uint32_t maxBiasSize[MAX_RECEPTIVE_FIELD_LIST_SIZE]					= {0, 0, 0, 0, 0};

	uint32_t maxPartitionSizeOp[MAX_RECEPTIVE_FIELD_LIST_SIZE]			= {0, 0, 0, 0, 0};
	uint32_t maxPartitionSizeOpPart[MAX_RECEPTIVE_FIELD_LIST_SIZE]		= {0, 0, 0, 0, 0};
	uint32_t maxFilterSizeOps[MAX_RECEPTIVE_FIELD_LIST_SIZE]			= {0, 0, 0, 0, 0};
	uint32_t maxReceptiveFieldSizeOps[MAX_RECEPTIVE_FIELD_LIST_SIZE]	= {0, 0, 0, 0, 0};
	uint32_t maxBiasSizeOps[MAX_RECEPTIVE_FIELD_LIST_SIZE]				= {0, 0, 0, 0, 0};

//	uart_buf_len = sprintf(uart_buf, "Input Sizes:\n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

//	uart_buf_len = sprintf(uart_buf, "Max Part Sizes:\n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	for ( int i = 0; i < exec_state->totalOps; i++ ) {

		auto * node = &(nodes_and_regs[i].node);
		auto * reg = (nodes_and_regs[i].registration);

		volatile PartitionInfo * op_part_info = &exec_state->ops_parts_info[i];

		if ( op_part_info->numParts != 0 ) {
			const TfLiteEvalTensor * input1	= 0;
			const TfLiteEvalTensor * input2	= 0;
			const TfLiteEvalTensor * filter	= 0;
			const TfLiteEvalTensor * bias	= 0;

			uint32_t partitionSize	= 1;
			uint32_t filterSize		= 0;
			uint32_t biasSize		= 0;

			uint32_t rc_multiplier;

			if ( reg->builtin_code == BuiltinOperator_CONV_2D ) {
				input1	= tflite::micro::GetEvalInput(context, node, kConvInputTensor);
				filter	= tflite::micro::GetEvalInput(context, node, kConvWeightsTensor);
				bias	= (NumInputs(node) == 3) ? tflite::micro::GetEvalInput(context, node, kConvBiasTensor) : 0;
				rc_multiplier = 4;
			}
			else if ( reg->builtin_code == BuiltinOperator_DEPTHWISE_CONV_2D ) {
				input1	= tflite::micro::GetEvalInput(context, node, kConvInputTensor);
				filter	= tflite::micro::GetEvalInput(context, node, kConvWeightsTensor);
				bias	= (NumInputs(node) == 3) ? tflite::micro::GetEvalInput(context, node, kConvBiasTensor) : 0;
				rc_multiplier = 2;
			}
			else if ( reg->builtin_code == BuiltinOperator_AVERAGE_POOL_2D )
				input1 = tflite::micro::GetEvalInput(context, node, 0);
			else if ( reg->builtin_code == BuiltinOperator_PAD )
				input1 = tflite::micro::GetEvalInput(context, node, 0);
			else if ( reg->builtin_code == BuiltinOperator_ADD ) {
				input1 = tflite::micro::GetEvalInput(context, node, 0);
				input2 = tflite::micro::GetEvalInput(context, node, 1);
			}
			else if ( reg->builtin_code == BuiltinOperator_MUL ) {
				input1 = tflite::micro::GetEvalInput(context, node, 0);
				input2 = tflite::micro::GetEvalInput(context, node, 1);
			}
			else if ( reg->builtin_code == BuiltinOperator_HARD_SWISH )
				input1 = tflite::micro::GetEvalInput(context, node, 0);


			uint32_t maxPartSize = 0;
			for ( int j = 0; j < op_part_info->numParts; j++ ) {
				partitionSize = 1;
				for ( int k = 0; k < input1->dims->size; k++ ) {
					if ( k != DIM_IDX_HEIGHT )
						partitionSize = partitionSize * input1->dims->data[k];
				}
				partitionSize = partitionSize * (op_part_info->partitionEnd[j] - op_part_info->partitionStart[j]);

				if (partitionSize > maxPartSize) {
					maxPartSize = partitionSize;
				}

				if ( partitionSize > maxPartitionSize[0] ) {
					maxPartitionSize[4]		= maxPartitionSize[3];
					maxPartitionSize[3]		= maxPartitionSize[2];
					maxPartitionSize[2]		= maxPartitionSize[1];
					maxPartitionSize[1]		= maxPartitionSize[0];
					maxPartitionSize[0]		= partitionSize;

					maxPartitionSizeOp[4]	= maxPartitionSizeOp[3];
					maxPartitionSizeOp[3]	= maxPartitionSizeOp[2];
					maxPartitionSizeOp[2]	= maxPartitionSizeOp[1];
					maxPartitionSizeOp[1]	= maxPartitionSizeOp[0];
					maxPartitionSizeOp[0]	= i;

					maxPartitionSizeOpPart[4]	= maxPartitionSizeOpPart[3];
					maxPartitionSizeOpPart[3]	= maxPartitionSizeOpPart[2];
					maxPartitionSizeOpPart[2]	= maxPartitionSizeOpPart[1];
					maxPartitionSizeOpPart[1]	= maxPartitionSizeOpPart[0];
					maxPartitionSizeOpPart[0]	= j;

//					maxPartitionSize		= partitionSize;
//					maxPartitionSizeOp		= i;
//					maxPartitionSizeOpPart	= j;
				}
			}

			uint32_t input_size = 1;
			for ( int k = 0; k < input1->dims->size; k++ )
				input_size = input_size * input1->dims->data[k];

//			uart_buf_len = sprintf(uart_buf, "Op_%u, ", i);
//			HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
//			uart_buf_len = sprintf(uart_buf, "%u\n", input_size);
//			HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

//			uart_buf_len = sprintf(uart_buf, "Op_%u, ", i);
//			HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
//			uart_buf_len = sprintf(uart_buf, "%u\n", maxPartSize);
//			HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);


			if ( filter ) {
				filterSize = 1;
				for ( int j = 0; j < filter->dims->size; j++ )
					filterSize = filterSize * filter->dims->data[j];

				uint32_t receptiveFieldSize = rc_multiplier * filter->dims->data[1] * filter->dims->data[2] * filter->dims->data[3];

				if ( receptiveFieldSize > maxReceptiveFieldSize[0] ) {
					maxReceptiveFieldSize[4]	= maxReceptiveFieldSize[3];
					maxReceptiveFieldSize[3]	= maxReceptiveFieldSize[2];
					maxReceptiveFieldSize[2]	= maxReceptiveFieldSize[1];
					maxReceptiveFieldSize[1]	= maxReceptiveFieldSize[0];
					maxReceptiveFieldSize[0]	= receptiveFieldSize;

					maxReceptiveFieldSizeOps[4]	= maxReceptiveFieldSizeOps[3];
					maxReceptiveFieldSizeOps[3]	= maxReceptiveFieldSizeOps[2];
					maxReceptiveFieldSizeOps[2]	= maxReceptiveFieldSizeOps[1];
					maxReceptiveFieldSizeOps[1]	= maxReceptiveFieldSizeOps[0];
					maxReceptiveFieldSizeOps[0]	= i;
				}
			}

			if ( filterSize > maxFilterSize[0] ) {
				maxFilterSize[4]	= maxFilterSize[3];
				maxFilterSize[3]	= maxFilterSize[2];
				maxFilterSize[2]	= maxFilterSize[1];
				maxFilterSize[1]	= maxFilterSize[0];
				maxFilterSize[0]	= filterSize;

				maxFilterSizeOps[4]	= maxFilterSizeOps[3];
				maxFilterSizeOps[3]	= maxFilterSizeOps[2];
				maxFilterSizeOps[2]	= maxFilterSizeOps[1];
				maxFilterSizeOps[1]	= maxFilterSizeOps[0];
				maxFilterSizeOps[0]	= i;
			}

			if ( bias ) {
				biasSize = 4;
				for ( int j = 0; j < bias->dims->size; j++ )
					biasSize = biasSize * bias->dims->data[j];
				if ( biasSize > maxBiasSize[0] ) {
					maxBiasSize[4]		= maxBiasSize[3];
					maxBiasSize[3]		= maxBiasSize[2];
					maxBiasSize[2]		= maxBiasSize[1];
					maxBiasSize[1]		= maxBiasSize[0];
					maxBiasSize[0]		= biasSize;

					maxBiasSizeOps[4]	= maxBiasSizeOps[3];
					maxBiasSizeOps[3]	= maxBiasSizeOps[2];
					maxBiasSizeOps[2]	= maxBiasSizeOps[1];
					maxBiasSizeOps[1]	= maxBiasSizeOps[0];
					maxBiasSizeOps[0]	= i;
				}
			}
		}
	}

	tinyOpsBuffSizes.partitionBuff1Size = maxPartitionSize[0] + MEM_ALIGNMENT_PAINT;
	tinyOpsBuffSizes.partitionBuff2Size = maxPartitionSize[0] + MEM_ALIGNMENT_PAINT;
	tinyOpsBuffSizes.partitionFilterSize = maxPartitionSize[0] + MEM_ALIGNMENT_PAINT;
	tinyOpsBuffSizes.partitionIm2colSize = (maxPartitionSize[0] > maxReceptiveFieldSize[0] ? maxPartitionSize[0] : maxReceptiveFieldSize[0]) + MEM_ALIGNMENT_PAINT;
	#ifdef OVERLAY_BIASES
	tinyOpsBuffSizes.biasSize = maxBiasSize[0] + MEM_ALIGNMENT_PAINT;
	#else
	tinyOpsBuffSizes.biasSize = 0;
	#endif
	#ifdef OVERLAY_QUANT_PARAMS
	tinyOpsBuffSizes.quantParamsOutMultiplierSize = maxBiasSize[0] + MEM_ALIGNMENT_PAINT + 128;
	tinyOpsBuffSizes.quantParamsOutShiftSize = maxBiasSize[0] + MEM_ALIGNMENT_PAINT + 128;
	#else
	tinyOpsBuffSizes.quantParamsOutMultiplierSize = 0;
	tinyOpsBuffSizes.quantParamsOutShiftSize = 0;
	#endif

	int32_t tinyops_sram_usage = MEM_ALIGNMENT_PAINT;

	tinyops_sram_usage += tinyOpsBuffSizes.partitionBuff1Size + MEM_ALIGNMENT_PAINT;
	tinyops_sram_usage += tinyOpsBuffSizes.partitionBuff2Size + MEM_ALIGNMENT_PAINT;
	tinyops_sram_usage += tinyOpsBuffSizes.partitionFilterSize + MEM_ALIGNMENT_PAINT;
	tinyops_sram_usage += tinyOpsBuffSizes.partitionIm2colSize + MEM_ALIGNMENT_PAINT;
	tinyops_sram_usage += tinyOpsBuffSizes.biasSize + MEM_ALIGNMENT_PAINT;
	tinyops_sram_usage += tinyOpsBuffSizes.quantParamsOutMultiplierSize + MEM_ALIGNMENT_PAINT;
	tinyops_sram_usage += tinyOpsBuffSizes.quantParamsOutShiftSize + MEM_ALIGNMENT_PAINT;

//	if (tinyops_sram_usage < PLATFORM_AVAILABLE_SRAM) {
//		tinyOpsBuffSizes.partitionFilterSize += (PLATFORM_AVAILABLE_SRAM - tinyops_sram_usage);
//		tinyops_sram_usage += (PLATFORM_AVAILABLE_SRAM - tinyops_sram_usage);
//	}

//	uart_buf_len = sprintf(uart_buf, "Max Partition Size: %u\n", maxPartitionSize);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

//	uart_buf_len = sprintf(uart_buf, "Max Partition Sizes:\n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	for ( int j = 0; j < MAX_RECEPTIVE_FIELD_LIST_SIZE; j++ ) {
//		uart_buf_len = sprintf(uart_buf, "%u, %u: %u\n", maxPartitionSizeOp[j], maxPartitionSizeOpPart[j], maxPartitionSize[j]);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	}
//
//	uart_buf_len = sprintf(uart_buf, "Max Filter Sizes:\n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	for ( int j = 0; j < MAX_RECEPTIVE_FIELD_LIST_SIZE; j++ ) {
//		uart_buf_len = sprintf(uart_buf, "%u: %u\n", maxFilterSizeOps[j], maxFilterSize[j]);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	}
//
//	uart_buf_len = sprintf(uart_buf, "Max Receptive Field Sizes: \n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	for ( int j = 0; j < MAX_RECEPTIVE_FIELD_LIST_SIZE; j++ ) {
//		uart_buf_len = sprintf(uart_buf, "%u: %u\n", maxReceptiveFieldSizeOps[j], maxReceptiveFieldSize[j]);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	}
//
//	uart_buf_len = sprintf(uart_buf, "Max Bias Sizes: \n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	for ( int j = 0; j < MAX_RECEPTIVE_FIELD_LIST_SIZE; j++ ) {
//		uart_buf_len = sprintf(uart_buf, "%u: %u\n", maxBiasSizeOps[j], maxBiasSize[j]);
//		HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	}

}

}

void OVERLAY_CopyInData(	volatile DMACopyQue *				dmaQue,
							volatile PartitionInfo *			op_part_info,
							uint32_t							partNum,
							volatile const TfLiteEvalTensor *	input,
							volatile TfLiteEvalTensor *			overlay_input_tensor ) {

    uint32_t startingRow	= op_part_info->partitionStart[partNum];
	uint32_t endingRow		= op_part_info->partitionEnd[partNum];

	// calculate offset and size for DMA copy for prologue
	uint32_t input_tensor_processed_bytes = startingRow * input->dims->data[0] * input->dims->data[2] * input->dims->data[3];
	uint32_t bytes_in_partition = (endingRow-startingRow)* input->dims->data[0] * input->dims->data[2] * input->dims->data[3];

//	uart_buf_len = sprintf(uart_buf, "op_part_info->numParts: %u us \r\n", op_part_info->numParts);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	uart_buf_len = sprintf(uart_buf, "startingRow: %u us \r\n", startingRow);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	uart_buf_len = sprintf(uart_buf, "endingRow: %u us \r\n", endingRow);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	uart_buf_len = sprintf(uart_buf, "input_tensor_processed_bytes: %u us \r\n", input_tensor_processed_bytes);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	uart_buf_len = sprintf(uart_buf, "bytes_in_partition: %u us \r\n", bytes_in_partition);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	overlay_input_tensor->type			= input->type;		  // fill out tensor with partition data
	overlay_input_tensor->dims->size	= input->dims->size;;
	overlay_input_tensor->dims->data[0]	= input->dims->data[0];
	overlay_input_tensor->dims->data[1]	= op_part_info->inp_partitioned_dim[partNum];
	overlay_input_tensor->dims->data[2]	= input->dims->data[2];
	overlay_input_tensor->dims->data[3]	= input->dims->data[3];

	DMA_Copy_Que_Req_Put(dmaQue, (uint8_t *) &input->data.int8[input_tensor_processed_bytes], (uint8_t *)overlay_input_tensor->data.data, bytes_in_partition);
}

void OVERLAY_CopyInData(	volatile DMACopyQue *				dmaQue,
							volatile PartitionInfo *			op_part_info,
							uint32_t							partNum,
							volatile const TfLiteEvalTensor *	input1,
							volatile TfLiteEvalTensor *			overlay_input1_tensor,
							volatile const TfLiteEvalTensor *	input2,
							volatile TfLiteEvalTensor *			overlay_input2_tensor ) {

    uint32_t startingRow	= op_part_info->partitionStart[partNum];
	uint32_t endingRow		= op_part_info->partitionEnd[partNum];

	// calculate offset and size for DMA copy for prologue
	uint32_t input_tensor_processed_bytes = startingRow * input1->dims->data[0] * input1->dims->data[2] * input1->dims->data[3];
	uint32_t bytes_in_partition = (endingRow-startingRow)* input1->dims->data[0] * input1->dims->data[2] * input1->dims->data[3];

//	uart_buf_len = sprintf(uart_buf, "op_part_info->numParts: %u us \r\n", op_part_info->numParts);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	uart_buf_len = sprintf(uart_buf, "startingRow: %u us \r\n", startingRow);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	uart_buf_len = sprintf(uart_buf, "endingRow: %u us \r\n", endingRow);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	uart_buf_len = sprintf(uart_buf, "input_tensor_processed_bytes: %u us \r\n", input_tensor_processed_bytes);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//	uart_buf_len = sprintf(uart_buf, "bytes_in_partition: %u us \r\n", bytes_in_partition);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	overlay_input1_tensor->type				= input1->type;		  // fill out tensor with partition data
	overlay_input1_tensor->dims->size		= input1->dims->size;;
	overlay_input1_tensor->dims->data[0]	= input1->dims->data[0];
	overlay_input1_tensor->dims->data[1]	= op_part_info->inp_partitioned_dim[partNum];
	overlay_input1_tensor->dims->data[2]	= input1->dims->data[2];
	overlay_input1_tensor->dims->data[3]	= input1->dims->data[3];
	DMA_Copy_Que_Req_Put(dmaQue, (uint8_t *) &input1->data.int8[input_tensor_processed_bytes], (uint8_t *)overlay_input1_tensor->data.data, bytes_in_partition);

	overlay_input2_tensor->type				= input2->type;		  // fill out tensor with partition data
	overlay_input2_tensor->dims->size		= input2->dims->size;;
	overlay_input2_tensor->dims->data[0]	= input2->dims->data[0];
	overlay_input2_tensor->dims->data[1]	= op_part_info->inp_partitioned_dim[partNum];
	overlay_input2_tensor->dims->data[2]	= input2->dims->data[2];
	overlay_input2_tensor->dims->data[3]	= input2->dims->data[3];
	DMA_Copy_Que_Req_Put(dmaQue, (uint8_t *) &input2->data.int8[input_tensor_processed_bytes], (uint8_t *)overlay_input2_tensor->data.data, bytes_in_partition);

}

void OVERLAY_CopyInData(	volatile DMACopyQue *			dmaQue,
							volatile PartitionInfo *		op_part_info,
							uint32_t						partNum,
							volatile TfLiteEvalTensor *		overlay_input1_tensor,
							volatile TfLiteEvalTensor *		overlay_input2_tensor ) {

	const TfLiteEvalTensor * input1 = op_part_info->input1;
	const TfLiteEvalTensor * input2 = op_part_info->input2;

	if ( op_part_info->partSizes[partNum] != 0) {

	    uint32_t startingRow	= op_part_info->partitionStart[partNum];
		uint32_t endingRow		= op_part_info->partitionEnd[partNum];

		// calculate offset and size for DMA copy for prologue
		uint32_t input_tensor_processed_bytes = startingRow * input1->dims->data[0] * input1->dims->data[2] * input1->dims->data[3];
		uint32_t bytes_in_partition = (endingRow-startingRow)* input1->dims->data[0] * input1->dims->data[2] * input1->dims->data[3];

		overlay_input1_tensor->type				= input1->type;		  // fill out tensor with partition data
		overlay_input1_tensor->dims->size		= input1->dims->size;;
		overlay_input1_tensor->dims->data[0]	= input1->dims->data[0];
		overlay_input1_tensor->dims->data[1]	= op_part_info->inp_partitioned_dim[partNum];
		overlay_input1_tensor->dims->data[2]	= input1->dims->data[2];
		overlay_input1_tensor->dims->data[3]	= input1->dims->data[3];
		DMA_Copy_Que_Req_Put(dmaQue, (uint8_t *) &input1->data.int8[input_tensor_processed_bytes], (uint8_t *)overlay_input1_tensor->data.data, bytes_in_partition);

		if ( op_part_info->input2 ) {
			overlay_input2_tensor->type				= input2->type;		  // fill out tensor with partition data
			overlay_input2_tensor->dims->size		= input2->dims->size;;
			overlay_input2_tensor->dims->data[0]	= input2->dims->data[0];
			overlay_input2_tensor->dims->data[1]	= op_part_info->inp_partitioned_dim[partNum];
			overlay_input2_tensor->dims->data[2]	= input2->dims->data[2];
			overlay_input2_tensor->dims->data[3]	= input2->dims->data[3];
			DMA_Copy_Que_Req_Put(dmaQue, (uint8_t *) &input2->data.int8[input_tensor_processed_bytes], (uint8_t *)overlay_input2_tensor->data.data, bytes_in_partition);
		}
	}

}

void DMA_CopyTensor(	volatile DMACopyQue *				dmaQue,
						volatile const TfLiteEvalTensor *	src_tensor,
						volatile TfLiteEvalTensor *			dst_tensor ) {

	dst_tensor->type			= src_tensor->type;
	dst_tensor->dims->size		= src_tensor->dims->size;
	dst_tensor->dims->data[0]	= src_tensor->dims->data[0];
	dst_tensor->dims->data[1]	= src_tensor->dims->data[1];
	dst_tensor->dims->data[2]	= src_tensor->dims->data[2];
	dst_tensor->dims->data[3]	= src_tensor->dims->data[3];

	int32_t tensor_size = 1;
	if ( dst_tensor->type == kTfLiteInt32 )
		tensor_size = tensor_size * 4;
	for ( int i = 0; i < src_tensor->dims->size; i++ )
		tensor_size = tensor_size * src_tensor->dims->data[i];

	DMA_Copy_Que_Req_Put(dmaQue, (uint8_t *) src_tensor->data.data, (uint8_t *) dst_tensor->data.data, tensor_size );
}

#endif
