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
** 文   件   名: iot_pi_esp8266.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi ESP8266 驱动
*********************************************************************************************************/
#include "config.h"
#include "ms_rtos.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_rcc.h"
#include "esp/esp.h"
#include "esp/esp_mem.h"
#include "esp/esp_input.h"
#include "system/esp_ll.h"

#if BSP_CFG_ESP8266_EN > 0

/* USART */
#define ESP_USART                           USART6
#define ESP_USART_CLK                       LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART6)
#define ESP_USART_IRQ                       USART6_IRQn
#define ESP_USART_IRQHANDLER                USART6_IRQHandler
#define ESP_USART_RDR_NAME                  DR

/* DMA settings */
#define ESP_USART_DMA                       DMA2
#define ESP_USART_DMA_CLK                   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2)
#define ESP_USART_DMA_RX_STREAM             LL_DMA_STREAM_2
#define ESP_USART_DMA_RX_CH                 LL_DMA_CHANNEL_5
#define ESP_USART_DMA_RX_IRQ                DMA2_Stream2_IRQn
#define ESP_USART_DMA_RX_IRQHANDLER         DMA2_Stream2_IRQHandler

/* DMA flags management */
#define ESP_USART_DMA_RX_IS_TC              LL_DMA_IsActiveFlag_TC2(ESP_USART_DMA)
#define ESP_USART_DMA_RX_IS_HT              LL_DMA_IsActiveFlag_HT2(ESP_USART_DMA)
#define ESP_USART_DMA_RX_CLEAR_TC           LL_DMA_ClearFlag_TC2(ESP_USART_DMA)
#define ESP_USART_DMA_RX_CLEAR_HT           LL_DMA_ClearFlag_HT2(ESP_USART_DMA)

/* USART TX PIN */
#define ESP_USART_TX_PORT_CLK               LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA)
#define ESP_USART_TX_PORT                   GPIOA
#define ESP_USART_TX_PIN                    LL_GPIO_PIN_12
#define ESP_USART_TX_PIN_AF                 GPIO_AF8_USART6

/* USART RX PIN */
#define ESP_USART_RX_PORT_CLK               LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA)
#define ESP_USART_RX_PORT                   GPIOA
#define ESP_USART_RX_PIN                    LL_GPIO_PIN_11
#define ESP_USART_RX_PIN_AF                 GPIO_AF8_USART6

/* RESET PIN */
#define ESP_RESET_PORT_CLK                  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA)
#define ESP_RESET_PORT                      GPIOA
#define ESP_RESET_PIN                       LL_GPIO_PIN_15

/* GPIO0 PIN */
#define ESP_GPIO0_PORT_CLK                  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC)
#define ESP_GPIO0_PORT                      GPIOC
#define ESP_GPIO0_PIN                       LL_GPIO_PIN_14

/* CH_PD PIN */
#define ESP_CH_PD_PORT_CLK                  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB)
#define ESP_CH_PD_PORT                      GPIOB
#define ESP_CH_PD_PIN                       LL_GPIO_PIN_5

#if BSP_CFG_ESP8266_UPDATE_FW == 0

/* Include STM32 generic driver */
#include "driver/esp_ll_stm32.c"

#else

/* USART */
#define HOST_USART                          USART1
#define HOST_USART_CLK                      LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1)
#define HOST_USART_IRQ                      USART1_IRQn
#define HOST_USART_IRQHANDLER               USART1_IRQHandler
#define HOST_USART_RDR_NAME                 DR

/* USART TX PIN */
#define HOST_USART_TX_PORT_CLK              LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB)
#define HOST_USART_TX_PORT                  GPIOA
#define HOST_USART_TX_PIN                   LL_GPIO_PIN_9
#define HOST_USART_TX_PIN_AF                GPIO_AF7_USART1

/* USART RX PIN */
#define HOST_USART_RX_PORT_CLK              LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA)
#define HOST_USART_RX_PORT                  GPIOA
#define HOST_USART_RX_PIN                   LL_GPIO_PIN_10
#define HOST_USART_RX_PIN_AF                GPIO_AF7_USART1

void UART4_Config(uint32_t baudrate)
{
    static LL_USART_InitTypeDef usart_init;
    LL_GPIO_InitTypeDef gpio_init;

    /* Enable peripheral clocks */
    ESP_USART_CLK;
    ESP_USART_TX_PORT_CLK;
    ESP_USART_RX_PORT_CLK;

    LL_GPIO_StructInit(&gpio_init);
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_UP;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Mode = LL_GPIO_MODE_OUTPUT;

    /* Configure USART pins */
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;

    /* TX PIN */
    gpio_init.Alternate = ESP_USART_TX_PIN_AF;
    gpio_init.Pin = ESP_USART_TX_PIN;
    LL_GPIO_Init(ESP_USART_TX_PORT, &gpio_init);

    /* RX PIN */
    gpio_init.Alternate = ESP_USART_RX_PIN_AF;
    gpio_init.Pin = ESP_USART_RX_PIN;
    LL_GPIO_Init(ESP_USART_RX_PORT, &gpio_init);

    /* Configure UART */
    LL_USART_DeInit(ESP_USART);
    LL_USART_StructInit(&usart_init);
    usart_init.BaudRate = baudrate;
    usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
    usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    usart_init.OverSampling = LL_USART_OVERSAMPLING_8;
    usart_init.Parity = LL_USART_PARITY_NONE;
    usart_init.StopBits = LL_USART_STOPBITS_1;
    usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
    LL_USART_Init(ESP_USART, &usart_init);

    /* Enable USART interrupts and DMA request */
    LL_USART_EnableIT_RXNE(ESP_USART);


    LL_USART_Enable(ESP_USART);
}

void UART1_Config(uint32_t baudrate)
{
    static LL_USART_InitTypeDef usart_init;
    LL_GPIO_InitTypeDef gpio_init;

    /* Enable peripheral clocks */
    HOST_USART_CLK;
    HOST_USART_TX_PORT_CLK;
    HOST_USART_RX_PORT_CLK;

    LL_GPIO_StructInit(&gpio_init);
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_UP;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Mode = LL_GPIO_MODE_OUTPUT;

    /* Configure USART pins */
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;

    /* TX PIN */
    gpio_init.Alternate = HOST_USART_TX_PIN_AF;
    gpio_init.Pin = HOST_USART_TX_PIN;
    LL_GPIO_Init(HOST_USART_TX_PORT, &gpio_init);

    /* RX PIN */
    gpio_init.Alternate = HOST_USART_RX_PIN_AF;
    gpio_init.Pin = HOST_USART_RX_PIN;
    LL_GPIO_Init(HOST_USART_RX_PORT, &gpio_init);

    /* Configure UART */
    LL_USART_DeInit(HOST_USART);
    LL_USART_StructInit(&usart_init);
    usart_init.BaudRate = baudrate;
    usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
    usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    usart_init.OverSampling = LL_USART_OVERSAMPLING_8;
    usart_init.Parity = LL_USART_PARITY_NONE;
    usart_init.StopBits = LL_USART_STOPBITS_1;
    usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
    LL_USART_Init(HOST_USART, &usart_init);

    /* Enable USART interrupts and DMA request */
    LL_USART_EnableIT_RXNE(HOST_USART);


    LL_USART_Enable(HOST_USART);
}

void esp8266_update_fw_init(void)
{
    LL_GPIO_InitTypeDef gpio_init;

#if defined(ESP_RESET_PIN)
    ESP_RESET_PORT_CLK;
#endif /* defined(ESP_RESET_PIN) */

#if defined(ESP_GPIO0_PIN)
    ESP_GPIO0_PORT_CLK;
#endif /* defined(ESP_GPIO0_PIN) */

#if defined(ESP_GPIO2_PIN)
    ESP_GPIO2_PORT_CLK;
#endif /* defined(ESP_GPIO2_PIN) */

#if defined(ESP_CH_PD_PIN)
    ESP_CH_PD_PORT_CLK;
#endif /* defined(ESP_CH_PD_PIN) */

    /* Global pin configuration */
    LL_GPIO_StructInit(&gpio_init);
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_UP;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Mode = LL_GPIO_MODE_OUTPUT;

#if defined(ESP_RESET_PIN)
    /* Configure RESET pin */
    gpio_init.Pin = ESP_RESET_PIN;
    LL_GPIO_Init(ESP_RESET_PORT, &gpio_init);
#endif /* defined(ESP_RESET_PIN) */

#if defined(ESP_GPIO0_PIN)
    /* Configure GPIO0 pin */
    gpio_init.Pin = ESP_GPIO0_PIN;
    LL_GPIO_Init(ESP_GPIO0_PORT, &gpio_init);
    /*
     * Output low for uart download mode
     */
    LL_GPIO_ResetOutputPin(ESP_GPIO0_PORT, ESP_GPIO0_PIN);

    ms_thread_sleep_ms(100U);
#endif /* defined(ESP_GPIO0_PIN) */

#if defined(ESP_CH_PD_PIN)
    /* Configure CH_PD pin */
    gpio_init.Pin = ESP_CH_PD_PIN;
    LL_GPIO_Init(ESP_CH_PD_PORT, &gpio_init);
    /*
     * Output high for enable esp8266
     */
    LL_GPIO_SetOutputPin(ESP_CH_PD_PORT, ESP_CH_PD_PIN);
#endif /* defined(ESP_CH_PD_PIN) */

    UART1_Config(115200U);
    UART4_Config(115200U);

    /*
     * Output low for reset
     */
    LL_GPIO_ResetOutputPin(ESP_RESET_PORT, ESP_RESET_PIN);
    ms_thread_sleep_ms(200U);
    LL_GPIO_SetOutputPin(ESP_RESET_PORT, ESP_RESET_PIN);

    ms_arch_int_disable();

    while (MS_TRUE) {
        if (ESP_USART->SR & USART_SR_RXNE) {
            LL_USART_TransmitData8(HOST_USART, ESP_USART->DR);
            while (!LL_USART_IsActiveFlag_TXE(HOST_USART)) {}
        }

        if (HOST_USART->SR & USART_SR_RXNE) {
            LL_USART_TransmitData8(ESP_USART, HOST_USART->DR);
            while (!LL_USART_IsActiveFlag_TXE(ESP_USART)) {}
        }
    }
}

espr_t esp_ll_init(esp_ll_t* ll)
{
    return espERR;
}

#endif
#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
