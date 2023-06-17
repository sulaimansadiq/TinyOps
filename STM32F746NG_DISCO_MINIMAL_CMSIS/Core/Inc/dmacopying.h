#ifndef DMA_COPYING
#define DMA_COPYING

#include "main.h"

//namespace tflite {

#define DMA_COPY_QUE_SIZE 16

#define DMA_COPY_REQ_PUT_OK		1
#define DMA_COPY_REQ_PUT_FAIL	0

#define DMA_COPY_QUE_FULL		1
#define DMA_COPY_QUE_NOT_FULL	0

#define DMA_COPY_QUE_EMPTY		1
#define	DMA_COPY_QUE_NOT_EMPTY	0

#define DMA_MAX_COPY_SIZE		32768 // 8192  // 32768

#define HAL_DMA_MAX_COPY_SIZE	65535 // 8192  // 65535

typedef struct DMACopyReq {

	uint8_t *	src;
	uint8_t *	dst;
	uint32_t	size;

} DMACopyReq;

typedef struct DMACopyQue {

	DMA_HandleTypeDef *	dmaHandle;
	DMACopyReq			copyReq[DMA_COPY_QUE_SIZE];
	volatile uint32_t	readIdx;
	volatile uint32_t	writeIdx;

} DMACopyQue;

int8_t DMA_Copy_Que_Empty( volatile DMACopyQue * que );
int8_t DMA_Copy_Que_Full( volatile DMACopyQue * que );
int8_t DMA_Copy_Que_Req_Put( volatile DMACopyQue * que, uint8_t * src, uint8_t * dst, uint32_t size );

void DMA_Config(void);
void StartTransfer( volatile DMACopyQue * que );

void TransferComplete( DMA_HandleTypeDef *DmaHandle );
void TransferError( DMA_HandleTypeDef *DmaHandle );
void TransferComplete_Strm1_Ch0( DMA_HandleTypeDef *DmaHandle );
void TransferError_Strm1_Ch0( DMA_HandleTypeDef *DmaHandle );
//}

#endif
