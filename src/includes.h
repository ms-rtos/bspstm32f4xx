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
** 文   件   名: includes.h
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: 板级相关头文件
*********************************************************************************************************/

#ifndef __BSP_INCLUDES_H
#define __BSP_INCLUDES_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  通用头文件
*********************************************************************************************************/

#include "config.h"

#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_rcc.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_smartcard.h"
#include "stm32f4xx_hal_dma2d.h"

#include "core_cm4.h"

/*********************************************************************************************************
  板级相关头文件
*********************************************************************************************************/

#if defined(IOT_PI_V1)

#include "iot_pi_v1.h"

#elif defined(IOT_PI)

#include "iot_pi.h"

#elif defined(CSG_SMART_METER)

#include "csg_smart_meter.h"

#elif defined(ALIENTEK_APOLLO_F429)

#include "alientek_apollo_f429.h"

#elif defined(STM32F429I_DISCOVERY)

#include "stm32f429i_discovery_includes.h"

#endif

/*********************************************************************************************************
  函数声明
*********************************************************************************************************/

void SystemClock_Config(void);

const ms_flashfs_partition_t *ms_bsp_flash_part_info(void);
ms_uint32_t ms_bsp_flash_data_sector(ms_uint32_t data_sector_id);
ms_uint32_t ms_bsp_flash_addr_to_sector(ms_addr_t addr);

void        ms_bsp_printk_init(void);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __BSP_INCLUDES_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
