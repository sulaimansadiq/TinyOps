/**
  ******************************************************************************
  * @file    stm32469i_discovery.c
  * @author  MCD Application Team
  * @brief   This file provides a set of firmware functions to manage LEDs,
  *          push-buttons, external SDRAM, external QSPI Flash, RF EEPROM,
  *          available on STM32469I-Discovery
  *          board (MB1189) RevA/B/C from STMicroelectronics.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32469i_discovery.h"

/** @defgroup BSP BSP
  * @{
  */

/** @defgroup STM32469I_Discovery STM32469I Discovery
  * @{
  */

/** @defgroup STM32469I_Discovery_LOW_LEVEL STM32469I Discovery LOW LEVEL
  * @{
  */

/** @defgroup STM32469I_Discovery_LOW_LEVEL_Private_TypesDefinitions STM32469I Discovery LOW LEVEL Private TypesDefinitions
  * @{
  */
/**
  * @}
  */

/** @defgroup STM32469I_Discovery_LOW_LEVEL_Private_Defines STM32469I Discovery LOW LEVEL Private Defines
  * @{
  */
/**
 * @brief STM32469I Discovery BSP Driver version number V2.1.0
   */
#define __STM32469I_DISCOVERY_BSP_VERSION_MAIN   (0x02) /*!< [31:24] main version */
#define __STM32469I_DISCOVERY_BSP_VERSION_SUB1   (0x01) /*!< [23:16] sub1 version */
#define __STM32469I_DISCOVERY_BSP_VERSION_SUB2   (0x00) /*!< [15:8]  sub2 version */
#define __STM32469I_DISCOVERY_BSP_VERSION_RC     (0x00) /*!< [7:0]  release candidate */
#define __STM32469I_DISCOVERY_BSP_VERSION        ((__STM32469I_DISCOVERY_BSP_VERSION_MAIN << 24)\
                                                 |(__STM32469I_DISCOVERY_BSP_VERSION_SUB1 << 16)\
                                                 |(__STM32469I_DISCOVERY_BSP_VERSION_SUB2 << 8 )\
                                                 |(__STM32469I_DISCOVERY_BSP_VERSION_RC))
/**
  * @}
  */

/** @defgroup STM32469I_Discovery_LOW_LEVEL_Private_Macros  STM32469I Discovery LOW LEVEL Private Macros
  * @{
  */
/**
  * @}
  */

/** @defgroup STM32469I_Discovery_LOW_LEVEL_Private_Variables STM32469I Discovery LOW LEVEL Private Variables
  * @{
  */
uint32_t GPIO_PIN[LEDn] = {LED1_PIN,
                           LED2_PIN,
                           LED3_PIN,
                           LED4_PIN};

GPIO_TypeDef* GPIO_PORT[LEDn] = {LED1_GPIO_PORT,
                                 LED2_GPIO_PORT,
                                 LED3_GPIO_PORT,
                                 LED4_GPIO_PORT};

GPIO_TypeDef* BUTTON_PORT[BUTTONn] = {WAKEUP_BUTTON_GPIO_PORT };

const uint16_t BUTTON_PIN[BUTTONn] = {WAKEUP_BUTTON_PIN };

const uint16_t BUTTON_IRQn[BUTTONn] = {WAKEUP_BUTTON_EXTI_IRQn };


/** @defgroup STM32469I_Discovery_BSP_Public_Functions STM32469I Discovery BSP Public Functions
  * @{
  */

  /**
  * @brief  This method returns the STM32469I Discovery BSP Driver revision
  * @retval version: 0xXYZR (8bits for each decimal, R for RC)
  */
uint32_t BSP_GetVersion(void)
{
  return __STM32469I_DISCOVERY_BSP_VERSION;
}

/**
  * @brief  Configures LED GPIO.
  * @param  Led: LED to be configured.
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  */
void BSP_LED_Init(Led_TypeDef Led)
{
  GPIO_InitTypeDef  gpio_init_structure;

  if (Led <= LED4)
  {
    /* Configure the GPIO_LED pin */
    gpio_init_structure.Pin   = GPIO_PIN[Led];
    gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull  = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_HIGH;

    switch(Led)
    {
    case LED1 :
      LED1_GPIO_CLK_ENABLE();
      break;
    case LED2 :
      LED2_GPIO_CLK_ENABLE();
      break;
    case LED3 :
      LED3_GPIO_CLK_ENABLE();
      break;
    case LED4 :
      LED4_GPIO_CLK_ENABLE();
      break;
    default :
      break;

    } /* end switch */

    HAL_GPIO_Init(GPIO_PORT[Led], &gpio_init_structure);

    /* By default, turn off LED by setting a high level on corresponding GPIO */
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);

  } /* of if (Led <= LED4) */

}


/**
  * @brief  DeInit LEDs.
  * @param  Led: LED to be configured.
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  * @note Led DeInit does not disable the GPIO clock nor disable the Mfx
  */
void BSP_LED_DeInit(Led_TypeDef Led)
{
  GPIO_InitTypeDef  gpio_init_structure;

  if (Led <= LED4)
  {
    /* DeInit the GPIO_LED pin */
    gpio_init_structure.Pin = GPIO_PIN[Led];

    /* Turn off LED */
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
    HAL_GPIO_DeInit(GPIO_PORT[Led], gpio_init_structure.Pin);
  }

}

/**
  * @brief  Turns selected LED On.
  * @param  Led: LED to be set on
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  */
void BSP_LED_On(Led_TypeDef Led)
{
  if (Led <= LED4)
  {
     HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET);
  }

}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: LED to be set off
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  */
void BSP_LED_Off(Led_TypeDef Led)
{
  if (Led <= LED4)
  {
    HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET);
  }
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: LED to be toggled
  *          This parameter can be one of the following values:
  *            @arg  LED1
  *            @arg  LED2
  *            @arg  LED3
  *            @arg  LED4
  */
void BSP_LED_Toggle(Led_TypeDef Led)
{
  if (Led <= LED4)
  {
     HAL_GPIO_TogglePin(GPIO_PORT[Led], GPIO_PIN[Led]);
  }
}

/**
  * @brief  Configures button GPIO and EXTI Line.
  * @param  Button: Button to be configured
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  *            @arg  BUTTON_USER: User Push Button
  * @param  Button_Mode: Button mode
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_MODE_GPIO: Button will be used as simple IO
  *            @arg  BUTTON_MODE_EXTI: Button will be connected to EXTI line
  *                                    with interrupt generation capability
  */
void BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode)
{
  GPIO_InitTypeDef gpio_init_structure;

  /* Enable the BUTTON clock */
  BUTTON_GPIO_CLK_ENABLE();

  if(Button_Mode == BUTTON_MODE_GPIO)
  {
    /* Configure Button pin as input */
    gpio_init_structure.Pin = BUTTON_PIN[Button];
    gpio_init_structure.Mode = GPIO_MODE_INPUT;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(BUTTON_PORT[Button], &gpio_init_structure);
  }

  if(Button_Mode == BUTTON_MODE_EXTI)
  {
    /* Configure Button pin as input with External interrupt */
    gpio_init_structure.Pin = BUTTON_PIN[Button];
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FAST;

    gpio_init_structure.Mode = GPIO_MODE_IT_RISING;

    HAL_GPIO_Init(BUTTON_PORT[Button], &gpio_init_structure);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority((IRQn_Type)(BUTTON_IRQn[Button]), 0x0F, 0x00);
    HAL_NVIC_EnableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
  }
}

/**
  * @brief  Push Button DeInit.
  * @param  Button: Button to be configured
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  *            @arg  BUTTON_USER: User Push Button
  * @note PB DeInit does not disable the GPIO clock
  */
void BSP_PB_DeInit(Button_TypeDef Button)
{
    GPIO_InitTypeDef gpio_init_structure;

    gpio_init_structure.Pin = BUTTON_PIN[Button];
    HAL_NVIC_DisableIRQ((IRQn_Type)(BUTTON_IRQn[Button]));
    HAL_GPIO_DeInit(BUTTON_PORT[Button], gpio_init_structure.Pin);
}


/**
  * @brief  Returns the selected button state.
  * @param  Button: Button to be checked
  *          This parameter can be one of the following values:
  *            @arg  BUTTON_WAKEUP: Wakeup Push Button
  *            @arg  BUTTON_USER: User Push Button
  * @retval The Button GPIO pin value
  */
uint32_t BSP_PB_GetState(Button_TypeDef Button)
{
  return HAL_GPIO_ReadPin(BUTTON_PORT[Button], BUTTON_PIN[Button]);
}

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
