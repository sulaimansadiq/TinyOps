/*
 * overlay_fw.h
 *
 *  Created on: 22 May 2021
 *      Author: ss2n18
 */

#ifndef INC_OVERLAY_FW_H_
#define INC_OVERLAY_FW_H_

#include "overlay_base.h"

#ifdef OVERLAY_FW
#include "tensorflow/lite/micro/micro_allocator.h"
extern "C" {
#include "dmacopying.h"
}

typedef struct PartitionInfo {
	TfLiteIntArray *	dims;

	const TfLiteEvalTensor *	input1;
	const TfLiteEvalTensor *	input2;

	const TfLiteEvalTensor *	filter;
	const TfLiteEvalTensor *	bias;

	uint8_t				numParts;

	uint32_t			partSizes[PARTITION_FACTOR];
	uint32_t			partitionStart[PARTITION_FACTOR];
	uint32_t			partitionEnd[PARTITION_FACTOR];

	uint32_t			inp_partitioned_dim[PARTITION_FACTOR];
	uint32_t			out_partitioned_dim[PARTITION_FACTOR];

	uint32_t			op_part_data[PARTITION_FACTOR][PARTITION_OP_DATA_SIZE];


} PartitionInfo;

//typedef struct {
//  TfLiteNode node;
//  const TfLiteRegistration* registration;
//} NodeAndRegistration;

typedef struct PartitionedExecutionState {

	uint32_t pingPong;			// flag used to select ping-pong buffer

	uint32_t totalOps;			// Total Operations in Graph
	uint32_t totalParts;		// Partition Factor

	// Current execution state, where are we in the inference graph right now?
	uint32_t currentOp;			// Current Op being executed
	uint32_t currentPart;		// Current Part being executed
	uint32_t processed_dims[MAX_NUM_DIMS];

	// declaring a global copy of the pointer to this struct because of access issues
	tflite::NodeAndRegistration * nodes_and_regs_;

	/* This needs a proper permanent solution, instead of just having a MAX_NUM_OPERATIONS here */
	volatile PartitionInfo ops_parts_info[MAX_NUM_OPERATIONS];

} PartitionedExecutionState;

void OVERLAY_CopyInData( volatile DMACopyQue *, volatile PartitionInfo *, uint32_t, volatile const TfLiteEvalTensor *, volatile TfLiteEvalTensor * );
void OVERLAY_CopyInData( volatile DMACopyQue *, volatile PartitionInfo *, uint32_t, volatile const TfLiteEvalTensor *, volatile TfLiteEvalTensor *, volatile const TfLiteEvalTensor *, volatile TfLiteEvalTensor * );
void OVERLAY_CopyInData( volatile DMACopyQue *, volatile PartitionInfo *,	uint32_t, volatile TfLiteEvalTensor *, volatile TfLiteEvalTensor * );
void DMA_CopyTensor( volatile DMACopyQue *, volatile const TfLiteEvalTensor *, volatile TfLiteEvalTensor * );

#endif
#endif /* INC_OVERLAY_FW_H_ */
