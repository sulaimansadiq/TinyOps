/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dmacopying.h"
#include "overlay_base.h"
#include "model.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim10;

UART_HandleTypeDef huart3;

DMA_HandleTypeDef hdma_memtomem_dma2_stream0;
DMA_HandleTypeDef hdma_memtomem_dma2_stream1;
/* USER CODE BEGIN PV */
SDRAM_HandleTypeDef      hsdram;
FMC_SDRAM_TimingTypeDef  SDRAM_Timing;
FMC_SDRAM_CommandTypeDef command;

extern uint32_t _qspi_init_base;
extern uint32_t _qspi_init_length;

QSPI_HandleTypeDef QSPIHandle;
__IO uint8_t CmdCplt, RxCplt, TxCplt, StatusMatch;

static __IO uint32_t transferErrorDetected;    /* Set to 1 if an error transfer is detected */
static __IO uint32_t transferCompleteDetected; /* Set to 1 if transfer is correctly completed */

volatile DMACopyQue DMAQue[2];
DMA_HandleTypeDef * DmaHandle[2];

#define BUFFER_SIZE 32
//static uint32_t __attribute__((section (".sdram_bss"))) aSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss"))) aDST_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss"))) bSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss"))) bDST_Buffer[BUFFER_SIZE];
static uint8_t aSRC_Const_Buffer[BUFFER_SIZE];
static uint8_t aDST_Buffer[BUFFER_SIZE];
static uint8_t bSRC_Const_Buffer[BUFFER_SIZE];
static uint8_t bDST_Buffer[BUFFER_SIZE];
static uint8_t cSRC_Const_Buffer[BUFFER_SIZE];
static uint8_t cDST_Buffer[BUFFER_SIZE];

//volatile uint8_t __attribute__((section (".qspi_data"))) tensor_arena2[] = {12, 11, 10, 9, 8, 7, 6, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//volatile uint8_t tensor_arena[] = {5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//volatile uint8_t read_arena[5];

volatile uint8_t * debugPrintPtr = (uint8_t *)0xC0800000;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM10_Init(void);
/* USER CODE BEGIN PFP */

static void QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi);
static void QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi);
static void QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi);
static void GpioToggle(void);
static void QSPI_Init_();
static void BSP_SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command);
static void Fill_Buffer(uint32_t *pBuffer, uint32_t uwBufferLenght, uint32_t uwOffset);
static void FMC_Init_();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void init(void) {

	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	#ifdef OVERLAY_FW
	MX_DMA_Init();
	#endif

	QSPI_Init_();
	FMC_Init_();

	for (int i = 0; i<40000; i++)
		*((unsigned char *)(0xC0800000+i)) = 0x20;

}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  char uart_buf[50];
  int uart_buf_len;
  uint16_t timer_val;
  uint16_t timer_val2;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  #ifdef OVERLAY_FW
  MX_DMA_Init();
  #endif
  /* USER CODE BEGIN 2 */

//  FLASH->ACR = FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;
  FLASH->ACR = FLASH_ACR_PRFTEN |FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;
  #ifdef DEBUG_PRINTS
  timer_val = HAL_GetTick(); // __HAL_TIM_GET_COUNTER(&htim10);
  uart_buf_len = sprintf(debugPrintPtr, "Time_Stamp 1: %u ... \n", timer_val);
  debugPrintPtr += 32;
  #endif

  DmaHandle[0] = &hdma_memtomem_dma2_stream0;
  DmaHandle[1] = &hdma_memtomem_dma2_stream1;

  DMAQue[0].dmaHandle	=	DmaHandle[0];
  DMAQue[0].readIdx		=	0;
  DMAQue[0].writeIdx	=	0;

  DMAQue[1].dmaHandle	=	DmaHandle[1];
  DMAQue[1].readIdx		=	0;
  DMAQue[1].writeIdx	=	0;

  HAL_DMA_RegisterCallback(DmaHandle[0], HAL_DMA_XFER_CPLT_CB_ID, TransferComplete);
  HAL_DMA_RegisterCallback(DmaHandle[0], HAL_DMA_XFER_ERROR_CB_ID, TransferError);

  HAL_DMA_RegisterCallback(DmaHandle[1], HAL_DMA_XFER_CPLT_CB_ID, TransferComplete_Strm1_Ch0);
  HAL_DMA_RegisterCallback(DmaHandle[1], HAL_DMA_XFER_ERROR_CB_ID, TransferError_Strm1_Ch0);

  #ifdef DEBUG_PRINTS
  timer_val = HAL_GetTick(); // __HAL_TIM_GET_COUNTER(&htim10);
  uart_buf_len = sprintf(debugPrintPtr, "Time_Stamp 2: %u ... \n", timer_val);
  debugPrintPtr += 32;
  #endif

//  BSP_LED_Init(LED1);
//  BSP_LED_Init(LED2);
//  BSP_LED_Init(LED3);
//  BSP_LED_Init(LED4);
//
//  BSP_LED_Off(LED1);
//  BSP_LED_Off(LED2);
//  BSP_LED_Off(LED3);
//  BSP_LED_Off(LED4);

  #ifdef DEBUG_PRINTS
  timer_val = HAL_GetTick(); // __HAL_TIM_GET_COUNTER(&htim10);
  uart_buf_len = sprintf(debugPrintPtr, "Time_Stamp 3: %u ... \n", timer_val);
  debugPrintPtr += 32;
  #endif

  #ifdef DEBUG_PRINTS
  //  HAL_TIM_Base_Start(&htim10);
  uart_buf_len = sprintf(debugPrintPtr, "Starting setup ... \r\n");
  debugPrintPtr += 32;
//  HAL_UART_Transmit(&huart3, (uint8_t *)uart_buf, uart_buf_len, 100);
  #endif

  setup();

  #ifdef DEBUG_PRINTS
  uart_buf_len = sprintf(debugPrintPtr, "Finished setup ... \r\n");
  debugPrintPtr += 32;
//  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
  #endif

  #ifdef DEBUG_PRINTS
  timer_val = HAL_GetTick(); // __HAL_TIM_GET_COUNTER(&htim10);
  uart_buf_len = sprintf(debugPrintPtr, "Time_Stamp 4: %u ... \n", timer_val);
  debugPrintPtr += 32;
  #endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	#ifdef DEBUG_PRINTS
    uart_buf_len = sprintf(debugPrintPtr, "%u ms ... \r\n", timer_val);
    debugPrintPtr += 32;
	#endif

	#ifdef DEBUG_PRINTS
	uart_buf_len = sprintf(debugPrintPtr, "Starting Inference... \n");
    debugPrintPtr += 32;
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
	#endif

	#ifdef DEBUG_PRINTS
    timer_val = HAL_GetTick(); // __HAL_TIM_GET_COUNTER(&htim10);
    uart_buf_len = sprintf(debugPrintPtr, "Time_Stamp 5: %u ... \n", timer_val);
    debugPrintPtr += 32;
	#endif

	loop();

	#ifdef DEBUG_PRINTS
    timer_val = HAL_GetTick(); // __HAL_TIM_GET_COUNTER(&htim10);
    uart_buf_len = sprintf(debugPrintPtr, "Time_Stamp 6: %u ... \n", timer_val);
    debugPrintPtr += 32;
	#endif

	#ifdef DEBUG_PRINTS
	uart_buf_len = sprintf(debugPrintPtr, "Finished Inference... \n", timer_val);
    debugPrintPtr += 32;
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
	#endif

//    BSP_LED_On(LED1);
//    BSP_LED_On(LED2);
//    BSP_LED_On(LED3);
//    BSP_LED_On(LED4);
//    BSP_LED_Toggle(LED1);
//    BSP_LED_Toggle(LED2);
//    BSP_LED_Toggle(LED3);
//    BSP_LED_Toggle(LED4);

	#ifdef DEBUG_PRINTS
    timer_val = HAL_GetTick(); // __HAL_TIM_GET_COUNTER(&htim10);
    uart_buf_len = sprintf(debugPrintPtr, "Time_Stamp 7: %u ... \n", timer_val);
    debugPrintPtr += 32;
	#endif

	#ifdef DEBUG_PRINTS
    timer_val2 = __HAL_TIM_GET_COUNTER(&htim10);
    uart_buf_len = sprintf(debugPrintPtr, "Time_Stamp 8: %u ... \n", timer_val2);
    debugPrintPtr += 32;
	#endif

//    if (uart_buf_len)
//    	while(1);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{

  /* USER CODE BEGIN TIM10_Init 0 */

  /* USER CODE END TIM10_Init 0 */

  /* USER CODE BEGIN TIM10_Init 1 */

  /* USER CODE END TIM10_Init 1 */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 1800-1;
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 65535;
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM10_Init 2 */

  /* USER CODE END TIM10_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
//  huart3.Init.Parity = UART_PARITY_EVEN;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_memtomem_dma2_stream0
  *   hdma_memtomem_dma2_stream1
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma2_stream0 on DMA2_Stream0 */
  hdma_memtomem_dma2_stream0.Instance = DMA2_Stream0;
  hdma_memtomem_dma2_stream0.Init.Channel = DMA_CHANNEL_0;
  hdma_memtomem_dma2_stream0.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma2_stream0.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma2_stream0.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma2_stream0.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_memtomem_dma2_stream0.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_memtomem_dma2_stream0.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma2_stream0.Init.Priority = DMA_PRIORITY_LOW;
  hdma_memtomem_dma2_stream0.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdma_memtomem_dma2_stream0.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdma_memtomem_dma2_stream0.Init.MemBurst = DMA_MBURST_SINGLE;
  hdma_memtomem_dma2_stream0.Init.PeriphBurst = DMA_PBURST_SINGLE;
  if (HAL_DMA_Init(&hdma_memtomem_dma2_stream0) != HAL_OK)
  {
    Error_Handler( );
  }

  /* Configure DMA request hdma_memtomem_dma2_stream1 on DMA2_Stream1 */
  hdma_memtomem_dma2_stream1.Instance = DMA2_Stream1;
  hdma_memtomem_dma2_stream1.Init.Channel = DMA_CHANNEL_0;
  hdma_memtomem_dma2_stream1.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma2_stream1.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma2_stream1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma2_stream1.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_memtomem_dma2_stream1.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_memtomem_dma2_stream1.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma2_stream1.Init.Priority = DMA_PRIORITY_LOW;
  hdma_memtomem_dma2_stream1.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdma_memtomem_dma2_stream1.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdma_memtomem_dma2_stream1.Init.MemBurst = DMA_MBURST_SINGLE;
  hdma_memtomem_dma2_stream1.Init.PeriphBurst = DMA_PBURST_SINGLE;
  if (HAL_DMA_Init(&hdma_memtomem_dma2_stream1) != HAL_OK)
  {
    Error_Handler( );
  }

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/**
  * @brief  Perform the SDRAM exernal memory inialization sequence
  * @param  hsdram: SDRAM handle
  * @param  Command: Pointer to SDRAM command structure
  * @retval None
  */
static void BSP_SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command)
{
  __IO uint32_t tmpmrd =0;
  /* Step 3:  Configure a clock configuration enable command */
  Command->CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
  Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
  Command->AutoRefreshNumber = 1;
  Command->ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

  /* Step 4: Insert 100 us minimum delay */
  /* Inserted delay is equal to 1 ms due to systick time base unit (ms) */
  HAL_Delay(1);

  /* Step 5: Configure a PALL (precharge all) command */
  Command->CommandMode = FMC_SDRAM_CMD_PALL;
  Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
  Command->AutoRefreshNumber = 1;
  Command->ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

  /* Step 6 : Configure a Auto-Refresh command */
  Command->CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
  Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
  Command->AutoRefreshNumber = 8;
  Command->ModeRegisterDefinition = 0;

  /* Send the command */
  HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

  /* Step 7: Program the external memory mode register */
  tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1          |
                     SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
                     SDRAM_MODEREG_CAS_LATENCY_3           |
                     SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                     SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

  Command->CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
  Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
  Command->AutoRefreshNumber = 1;
  Command->ModeRegisterDefinition = tmpmrd;

  /* Send the command */
  HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

  /* Step 8: Set the refresh rate counter */
  /* (15.62 us x Freq) - 20 */
  /* Set the device refresh counter */
  hsdram->Instance->SDRTR |= ((uint32_t)((1386)<< 1));
}

static void FMC_Init_() {

	/*##-1- Configure the SDRAM device #########################################*/
	/* SDRAM device configuration */
	hsdram.Instance = FMC_SDRAM_DEVICE;

	SDRAM_Timing.LoadToActiveDelay    = 2;
	SDRAM_Timing.ExitSelfRefreshDelay = 6;
	SDRAM_Timing.SelfRefreshTime      = 4;
	SDRAM_Timing.RowCycleDelay        = 6;
	SDRAM_Timing.WriteRecoveryTime    = 2;
	SDRAM_Timing.RPDelay              = 2;
	SDRAM_Timing.RCDDelay             = 2;

	hsdram.Init.SDBank             = FMC_SDRAM_BANK1;
	hsdram.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8;
	hsdram.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
	hsdram.Init.MemoryDataWidth    = SDRAM_MEMORY_WIDTH;
	hsdram.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
	hsdram.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_3;
	hsdram.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
	hsdram.Init.SDClockPeriod      = SDCLOCK_PERIOD;
	hsdram.Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE;
	hsdram.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0;

	/* Initialize the SDRAM controller */
	if(HAL_SDRAM_Init(&hsdram, &SDRAM_Timing) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	/* Program the SDRAM external device */
	BSP_SDRAM_Initialization_Sequence(&hsdram, &command);

}

static void QSPI_Init_() {

	QSPI_CommandTypeDef      sCommand;
	QSPI_MemoryMappedTypeDef sMemMappedCfg;
	__IO uint32_t qspi_addr = 0;
	uint8_t *flash_addr = 0;
	__IO uint8_t step = 0;
	uint32_t max_size = 0, size = 0, nb_sectors_to_erase;

	/* Initialize QuadSPI ------------------------------------------------------ */
	QSPIHandle.Instance = QUADSPI;
	HAL_QSPI_DeInit(&QSPIHandle);

	QSPIHandle.Init.ClockPrescaler     = 1;
	QSPIHandle.Init.FifoThreshold      = 4;
	QSPIHandle.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
	QSPIHandle.Init.FlashSize          = QSPI_FLASH_SIZE;
	QSPIHandle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
	QSPIHandle.Init.ClockMode          = QSPI_CLOCK_MODE_0;
	QSPIHandle.Init.FlashID            = QSPI_FLASH_ID_1;
	QSPIHandle.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;

	if (HAL_QSPI_Init(&QSPIHandle) != HAL_OK)
	{
		Error_Handler();
	}

	sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

	max_size = (uint32_t)((uint8_t *)(&_qspi_init_length));
	nb_sectors_to_erase = (max_size / QSPI_SECTOR_SIZE) + 1;

	step = 4;
	while (step != 6) {

		switch(step)
		{
		  case 0:
			CmdCplt = 0;

			/* Enable write operations ------------------------------------------- */
			QSPI_WriteEnable(&QSPIHandle);

			/* Erasing Sequence -------------------------------------------------- */
			sCommand.Instruction = SUBSECTOR_ERASE_CMD;
			sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
			sCommand.Address     = qspi_addr;
			sCommand.DataMode    = QSPI_DATA_NONE;
			sCommand.DummyCycles = 0;

			if (HAL_QSPI_Command_IT(&QSPIHandle, &sCommand) != HAL_OK)
			{
			  Error_Handler();
			}

			step++;
			break;

		  case 1:
	//    	CmdCplt = 1;
			if(CmdCplt != 0)
			{
			  CmdCplt = 0;
			  StatusMatch = 0;

			  /* Configure automatic polling mode to wait for end of erase ------- */
			  QSPI_AutoPollingMemReady(&QSPIHandle);

			  nb_sectors_to_erase --;
			  if(nb_sectors_to_erase != 0)
			  {
				qspi_addr += QSPI_SECTOR_SIZE;
			  }
			  else
			  {
			  /* Initialize the variables for the data writing ------------------- */
				qspi_addr = 0;
	#if defined(__CC_ARM)
				flash_addr = (uint8_t *)(&Load$$QSPI$$Base);
	#elif defined(__ICCARM__)
			  flash_addr = (uint8_t *)(__section_begin(".qspi_init"));
	#elif defined(__GNUC__)
				flash_addr = (uint8_t *)(&_qspi_init_base);
	#endif

			  /* Copy only one page if the section is bigger */
			  if (max_size > QSPI_PAGE_SIZE)
			  {
				size = QSPI_PAGE_SIZE;
			  }
			  else
			  {
				size = max_size;
			  }
			  }
			  step++;
			}
			break;

		  case 2:
			if(StatusMatch != 0)
			{
			  StatusMatch = 0;
			  TxCplt = 0;

			  if(nb_sectors_to_erase != 0)
			  {
				step = 0;
			  }
			  else
			  {
				/* Enable write operations ----------------------------------------- */
				QSPI_WriteEnable(&QSPIHandle);

				/* Writing Sequence ------------------------------------------------ */
				sCommand.Instruction = QUAD_IN_FAST_PROG_CMD;
				sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
				sCommand.Address     = qspi_addr;
				sCommand.DataMode    = QSPI_DATA_4_LINES;
				sCommand.NbData      = size;

				if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
				{
				  Error_Handler();
				}

				if (HAL_QSPI_Transmit_DMA(&QSPIHandle, flash_addr) != HAL_OK)
				{
				  Error_Handler();
				}

				step++;
			  }
			}
			break;

		  case 3:
			if(TxCplt != 0)
			{
			  TxCplt = 0;
			  StatusMatch = 0;

			  /* Configure automatic polling mode to wait for end of program ----- */
			  QSPI_AutoPollingMemReady(&QSPIHandle);

			  step++;
			}
			break;

		  case 4:
//			if(StatusMatch != 0)
//			{
//			  qspi_addr += size;
//			  flash_addr += size;
//
//			  /* Check if a new page writing is needed */
//			  if (qspi_addr < max_size)
//			  {
//				/* Update the remaining size if it is less than the page size */
//				if ((qspi_addr + size) > max_size)
//				{
//				  size = max_size - qspi_addr;
//				}
//				step = 2;
//			  }
//			  else
//			  {
//				StatusMatch = 0;
//				RxCplt = 0;

	            sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
	            sCommand.Address     = 0;
	            sCommand.DataMode    = QSPI_DATA_4_LINES;
	            sCommand.NbData      = 0;

				/* Configure Volatile Configuration register (with new dummy cycles) */
				QSPI_DummyCyclesCfg(&QSPIHandle);

				/* Reading Sequence ------------------------------------------------ */
				sCommand.Instruction = QUAD_OUT_FAST_READ_CMD;
				sCommand.DummyCycles = DUMMY_CLOCK_CYCLES_READ_QUAD;

				sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

				if (HAL_QSPI_MemoryMapped(&QSPIHandle, &sCommand, &sMemMappedCfg) != HAL_OK)
				{
				  Error_Handler();
				}

				step++;
				step = 6;

//				for (int i=0; i<5; i++) {
//					read_arena[i] = tensor_arena2[i];
//				}
//			  }
//			}
			break;

		  case 5:
			  /* Execute the code from QSPI memory ------------------------------- */
			  GpioToggle();
			break;

		  case 6:
			break;

		  default :
			Error_Handler();
		}
	}
}

/**
  * @brief  This function send a Write Enable and wait it is effective.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static void QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Enable write operations ------------------------------------------ */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = WRITE_ENABLE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure automatic polling mode to wait for write enabling ---- */
  sConfig.Match           = 0x02;
  sConfig.Mask            = 0x02;
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  sCommand.Instruction    = READ_STATUS_REG_CMD;
  sCommand.DataMode       = QSPI_DATA_1_LINE;

  if (HAL_QSPI_AutoPolling(&QSPIHandle, &sCommand, &sConfig, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function read the SR of the memory and wait the EOP.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static void QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef     sCommand;
  QSPI_AutoPollingTypeDef sConfig;

  /* Configure automatic polling mode to wait for memory ready ------ */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_STATUS_REG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

  sConfig.Match           = 0x00;
  sConfig.Mask            = 0x01;
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_QSPI_AutoPolling_IT(&QSPIHandle, &sCommand, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function configure the dummy cycles on memory side.
  * @param  hqspi: QSPI handle
  * @retval None
  */
static void QSPI_DummyCyclesCfg(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef sCommand;
  uint8_t reg;

  /* Read Volatile Configuration register --------------------------- */
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_VOL_CFG_REG_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;
  sCommand.DdrMode           = QSPI_DDR_MODE_DISABLE;
  sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
  sCommand.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
  sCommand.NbData            = 1;

  if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_QSPI_Receive(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  /* Enable write operations ---------------------------------------- */
  QSPI_WriteEnable(&QSPIHandle);

  /* Write Volatile Configuration register (with new dummy cycles) -- */
  sCommand.Instruction = WRITE_VOL_CFG_REG_CMD;
  MODIFY_REG(reg, 0xF0, (DUMMY_CLOCK_CYCLES_READ_QUAD << POSITION_VAL(0xF0)));

  if (HAL_QSPI_Command(&QSPIHandle, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_QSPI_Transmit(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  Command completed callbacks.
  * @param  hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_CmdCpltCallback(QSPI_HandleTypeDef *hqspi)
{
  CmdCplt++;
}

/**
  * @brief  Rx Transfer completed callbacks.
  * @param  hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
  RxCplt++;
}

/**
  * @brief  Tx Transfer completed callbacks.
  * @param  hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
  TxCplt++;
}

/**
  * @brief  Status Match callbacks
  * @param  hqspi: QSPI handle
  * @retval None
  */
void HAL_QSPI_StatusMatchCallback(QSPI_HandleTypeDef *hqspi)
{
  StatusMatch++;
}

/**
  * @brief  QSPI error callbacks.
  * @param  hqspi: QSPI handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
 void HAL_QSPI_ErrorCallback(QSPI_HandleTypeDef *hqspi)
{
  /* Turn LED3 on: Transfer error in reception/transmission process */
//  BSP_LED_On(LED3);
}

/**
  * @brief  DMA conversion complete callback
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */

static void GpioToggle(void)
{
//BSP_LED_Toggle(LED1);
///* Insert delay 100 ms */
//HAL_Delay(100);
//BSP_LED_Toggle(LED2);
///* Insert delay 100 ms */
//HAL_Delay(100);
//BSP_LED_Toggle(LED3);
///* Insert delay 100 ms */
//HAL_Delay(100);
//BSP_LED_Toggle(LED4);
///* Insert delay 100 ms */
//HAL_Delay(100);
}

void TransferComplete(DMA_HandleTypeDef *dmaHandle)
{
  char uart_buf[50];
  int uart_buf_len;

  uint8_t * prev_dst = DMAQue[0].copyReq[DMAQue[0].readIdx].dst;
  uint32_t prev_size = DMAQue[0].copyReq[DMAQue[0].readIdx].size;

//  SCB_InvalidateDCache_by_Addr( (uint32_t*) ( ( (uint32_t)prev_dst ) & ~(uint32_t)0x1F), prev_size + 32 );

  DMAQue[0].readIdx++;
  if (DMAQue[0].readIdx == DMA_COPY_QUE_SIZE)
  	DMAQue[0].readIdx = 0;

  transferCompleteDetected++;
  if (DMAQue[0].readIdx != DMAQue[0].writeIdx) { // Que not empty condition
	uint8_t * src = DMAQue[0].copyReq[DMAQue[0].readIdx].src;
	uint8_t * dst = DMAQue[0].copyReq[DMAQue[0].readIdx].dst;
	uint32_t size = DMAQue[0].copyReq[DMAQue[0].readIdx].size;

//	SCB_CleanDCache_by_Addr( (uint32_t*) ( ( (uint32_t)src ) & ~(uint32_t)0x1F), size + 32 );

//	uart_buf_len = sprintf(uart_buf, "Transfer Size: %u\n", size);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	//	DMAQue.readIdx++;
//	if (DMAQue.readIdx == DMA_COPY_QUE_SIZE)
//		DMAQue.readIdx = 0;
//
//    SCB_InvalidateDCache_by_Addr( (uint32_t*) ( ( (uint32_t)dst ) & ~(uint32_t)0x1F), size + 32 );
	if (HAL_DMA_Start_IT(dmaHandle, (uint32_t)src, (uint32_t)dst, size) != HAL_OK)
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

/**
  * @brief  DMA conversion error callback
  * @note   This function is executed when the transfer error interrupt
  *         is generated during DMA transfer
  * @retval None
  */
void TransferError(DMA_HandleTypeDef *DmaHandle)
{
  transferErrorDetected = 1;
}

/**
  * @brief  DMA conversion complete callback
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
void TransferComplete_Strm1_Ch0(DMA_HandleTypeDef *dmaHandle)
{
  char uart_buf[50];
  int uart_buf_len;

  uint8_t * prev_dst = DMAQue[1].copyReq[DMAQue[1].readIdx].dst;
  uint32_t prev_size = DMAQue[1].copyReq[DMAQue[1].readIdx].size;

//  SCB_InvalidateDCache_by_Addr( (uint32_t*) ( ( (uint32_t)prev_dst ) & ~(uint32_t)0x1F), prev_size + 32 );

  DMAQue[1].readIdx++;
  if (DMAQue[1].readIdx == DMA_COPY_QUE_SIZE)
  	DMAQue[1].readIdx = 0;

  transferCompleteDetected++;
  if (DMAQue[1].readIdx != DMAQue[1].writeIdx) { // Que not empty condition
	uint8_t * src = DMAQue[1].copyReq[DMAQue[1].readIdx].src;
	uint8_t * dst = DMAQue[1].copyReq[DMAQue[1].readIdx].dst;
	uint32_t size = DMAQue[1].copyReq[DMAQue[1].readIdx].size;

//	SCB_CleanDCache_by_Addr( (uint32_t*) ( ( (uint32_t)src ) & ~(uint32_t)0x1F), size + 32 );

//	uart_buf_len = sprintf(uart_buf, "Transfer Size: %u\n", size);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	//	DMAQue.readIdx++;
//	if (DMAQue.readIdx == DMA_COPY_QUE_SIZE)
//		DMAQue.readIdx = 0;
//
	if (HAL_DMA_Start_IT(dmaHandle, (uint32_t)src, (uint32_t)dst, size) != HAL_OK)
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

/**
  * @brief  DMA conversion error callback
  * @note   This function is executed when the transfer error interrupt
  *         is generated during DMA transfer
  * @retval None
  */
void TransferError_Strm1_Ch0(DMA_HandleTypeDef *DmaHandle)
{
  transferErrorDetected = 1;
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
