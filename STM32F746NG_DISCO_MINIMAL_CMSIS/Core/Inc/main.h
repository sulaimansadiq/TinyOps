/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

#define DEBUG_PRINTS
#define PLATFORM_AVAILABLE_SRAM		319900 //315000
#define MEM_ALIGNMENT_PAINT			32

#define MODELS_BASE_PATH			"0:/models"

//#define FILENAME_PATTERN			"??????????????????????????????"
//#define FILENAME_LENGTH				30

#define FILE_PATH_LENGTH			FILENAME_LENGTH + 100

//#define FILENAME_PATTERN			"mcunet_f412-w1.00-r160.tflite"
//#define FILENAME_PATTERN			"?????????????????????????????"
//#define FILENAME_LENGTH				29
//#define OUTPUT_STATS_PATH			"latency_stats_mcunet.csv"

//#define FILENAME_PATTERN			"mbv3-w0.10-r192.tflite"
//#define FILENAME_PATTERN			"??????????????????????"
//#define FILENAME_LENGTH				22
//#define OUTPUT_STATS_PATH			"latency_stats_mbv3.csv"

//#define FILENAME_PATTERN			"mnasnet-w0.10-r192.tflite"
//#define FILENAME_PATTERN			"?????????????????????????"
//#define FILENAME_LENGTH				25
//#define OUTPUT_STATS_PATH			"latency_stats_mnasnet.csv"

//#define FILENAME_PATTERN			"efficientnet-w0.10-r192.tflite"
//#define FILENAME_PATTERN			"??????????????????????????????"
//#define FILENAME_LENGTH				30
//#define OUTPUT_STATS_PATH			"latency_stats_efficientnet.csv"

//#define	FILENAME_PATTERN			"proxyless_mod-w0.90-r144.tflite"
//#define FILENAME_PATTERN			"???????????????????????????????"
//#define FILENAME_LENGTH				31
//#define OUTPUT_STATS_PATH			"latency_stats_proxyless_mod.csv"

//#define FILENAME_PATTERN			"proxyless-w0.10-r192.tflite"
#define FILENAME_PATTERN			"???????????????????????????"
#define FILENAME_LENGTH				27
#define OUTPUT_STATS_PATH			"latency_stats_proxyless.csv"

#define OUTPUT_DUMP_BASE_PATH		"0:/output_dump/"
#define OUTPUT_DUMP_PREFIX			"out_dump_"
#define OUTPUT_DUMP_SIZE			1000

//#define MODEL_LOC_INT_FLASH

#define MODEL_INT_FLASH_ADDRESS		0x08040000
//#define MODEL_TENSOR_ARENA_SDRAM

//#define MODEL_SDRAM_TOTAL_ARENA		8102000
//#define MODEL_SDRAM_TOTAL_ARENA		6102000
#define MODEL_SDRAM_TOTAL_ARENA		5376000

#define MODEL_SDRAM_OFFSET			1024			// offset required otherwise reading from sdcard to base of sdram was buggy with bits flipping

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
#define SDRAM_TIMEOUT     ((uint32_t)0xFFFF)

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

#define QUAD_OUT_FAST_READ_CMD               	 0x6B
#define DUMMY_CLOCK_CYCLES_READ_QUAD         	 10

#define READ_VOL_CFG_REG_CMD                 	 0x85
#define WRITE_VOL_CFG_REG_CMD                	 0x81

/* Write Operations */
#define WRITE_ENABLE_CMD                     	 0x06
#define WRITE_DISABLE_CMD                    	 0x04

/* Register Operations */
#define READ_STATUS_REG_CMD                  	 0x05
#define WRITE_STATUS_REG_CMD                 	 0x01

/* Definition for QSPI clock resources */
#define QSPI_CLK_ENABLE()          __HAL_RCC_QSPI_CLK_ENABLE()
#define QSPI_CLK_DISABLE()         __HAL_RCC_QSPI_CLK_DISABLE()
#define QSPI_CS_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOB_CLK_ENABLE()
#define QSPI_CLK_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define QSPI_D0_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOD_CLK_ENABLE()
#define QSPI_D1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOD_CLK_ENABLE()
#define QSPI_D2_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOE_CLK_ENABLE()
#define QSPI_D3_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOD_CLK_ENABLE()
#define QSPI_DMA_CLK_ENABLE()      __HAL_RCC_DMA2_CLK_ENABLE()

#define QSPI_FORCE_RESET()         __HAL_RCC_QSPI_FORCE_RESET()
#define QSPI_RELEASE_RESET()       __HAL_RCC_QSPI_RELEASE_RESET()

/* Definition for QSPI Pins */
#define QSPI_CS_PIN                GPIO_PIN_6
#define QSPI_CS_GPIO_PORT          GPIOB
#define GPIO_AF_CS                 GPIO_AF10_QUADSPI

#define QSPI_CLK_PIN               GPIO_PIN_2
#define QSPI_CLK_GPIO_PORT         GPIOB
#define GPIO_AF_CLK                GPIO_AF9_QUADSPI

#define QSPI_D0_PIN                GPIO_PIN_11
#define QSPI_D0_GPIO_PORT          GPIOD
#define GPIO_AF_D0                 GPIO_AF9_QUADSPI

#define QSPI_D1_PIN                GPIO_PIN_12
#define QSPI_D1_GPIO_PORT          GPIOD
#define GPIO_AF_D1                 GPIO_AF9_QUADSPI

#define QSPI_D2_PIN                GPIO_PIN_2
#define QSPI_D2_GPIO_PORT          GPIOE
#define GPIO_AF_D2                 GPIO_AF9_QUADSPI

#define QSPI_D3_PIN                GPIO_PIN_13
#define QSPI_D3_GPIO_PORT          GPIOD
#define GPIO_AF_D3                 GPIO_AF9_QUADSPI

/* Definition for QSPI DMA */
#define QSPI_DMA_INSTANCE          DMA2_Stream7
#define QSPI_DMA_CHANNEL           DMA_CHANNEL_3
#define QSPI_DMA_IRQ               DMA2_Stream7_IRQn
#define QSPI_DMA_IRQ_HANDLER       DMA2_Stream7_IRQHandler

/* N25Q512A Micron memory */
/* Size of the flash */
#define QSPI_FLASH_SIZE                      23
#define QSPI_PAGE_SIZE                       256

/* Reset Operations */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99

/* Identification Operations */
#define READ_ID_CMD                          0x9E
#define READ_ID_CMD2                         0x9F
#define MULTIPLE_IO_READ_ID_CMD              0xAF
#define READ_SERIAL_FLASH_DISCO_PARAM_CMD    0x5A

/* Read Operations */
#define READ_CMD                             0x03
#define READ_4_BYTE_ADDR_CMD                 0x13

#define FAST_READ_CMD                        0x0B
#define FAST_READ_DTR_CMD                    0x0D
#define FAST_READ_4_BYTE_ADDR_CMD            0x0C

#define DUAL_OUT_FAST_READ_CMD               0x3B
#define DUAL_OUT_FAST_READ_DTR_CMD           0x3D
#define DUAL_OUT_FAST_READ_4_BYTE_ADDR_CMD   0x3C

#define DUAL_INOUT_FAST_READ_CMD             0xBB
#define DUAL_INOUT_FAST_READ_DTR_CMD         0xBD
#define DUAL_INOUT_FAST_READ_4_BYTE_ADDR_CMD 0xBC

#define QUAD_OUT_FAST_READ_CMD               0x6B
#define QUAD_OUT_FAST_READ_DTR_CMD           0x6D
#define QUAD_OUT_FAST_READ_4_BYTE_ADDR_CMD   0x6C

#define QUAD_INOUT_FAST_READ_CMD             0xEB
#define QUAD_INOUT_FAST_READ_DTR_CMD         0xED
#define QUAD_INOUT_FAST_READ_4_BYTE_ADDR_CMD 0xEC

/* Write Operations */
#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04

/* Register Operations */
#define READ_STATUS_REG_CMD                  0x05
#define WRITE_STATUS_REG_CMD                 0x01

#define READ_LOCK_REG_CMD                    0xE8
#define WRITE_LOCK_REG_CMD                   0xE5

#define READ_FLAG_STATUS_REG_CMD             0x70
#define CLEAR_FLAG_STATUS_REG_CMD            0x50

#define READ_NONVOL_CFG_REG_CMD              0xB5
#define WRITE_NONVOL_CFG_REG_CMD             0xB1

#define READ_VOL_CFG_REG_CMD                 0x85
#define WRITE_VOL_CFG_REG_CMD                0x81

#define READ_ENHANCED_VOL_CFG_REG_CMD        0x65
#define WRITE_ENHANCED_VOL_CFG_REG_CMD       0x61

#define READ_EXT_ADDR_REG_CMD                0xC8
#define WRITE_EXT_ADDR_REG_CMD               0xC5

/* Program Operations */
#define PAGE_PROG_CMD                        0x02
#define PAGE_PROG_4_BYTE_ADDR_CMD            0x12

#define DUAL_IN_FAST_PROG_CMD                0xA2
#define EXT_DUAL_IN_FAST_PROG_CMD            0xD2

#define QUAD_IN_FAST_PROG_CMD                0x32
#define EXT_QUAD_IN_FAST_PROG_CMD            0x12 /*0x38*/
#define QUAD_IN_FAST_PROG_4_BYTE_ADDR_CMD    0x34

/* Erase Operations */
#define SUBSECTOR_ERASE_CMD                  0x20
#define SUBSECTOR_ERASE_4_BYTE_ADDR_CMD      0x21

#define SECTOR_ERASE_CMD                     0xD8
#define SECTOR_ERASE_4_BYTE_ADDR_CMD         0xDC

#define BULK_ERASE_CMD                       0xC7

#define PROG_ERASE_RESUME_CMD                0x7A
#define PROG_ERASE_SUSPEND_CMD               0x75

/* One-Time Programmable Operations */
#define READ_OTP_ARRAY_CMD                   0x4B
#define PROG_OTP_ARRAY_CMD                   0x42

/* 4-byte Address Mode Operations */
#define ENTER_4_BYTE_ADDR_MODE_CMD           0xB7
#define EXIT_4_BYTE_ADDR_MODE_CMD            0xE9

/* Quad Operations */
#define ENTER_QUAD_CMD                       0x35
#define EXIT_QUAD_CMD                        0xF5

/* Default dummy clocks cycles */
#define DUMMY_CLOCK_CYCLES_READ              8
#define DUMMY_CLOCK_CYCLES_READ_QUAD         10

#define DUMMY_CLOCK_CYCLES_READ_DTR          6
#define DUMMY_CLOCK_CYCLES_READ_QUAD_DTR     8

/* End address of the QSPI memory */
#define QSPI_END_ADDR              (1 << QSPI_FLASH_SIZE)

/* DMA Defines */

#define DMA_INSTANCE               DMA2_Stream0
#define DMA_CHANNEL                DMA_CHANNEL_0
#define DMA_INSTANCE_IRQ           DMA2_Stream0_IRQn
#define DMA_INSTANCE_IRQHANDLER    DMA2_Stream0_IRQHandler

/* Size of buffers */
#define BUFFERSIZE                 (COUNTOF(aTxBuffer) - 1)

/* Exported macro ------------------------------------------------------------*/
#define COUNTOF(__BUFFER__)        (sizeof(__BUFFER__) / sizeof(*(__BUFFER__)))

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
