#include "dmacopying.h"
#include "main.h"
#include "stm32f7xx_it.h"


void StartTransfer( volatile DMACopyQue * que )
{
  char uart_buf[50];
  int uart_buf_len;

  if (que->readIdx != que->writeIdx) { // Que not empty condition
	uint8_t * src = que->copyReq[que->readIdx].src;
	uint8_t * dst = que->copyReq[que->readIdx].dst;
	uint32_t size = que->copyReq[que->readIdx].size;

	SCB_CleanDCache_by_Addr( (uint32_t*) ( ( (uint32_t)src ) & ~(uint32_t)0x1F), size + 32 );

//    SCB_InvalidateDCache_by_Addr( (uint32_t*) ( ( (uint32_t)dst ) & ~(uint32_t)0x1F), size + 32 );
	if (HAL_DMA_Start_IT(que->dmaHandle, (uint32_t)src, (uint32_t)dst, size) != HAL_OK)
	{
	  /* Transfer Error */
	  Error_Handler();
	}

	// increment read pointer when copying complete
	// on completion of copy, interrupt generated will be cached in background
	// next copy will start only when current ISR exits and the cached one is called
	// therefore only a max of one interrupt will be cached/pending while we are still in ISR

  }

}

int8_t DMA_Copy_Que_Full( volatile DMACopyQue * que ) {

	int8_t nextwriteIdx = que->writeIdx + 1;
	if (nextwriteIdx == DMA_COPY_QUE_SIZE) {
		nextwriteIdx = 0;
	}

	if (nextwriteIdx == que->readIdx)
		return DMA_COPY_QUE_FULL;
	else
		return DMA_COPY_QUE_NOT_FULL;

}

int8_t DMA_Copy_Que_Empty( volatile DMACopyQue * que ) {

	if ( que->writeIdx == que->readIdx )
		return DMA_COPY_QUE_EMPTY;
	else
		return DMA_COPY_QUE_NOT_EMPTY;

}

int8_t DMA_Copy_Que_Req_Put( volatile DMACopyQue * que, uint8_t * src, uint8_t * dst, uint32_t size ) {

//	if (!DMA_Copy_Que_Full(que)) {
//		que->copyReq[que->writeIdx].src	= src;
//		que->copyReq[que->writeIdx].dst	= dst;
//		que->copyReq[que->writeIdx].size = size;
//
//		que->writeIdx++;
//		if (que->writeIdx == DMA_COPY_QUE_SIZE)
//			que->writeIdx = 0;
//
//		if (que->dmaHandle->State != HAL_DMA_STATE_BUSY)
//			StartTransfer(que);
//
//		return DMA_COPY_REQ_PUT_OK;
//
//	}
//	else {
//		return DMA_COPY_REQ_PUT_FAIL;
//	}

	uint32_t size_copied = 0;

	uint8_t * copy_src = src;
	uint8_t * copy_dst = dst;
	uint32_t  copy_size = 0;

	while ( size_copied != size )
	{
		copy_src += copy_size;
		copy_dst += copy_size;

		if ( (size-size_copied) > HAL_DMA_MAX_COPY_SIZE )
			copy_size = DMA_MAX_COPY_SIZE;
		else
			copy_size = size - size_copied;
		size_copied += copy_size;

		if (!DMA_Copy_Que_Full(que)) {
			que->copyReq[que->writeIdx].src	= copy_src;
			que->copyReq[que->writeIdx].dst	= copy_dst;
			que->copyReq[que->writeIdx].size = copy_size;

			que->writeIdx++;
			if (que->writeIdx == DMA_COPY_QUE_SIZE)
				que->writeIdx = 0;

			if (que->dmaHandle->State != HAL_DMA_STATE_BUSY)
				StartTransfer(que);

		}
		else {
			return DMA_COPY_REQ_PUT_FAIL;
		}
	}

	return DMA_COPY_REQ_PUT_OK;
}
