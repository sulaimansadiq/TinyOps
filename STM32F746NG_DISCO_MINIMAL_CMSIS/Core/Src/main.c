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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "overlay_base.h"
#include "dmacopying.h"
#include <stdio.h>
#include "tensorflow/lite/micro/examples/hello_world/model.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
static __IO uint32_t transferErrorDetected;    /* Set to 1 if an error transfer is detected */
static __IO uint32_t transferCompleteDetected; /* Set to 1 if transfer is correctly completed */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

uint8_t workBuffer[_MAX_SS];

SD_HandleTypeDef hsd1;
DMA_HandleTypeDef hdma_sdmmc1_rx;
DMA_HandleTypeDef hdma_sdmmc1_tx;
TIM_HandleTypeDef htim10;

UART_HandleTypeDef huart1;

SDRAM_HandleTypeDef hsdram1;

/* USER CODE BEGIN PV */
volatile DMACopyQue DMAQue[NUM_DMA_STREAMS_USED];
DMA_HandleTypeDef DmaHandle[NUM_DMA_STREAMS_USED];

FMC_SDRAM_CommandTypeDef command;
QSPI_HandleTypeDef QSPIHandle;
//volatile uint8_t __attribute__((section (".qspi_data"))) tensor_arena2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

#ifdef BENCHMARK_TINYOPS_STATS
TinyOpsBuffSizes tinyOpsBuffSizes;
uint32_t tinyOpsPartFactor;
#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_GPIO_Init2(void);
static void MX_DMA_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM10_Init(void);
static void MX_FMC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @brief  Perform the SDRAM external memory initialization sequence
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
                     SDRAM_MODEREG_CAS_LATENCY_2           |
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
  hsdram->Instance->SDRTR |= ((uint32_t)((1292)<< 1));

}

static void MPU_Config (void) {
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes for SDRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16MB; //MPU_REGION_SIZE_8MB; //MPU_REGION_SIZE_4MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
//  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  This function sends a Write Enable and waits until it is effective.
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
  * @brief  This function configures the dummy cycles on memory side.
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
  sCommand.SIOOMode         = QSPI_SIOO_INST_EVERY_CMD;
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

void QPSI_Init_() {

	QSPI_CommandTypeDef      sCommand;
	QSPI_MemoryMappedTypeDef sMemMappedCfg;

	QSPIHandle.Instance = QUADSPI;
	HAL_QSPI_DeInit(&QSPIHandle);

	/* ClockPrescaler set to 2, so QSPI clock = 216MHz / (2+1) = 72MHz */
	QSPIHandle.Init.ClockPrescaler     = 1;
	QSPIHandle.Init.FifoThreshold      = 4;
	QSPIHandle.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
	QSPIHandle.Init.FlashSize          = POSITION_VAL(0x1000000) - 1;
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

//	flash_addr = 0;
//	size = 0 ;

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

}

//#define BUFFER_SIZE 32
//static uint32_t __attribute__((section (".sdram_bss"))) aSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss"))) aDST_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss"))) bSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss"))) bDST_Buffer[BUFFER_SIZE];

//static uint32_t __attribute__((section (".sdram_bss")))  aSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  aDST_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  bSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  bDST_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  cSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  cDST_Buffer[BUFFER_SIZE];

//static uint8_t aSRC_Const_Buffer[BUFFER_SIZE];
//static uint8_t aDST_Buffer[BUFFER_SIZE];
//static uint8_t bSRC_Const_Buffer[BUFFER_SIZE];
//static uint8_t bDST_Buffer[BUFFER_SIZE];
//static uint8_t cSRC_Const_Buffer[BUFFER_SIZE];
//static uint8_t cDST_Buffer[BUFFER_SIZE];

void init(void)
{

    CPU_CACHE_Enable();		// no variables used

	HAL_Init();				// local and global variables used

	SystemClock_Config();

//	DMA_Config();

    MX_GPIO_Init();
	#ifdef DEBUG_PRINTS
    MX_USART1_UART_Init();
	#endif

    MX_FMC_Init();
    QPSI_Init_();

    RCC->APB2ENR   |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->MEMRMP |= SYSCFG_MEMRMP_SWP_FMC_0;
////    SYSCFG->MEMRMP |= SYSCFG_MEMRMP_SWP_FMC_1;


//    MPU_Config();

	FLASH->ACR = FLASH_ACR_ARTEN | FLASH_ACR_LATENCY_5WS;
}

#ifdef MODEL_TENSOR_ARENA_SDRAM
extern uint8_t * tensor_arena;
volatile unsigned long model_size;
#else
extern uint8_t tensor_arena[];
#endif

uint32_t tensor_arena_size;
uint32_t tensor_arena_ptr;

uint8_t * g_model_int_flash = MODEL_INT_FLASH_ADDRESS;
uint16_t im2col_timer = 0;
//extern uint8_t * tensor_arena;

void FLASH_Program_Byte(uint32_t Address, uint8_t Data);
HAL_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout);

uint32_t FirstSector = 0, NbOfSectors = 0;
uint32_t Address = 0, SECTORError = 0;
__IO uint32_t data32 = 0 , MemoryProgramStatus = 0;

/*Variable used for Erase procedure*/
static FLASH_EraseInitTypeDef EraseInitStruct;

static uint32_t GetSector(uint32_t Address);

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  FRESULT res;                                          /* FatFs function common result code */
  uint32_t byteswritten, bytesread;                     /* File write/read counts */
  uint8_t wtext[] = "This is STM32 working with FatFs!"; /* File write buffer */
  uint8_t rtext[100];                                   /* File read buffer */

  char file_line[250];
  char uart_buf[250];
  int uart_buf_len;
  uint16_t timer_val;
  unsigned int mem_usage;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
//  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
//  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init2();
  /* USER CODE BEGIN 2 */

//  HAL_TIM_Base_Start(&htim10);

  #ifdef OVERLAY_FW
  MX_DMA_Init();
  DMA_Config();
  #endif

  DMAQue[0].dmaHandle	=	&DmaHandle[0];
  DMAQue[0].readIdx		=	0;
  DMAQue[0].writeIdx	=	0;

  DMAQue[1].dmaHandle	=	&DmaHandle[1];
  DMAQue[1].readIdx		=	0;
  DMAQue[1].writeIdx	=	0;

  uart_buf_len = sprintf(uart_buf, "Starting setup ... \r\n");
  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

  setup();

  uart_buf_len = sprintf(uart_buf, "Finished setup ... \r\n");
  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {

  	uint16_t timer_val;
  	unsigned char * out_ptr;
  	unsigned int * mem_usage;
  	unsigned int * tensor_arena_size;
  	unsigned int * tensor_arena_ptr;

	timer_val = loop(out_ptr, mem_usage, tensor_arena_size, tensor_arena_ptr);

//	if (uart_buf_len)
//		while(1);
	/* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

void DMA_Config(void)
{

  char uart_buf[50];
  int uart_buf_len;
  uint16_t timer_val;

  /*## -1- Enable DMA2 clock #################################################*/
  __HAL_RCC_DMA2_CLK_ENABLE();

  timer_val = __HAL_TIM_GET_COUNTER(&htim10);

  /*##-2- Select the DMA Stream 0, Channel 0 functional Parameters ###########*/
  DmaHandle[0].Init.Channel = DMA_CHANNEL_0;					/* DMA_CHANNEL_0                    */
  DmaHandle[0].Init.Direction = DMA_MEMORY_TO_MEMORY;			/* M2M transfer mode                */
  DmaHandle[0].Init.PeriphInc = DMA_PINC_ENABLE;				/* Peripheral increment mode Enable */
  DmaHandle[0].Init.MemInc = DMA_MINC_ENABLE;					/* Memory increment mode Enable     */
  DmaHandle[0].Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;	//DMA_PDATAALIGN_WORD;	/* Peripheral data alignment : Word */
  DmaHandle[0].Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;	//DMA_MDATAALIGN_WORD;	/* memory data alignment : Word     */
  DmaHandle[0].Init.Mode = DMA_NORMAL;							/* Normal DMA mode                  */
  DmaHandle[0].Init.Priority = DMA_PRIORITY_HIGH;				/* priority level : high            */
  DmaHandle[0].Init.FIFOMode = DMA_FIFOMODE_ENABLE;				/* FIFO mode enabled                */
  DmaHandle[0].Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL; /* FIFO threshold: 1/4 full   */
  DmaHandle[0].Init.MemBurst = DMA_MBURST_SINGLE;				/* Memory burst                     */
  DmaHandle[0].Init.PeriphBurst = DMA_PBURST_SINGLE;				/* Peripheral burst                 */
//  DmaHandle[0].Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;	/* FIFO threshold: 1/4 full			*/
//  DmaHandle[0].Init.MemBurst = DMA_MBURST_INC16;				/* Memory burst                     */
//  DmaHandle[0].Init.PeriphBurst = DMA_PBURST_INC16;				/* Peripheral burst                 */

  /*##-3- Select the DMA instance to be used for the transfer : DMA2_Stream0 #*/
  DmaHandle[0].Instance = DMA2_Stream0;

  /*##-4- Initialize the DMA stream ##########################################*/
  if (HAL_DMA_Init(&DmaHandle[0]) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /*##-5- Select Callbacks functions called after Transfer complete and Transfer error */
  HAL_DMA_RegisterCallback(&DmaHandle[0], HAL_DMA_XFER_CPLT_CB_ID, TransferComplete);
  HAL_DMA_RegisterCallback(&DmaHandle[0], HAL_DMA_XFER_ERROR_CB_ID, TransferError);

  /*##-6- Configure NVIC for DMA transfer complete/error interrupts ##########*/
  /* Set Interrupt Group Priority */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 15, 15);

  /* Enable the DMA STREAM global Interrupt */
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

  /*##-2- Select the DMA Stream 1, Channel 0 functional Parameters ###########*/
  DmaHandle[1].Init.Channel = DMA_CHANNEL_0;					/* DMA_CHANNEL_0                    */
  DmaHandle[1].Init.Direction = DMA_MEMORY_TO_MEMORY;			/* M2M transfer mode                */
  DmaHandle[1].Init.PeriphInc = DMA_PINC_ENABLE;				/* Peripheral increment mode Enable */
  DmaHandle[1].Init.MemInc = DMA_MINC_ENABLE;					/* Memory increment mode Enable     */
  DmaHandle[1].Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;	//DMA_PDATAALIGN_WORD;	/* Peripheral data alignment : Word */
  DmaHandle[1].Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;	//DMA_MDATAALIGN_WORD;	/* memory data alignment : Word     */
  DmaHandle[1].Init.Mode = DMA_NORMAL;							/* Normal DMA mode                  */
  DmaHandle[1].Init.Priority = DMA_PRIORITY_HIGH;				/* priority level : high            */
  DmaHandle[1].Init.FIFOMode = DMA_FIFOMODE_ENABLE;				/* FIFO mode enabled                */
  DmaHandle[1].Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL; /* FIFO threshold: 1/4 full   */
  DmaHandle[1].Init.MemBurst = DMA_MBURST_SINGLE;				/* Memory burst                     */
  DmaHandle[1].Init.PeriphBurst = DMA_PBURST_SINGLE;				/* Peripheral burst                 */
//  DmaHandle[1].Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;	/* FIFO threshold: 1/4 full			*/
//  DmaHandle[1].Init.MemBurst = DMA_MBURST_INC16;				/* Memory burst                     */
//  DmaHandle[1].Init.PeriphBurst = DMA_PBURST_INC16;				/* Peripheral burst                 */

  /*##-3- Select the DMA instance to be used for the transfer : DMA2_Stream0 #*/
  DmaHandle[1].Instance = DMA2_Stream1;

  /*##-4- Initialize the DMA stream ##########################################*/
  if (HAL_DMA_Init( &DmaHandle[1] ) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /*##-5- Select Callbacks functions called after Transfer complete and Transfer error */
  HAL_DMA_RegisterCallback(&DmaHandle[1], HAL_DMA_XFER_CPLT_CB_ID, TransferComplete_Strm1_Ch0);
  HAL_DMA_RegisterCallback(&DmaHandle[1], HAL_DMA_XFER_ERROR_CB_ID, TransferError_Strm1_Ch0);

  /*##-6- Configure NVIC for DMA transfer complete/error interrupts ##########*/
  /* Set Interrupt Group Priority */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 15, 15);

  /* Enable the DMA STREAM global Interrupt */
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

//  HAL_DMA_FULL_TRANSFER
}

/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base address of Sector 0, 32 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08008000) /* Base address of Sector 1, 32 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08010000) /* Base address of Sector 2, 32 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x08018000) /* Base address of Sector 3, 32 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08020000) /* Base address of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08040000) /* Base address of Sector 5, 256 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08080000) /* Base address of Sector 6, 256 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x080C0000) /* Base address of Sector 7, 256 Kbytes */

/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;
  }
  else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7) */
  {
    sector = FLASH_SECTOR_7;
  }
  return sector;
}

/**
  * @brief  DMA conversion complete callback
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
void TransferComplete(DMA_HandleTypeDef *dmaHandle)
{
  char uart_buf[50];
  int uart_buf_len;

  uint8_t * prev_dst = DMAQue[0].copyReq[DMAQue[0].readIdx].dst;
  uint32_t prev_size = DMAQue[0].copyReq[DMAQue[0].readIdx].size;

  SCB_InvalidateDCache_by_Addr( (uint32_t*) ( ( (uint32_t)prev_dst ) & ~(uint32_t)0x1F), prev_size + 32 );

  DMAQue[0].readIdx++;
  if (DMAQue[0].readIdx == DMA_COPY_QUE_SIZE)
  	DMAQue[0].readIdx = 0;

  transferCompleteDetected++;
  if (DMAQue[0].readIdx != DMAQue[0].writeIdx) { // Que not empty condition
	uint8_t * src = DMAQue[0].copyReq[DMAQue[0].readIdx].src;
	uint8_t * dst = DMAQue[0].copyReq[DMAQue[0].readIdx].dst;
	uint32_t size = DMAQue[0].copyReq[DMAQue[0].readIdx].size;

	SCB_CleanDCache_by_Addr( (uint32_t*) ( ( (uint32_t)src ) & ~(uint32_t)0x1F), size + 32 );

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

  SCB_InvalidateDCache_by_Addr( (uint32_t*) ( ( (uint32_t)prev_dst ) & ~(uint32_t)0x1F), prev_size + 32 );

  DMAQue[1].readIdx++;
  if (DMAQue[1].readIdx == DMA_COPY_QUE_SIZE)
  	DMAQue[1].readIdx = 0;

  transferCompleteDetected++;
  if (DMAQue[1].readIdx != DMAQue[1].writeIdx) { // Que not empty condition
	uint8_t * src = DMAQue[1].copyReq[DMAQue[1].readIdx].src;
	uint8_t * dst = DMAQue[1].copyReq[DMAQue[1].readIdx].dst;
	uint32_t size = DMAQue[1].copyReq[DMAQue[1].readIdx].size;

	SCB_CleanDCache_by_Addr( (uint32_t*) ( ( (uint32_t)src ) & ~(uint32_t)0x1F), size + 32 );

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

/**
  * @brief System Clock Configuration
  * @retval None
  */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

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
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 432;//400;//432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 0;
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  /* DMA2_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init2(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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
  htim10.Init.Prescaler = 21600-1;
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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK1;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_2;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 6;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 6;
  SdramTiming.WriteRecoveryTime = 2;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* Program the SDRAM external device */
  BSP_SDRAM_Initialization_Sequence(&hsdram1, &command);

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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