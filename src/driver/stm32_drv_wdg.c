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
** 文   件   名: stm32_drv_wdg.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片看门狗驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include "includes.h"

#if BSP_CFG_WDG_EN > 0

#if BSP_CFG_WDG_DEADLINE <= 100
#error BSP_CFG_WDG_DEADLINE must > 100
#endif

static void __stm32_wdg_start(void)
{
    IWDG->KR  = IWDG_KEY_WRITE_ACCESS_ENABLE;                       /* 使能对 IWDG->PR 和 IWDG->RLR 的写 */
    IWDG->PR  = IWDG_PRESCALER_64;                                  /* 设置分频系数 */
    IWDG->RLR = 500 * BSP_CFG_WDG_DEADLINE / MS_CFG_KERN_TICK_HZ;   /* 重载寄存器, 最大 0xFFF */
    IWDG->KR  = IWDG_KEY_RELOAD;                                    /* 重载 */
    IWDG->KR  = IWDG_KEY_ENABLE;                                    /* 使能看门狗 */
}

static void __stm32_wdg_feed(void)
{
    IWDG->KR = IWDG_KEY_RELOAD;                                     /* 重载 */
}

static ms_wdg_drv_t __stm32_wdg_drv = {
    .start     = __stm32_wdg_start,
    .feed      = __stm32_wdg_feed,
    .dead_line = BSP_CFG_WDG_DEADLINE / 2,
};

ms_wdg_drv_t *stm32_wdg_drv(void)
{
    return &__stm32_wdg_drv;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
