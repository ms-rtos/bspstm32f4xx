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
** 文   件   名: bsp_lib.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: BSP 函数集
*********************************************************************************************************/
#include "ms_config.h"
#include "ms_rtos.h"
#include "includes.h"
#include <string.h>

static ms_uint32_t systick_count_per_tick;

/*
 * The systick is a 24-bit counter.
 */
#define __ARMV7M_MAX_24_BIT_NUMBER          0xffffffUL

/*
 * A fiddle factor to estimate the number of SysTick counts that would have
 * occurred while the SysTick counter is stopped during tickless idle
 * calculations.
 */
#define __ARMV7M_MISSED_COUNTS_FACTOR       45UL

#define __ARMV7M_SYSTICK_CLOCK_HZ           SystemCoreClock
#define __ARMV7M_CPU_CLOCK_HZ               SystemCoreClock

/**
 * @brief This function provides minimum delay (in milliseconds) based
 *        on variable incremented.
 *
 * @param[in] delay Specifies the delay time length, in milliseconds.
 *
 * @return N/A
 */
void HAL_Delay(uint32_t Delay)
{
    ms_thread_sleep_ms(Delay);
}

/**
 * @brief Provides a tick value in millisecond.
 *
 * @retval Tick value
 */
uint32_t HAL_GetTick(void)
{
    return ms_time_get_ms();
}

/**
 * @brief Initial the bsp unit.
 *
 * @return Error number
 */
ms_err_t ms_bsp_unit_init(void)
{
#if BSP_CFG_USE_BOOTLOADER == 0
    HAL_Init();

    SystemClock_Config();
#else
    SystemCoreClock = BSP_CFG_CPU_HZ;
#endif
    systick_count_per_tick = SystemCoreClock / MS_CFG_KERN_TICK_HZ;

    SysTick_Config(systick_count_per_tick);

    /*
     * Enable UsageFault, BusFault, MemManage faults
     */
    SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk |
                   SCB_SHCSR_BUSFAULTENA_Msk |
                   SCB_SHCSR_MEMFAULTENA_Msk);

    /*
     * Configure the System Control Register to ensure 8-byte stack
     * alignment
     */
    SCB->CCR |= SCB_CCR_STKALIGN_Msk |
                SCB_CCR_DIV_0_TRP_Msk;

    NVIC_SetPriority(SysTick_IRQn, 0x3);

    /*
     * Priority 0xf
     */
    NVIC_SetPriority(SVCall_IRQn, 0xf);
    NVIC_SetPriority(PendSV_IRQn, 0xf);

    /*
     * Follow the architectural requirements
     */
    __DSB();
    __DSB();

    /*
     * Initial printk interface
     */
    ms_bsp_printk_init();

    return MS_ERR_NONE;
}

/**
 * @brief Get device name.
 *
 * @return Device name
 */
const char *ms_bsp_device_name(void)
{
    return "Cortex-M4";
}

/**
 * @brief Get interrupt trace description.
 *
 * @return Interrupt trace description
 */
const char *ms_bsp_int_trace_desc(void)
{
    return "I#15=SysTick";
}

/**
 * @brief Get system Frequency.
 *
 * @return System Frequency
 */
ms_uint32_t ms_bsp_cpu_freq(void)
{
    return SystemCoreClock;
}

/**
 * @brief Get the frequency of the timestamp.
 *
 * @return The frequency of the timestamp
 */
ms_uint32_t ms_bsp_timestamp_freq(void)
{
    return SystemCoreClock;
}

#if MS_CFG_KERN_TICK_LESS_EN > 0U
/**
 * @brief Get tick less parameters.
 *
 * @param[out] max_tick Max time in tick of hardware timer
 * @param[out] min_tick Min time in tick need enter tick less mode
 * @param[out] timer_cnt_per_tick Hardware timer count per tick
 * @param[out] timer_stop_compensation Hardware timer count compensation when timer stoped
 *
 * @return N/A
 */
void ms_bsp_tick_less_param(ms_tick_t *max_tick, ms_tick_t *min_tick, ms_uint32_t *timer_cnt_per_tick, ms_uint32_t *timer_stop_compensation)
{
    *max_tick                = __ARMV7M_MAX_24_BIT_NUMBER / systick_count_per_tick;
    *min_tick                = 5U;
    *timer_cnt_per_tick      = systick_count_per_tick;
    *timer_stop_compensation = __ARMV7M_MISSED_COUNTS_FACTOR / (__ARMV7M_CPU_CLOCK_HZ / __ARMV7M_SYSTICK_CLOCK_HZ);
}
#endif

/**
 * @brief Cortex-M System Tick handler.
 *
 * @return N/A
 */
void SysTick_Handler(void)
{
    (void)ms_int_enter();

    (void)ms_kern_tick(1U);

    (void)ms_int_exit();
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
