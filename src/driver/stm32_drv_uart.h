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
** 文   件   名: stm32_drv_uart.h
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 UART 驱动头文件
*********************************************************************************************************/

#ifndef STM32_DRV_UART_H
#define STM32_DRV_UART_H

#ifdef __cplusplus
extern "C" {
#endif

ms_err_t stm32_uart_drv_register(void);
ms_err_t stm32_uart_dev_create(const char *path, ms_uint8_t channel,
                               ms_uint32_t rx_buf_size, ms_uint32_t tx_buf_size);

ms_err_t stm32_uart_get_hw_info(ms_uint8_t channel, IRQn_Type *irq, UART_HandleTypeDef **handler);
void     stm32_uart_irq_handler(ms_uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  STM32_DRV_UART_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
