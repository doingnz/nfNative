//
// Copyright (c) .NET Foundation and Contributors
// Portions Copyright (c) Microsoft Corporation.  All rights reserved.
// See LICENSE file in the project root for full license information.
//

#include <stm32h7xx_hal.h>
#include <tx_api.h>
#include <BoardInit.h>
#include <targetHAL.h>

void HAL_MspInit(void)
{
    // Add User Code here
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    // Add user interrupt information here
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  // Coded elsewhere
}

/**
* @brief UART MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART3)
    {
        /* USER CODE BEGIN USART3_MspDeInit 0 */

        /* USER CODE END USART3_MspDeInit 0 */
          /* Peripheral clock disable */
        __HAL_RCC_USART3_CLK_DISABLE();

        /**USART3 GPIO Configuration
        PD8     ------> USART3_TX
        PD9     ------> USART3_RX
        */
        HAL_GPIO_DeInit(GPIOD, wpUSART_RX_PIN | wpUSART_TX_PIN);

        /* USART3 DMA DeInit */
        HAL_DMA_DeInit(huart->hdmarx);

        /* USART3 interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART3_IRQn);
        /* USER CODE BEGIN USART3_MspDeInit 1 */

        /* USER CODE END USART3_MspDeInit 1 */
    }

}


#ifdef HAL_RTC_MODULE_ENABLED
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
    (void)hrtc;

    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    // Configue LSE as RTC clock source
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        while (1)
        {
        }
    }
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        while (1)
        {
        }
    }

    // Enable RTC Clock
    __HAL_RCC_RTC_ENABLE();
}
void HAL_RTC_MspDeInit(RTC_HandleTypeDef* hrtc)
{
    (void)hrtc;

    // Reset peripherals
    __HAL_RCC_RTC_DISABLE();
}

#endif // HAL_RTC_MODULE_ENABLED
void HAL_CRC_MspInit(CRC_HandleTypeDef* hcrc)
{
    (void)hcrc;
    /* CRC Peripheral clock enable */
    __HAL_RCC_CRC_CLK_ENABLE();
}
void HAL_CRC_MspDeInit(CRC_HandleTypeDef* hcrc)
{
    (void)hcrc;
    /* Enable CRC reset state */
    __HAL_RCC_CRC_FORCE_RESET();

    /* Release CRC from reset state */
    __HAL_RCC_CRC_RELEASE_RESET();
}
#ifdef HAL_RNG_MODULE_ENABLED
void HAL_RNG_MspInit(RNG_HandleTypeDef* hrng)
{
    if (hrng->Instance == RNG)
    {
        // Peripheral clock enable
        __HAL_RCC_RNG_CLK_ENABLE();
    }
}

void HAL_RNG_MspDeInit(RNG_HandleTypeDef* hrng)
{
    if (hrng->Instance == RNG)
    {
        // Peripheral clock disable
        __HAL_RCC_RNG_CLK_DISABLE();
    }
}

#endif // HAL_RNG_MODULE_ENABLED
