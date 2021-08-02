/*********************************************************************************************************
**
**                                北京翼辉信息技术有限公司
**
**                                  微型安全实时操作系统
**
**                                      MS-RTOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: iot_pi_usart.c
**
** 创   建   人: Yu.kangzhi
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi USART 驱动
*********************************************************************************************************/
#include "ms_config.h"
#include "ms_rtos.h"
#include "includes.h"
#include "iot_pi.h"

UART_HandleTypeDef Usart1Handle;
UART_HandleTypeDef Usart2Handle;
UART_HandleTypeDef Usart3Handle;
/**
  * @brief UART MSP Initialization
  * @param channel: UART channel
  * @param channel: UART irq pointer
  * @param channel: UART handler pointer pointer
  * @retval error code; 0, success; other, failed
  */
ms_err_t stm32_uart_get_hw_info(ms_uint8_t channel, IRQn_Type *irq, UART_HandleTypeDef **handler)
{
    if (!irq || !handler) {
        return MS_ERR;
    }

    switch (channel) {
    case 1:
        Usart1Handle.Instance = USART1;
        *irq = USART1_IRQn;
        *handler = &Usart1Handle;
        break;
    case 2:
        Usart2Handle.Instance = USART2;
        *irq = USART2_IRQn;
        *handler = &Usart2Handle;
        break;
    case 3:
        Usart3Handle.Instance = USART3;
        *irq = USART3_IRQn;
        *handler = &Usart3Handle;
        break;
    default:
        return MS_ERR;
    }

    return MS_ERR_NONE;
}

/**
  * @brief UART MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  *           - NVIC configuration for UART interrupt request enable
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_Initure;

    if (huart->Instance == USART1) {

        /* USART1_TX: PA9, USART1_RX: PA10 */

        /* ##-1- Enable peripherals and GPIO Clocks */

        /* Enable GPIO TX/RX clock */
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* Enable USARTx clock */
        __HAL_RCC_USART1_CLK_ENABLE();

        /*##-2- Configure peripheral GPIO */

        /* UART TX RX GPIO pin configuration  */
        GPIO_Initure.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_Initure.Mode      = GPIO_MODE_AF_PP;
        GPIO_Initure.Pull      = GPIO_PULLUP;
        GPIO_Initure.Speed     = GPIO_SPEED_HIGH;
        GPIO_Initure.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_Initure);

    } else if (huart->Instance == USART2) {
        /* USART2_TX: PA2, USART2_RX: PA3 */

        /* ##-1- Enable peripherals and GPIO Clocks */

        /* Enable GPIO TX/RX clock */
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* Enable USARTx clock */
        __HAL_RCC_USART2_CLK_ENABLE();

        /*##-2- Configure peripheral GPIO */

        /* UART TX RX GPIO pin configuration  */
        GPIO_Initure.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_Initure.Mode      = GPIO_MODE_AF_PP;
        GPIO_Initure.Pull      = GPIO_PULLUP;
        GPIO_Initure.Speed     = GPIO_SPEED_HIGH;
        GPIO_Initure.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_Initure);
    } else if (huart->Instance == USART3) {
        /* USART3_TX: PB10, USART3_RX: PC5 */

        /* ##-1- Enable peripherals and GPIO Clocks */

        /* Enable GPIO TX/RX clock */
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        /* Enable USARTx clock */
        __HAL_RCC_USART3_CLK_ENABLE();

        /*##-2- Configure peripheral GPIO */

        /* UART TX RX GPIO pin configuration  */
        GPIO_Initure.Pin       = GPIO_PIN_10;
        GPIO_Initure.Mode      = GPIO_MODE_AF_PP;
        GPIO_Initure.Pull      = GPIO_PULLUP;
        GPIO_Initure.Speed     = GPIO_SPEED_HIGH;
        GPIO_Initure.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOB, &GPIO_Initure);
        GPIO_Initure.Pin       = GPIO_PIN_5;
        GPIO_Initure.Mode      = GPIO_MODE_AF_PP;
        GPIO_Initure.Pull      = GPIO_PULLUP;
        GPIO_Initure.Speed     = GPIO_SPEED_HIGH;
        GPIO_Initure.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOC, &GPIO_Initure);
    } else {
        ms_printk(MS_PK_ERR, "this uart not support now!\n\r");
    }
}

/**
  * @brief UART MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO and NVIC configuration to their default state
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_FORCE_RESET();
        __HAL_RCC_USART1_RELEASE_RESET();

        /* UART TX GPIO pin configuration  */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

    } else if (huart->Instance == USART2) {
        __HAL_RCC_USART2_FORCE_RESET();
        __HAL_RCC_USART2_RELEASE_RESET();

        /* UART TX GPIO pin configuration  */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
    } else if (huart->Instance == USART3) {
        __HAL_RCC_USART3_FORCE_RESET();
        __HAL_RCC_USART3_RELEASE_RESET();

        /* UART TX GPIO pin configuration  */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_5);
    } else {
        ms_printk(MS_PK_ERR, "this uart not support now!\n\r");
    }
}
/*********************************************************************************************************
   END
*********************************************************************************************************/
