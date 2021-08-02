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
     * Reset all GPIOs.
     */
    ms_bsp_gpios_reset();

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
 * @brief Get bsp version.
 *
 * @return Bsp version
 */
ms_uint32_t ms_bsp_version(void)
{
    return MS_RTOS_VERSION_MAKE(1, 0, 0);
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

#if (MS_CFG_KERN_TICK_LESS_EN > 0) || (MS_CFG_KERN_PM_EN > 0)
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


#if MS_CFG_KERN_PM_EN > 0U && BSP_CFG_RTC_EN > 0

#include "driver/stm32_drv_rtc.h"

static ms_rtc_time_t  time_enter_sleep;
static ms_rtc_time_t  time_exit_sleep;
static ms_tick_t      pm_tick_elapsed;
static ms_tick_t      is_wakeup_by_rtc;

/**
 * @brief RTC wake up timer callback.
 *
 * @return N/A
 */
static void __rtc_wakeup_timer_callback(ms_ptr_t arg)
{
    is_wakeup_by_rtc = MS_TRUE;

    stm32_rtc_wakeup_timer_reset();
}

/**
 * @brief Calculate elapsed seconds.
 *
 * @return N/A
 */
static ms_uint32_t __rtc_seconds_elapsed(ms_rtc_time_t *enter_time, ms_rtc_time_t *exit_time)
{
    ms_uint32_t enter_stamp, exit_stamp;

    enter_stamp = (enter_time->hour * 3600) + (enter_time->minute * 60) + enter_time->second;
    exit_stamp  = (exit_time->hour * 3600) + (exit_time->minute * 60) + exit_time->second;

    return (exit_stamp - enter_stamp);
}

/**
 * @brief Start wake up timer.
 *
 * @notice Called after '__ms_io_dev_pm_suspend'
 *
 * @param[in] tick    Expected sleep tick
 *
 * @return N/A
 */
void ms_bsp_pm_timer_start(ms_tick_t tick)
{
    ms_uint32_t stop_seconds = (tick + MS_CFG_KERN_TICK_HZ - 1) / MS_CFG_KERN_TICK_HZ;

    stop_seconds = (stop_seconds == 0) ? 1 : stop_seconds;

    pm_tick_elapsed  = stop_seconds * MS_CFG_KERN_TICK_HZ;
    is_wakeup_by_rtc = MS_FALSE;

    /*
     * The RTC will generate a wake up interrupt after stop_seconds seconds.
     * If the sleep time is greater than the timer's maximum timeout time, then sleep maximum time.
     */
    stm32_rtc_set_wakeup_timer(stop_seconds, (ms_callback_t)__rtc_wakeup_timer_callback, MS_NULL);
}

/**
 * @brief Enter sleep mode.
 *
 * @param[in] sleep_mode    System power sleep mode.
 * @param[in] sr            CPU interrupt enable register val.
 *
 * @return N/A
 */
void ms_bsp_pm_sleep(ms_pm_sleep_mode_t sleep_mode, ms_arch_sr_t sr)
{
    switch (sleep_mode) {
    case MS_PM_SLEEP_MODE_DEEP:

        ms_arch_int_resume(sr);

        /*
         * Get time before sleep
         */
        stm32_rtc_get_time(&time_enter_sleep);

        /* FLASH Deep Power Down Mode enabled */
        HAL_PWREx_EnableFlashPowerDown();

        /* Suspend Tick increment to prevent wakeup by Systick interrupt. */
        /* Otherwise the Systick interrupt will wake up the device within 1ms (HAL time base) */
        HAL_SuspendTick();

        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

        /* Resume Tick interrupt if disabled prior to sleep mode entry */
        HAL_ResumeTick();

        /* Configures system clock after wake-up from STOP: enable HSE, PLL and select */
        /* PLL as system clock source (HSE and PLL are disabled in STOP mode) */
        SystemClock_Config();

        /*
         * Determines whether it is awakened by RTC or some other interrupt
         */
        if (is_wakeup_by_rtc != MS_TRUE) {
            /*
             * Get time after sleep
             */
            stm32_rtc_get_time(&time_exit_sleep);

            /*
             * Fix 'pm_tick_elapsed' value
             */
            pm_tick_elapsed = __rtc_seconds_elapsed(&time_enter_sleep, &time_enter_sleep);

            /*
             * Reset RTC wakeup timer
             */
            stm32_rtc_wakeup_timer_reset();
        }
        break;

    case MS_PM_SLEEP_MODE_SHUTDOWN:
    case MS_PM_SLEEP_MODE_STANDBY:
        /* Enter Standby Mode */
        ms_arch_int_resume(sr);

        HAL_PWR_EnterSTANDBYMode();

        /* System will reset after wakeup from standby mode. */
        break;

    default:
        ms_arch_int_resume(sr);
        MS_ARCH_NOP();
        break;
    }
}

/**
 * @brief Stop wake up timer.
 *
 * @return N/A
 */
void ms_bsp_pm_timer_stop(void)
{
    stm32_rtc_wakeup_timer_reset();
}

/**
 * @brief Early/Later notification of ms_pm_sleep_mode_t
 *
 * @param[in] sleep_mode    System power sleep mode.
 * @param[in] event         'MS_PM_EVENT_SLEEP_ENTER' or 'MS_PM_EVENT_SLEEP_EXIT'
 *
 * @return N/A
 */
void ms_bsp_pm_notify(ms_pm_sleep_mode_t sleep_mode, ms_pm_event_t event)
{
    /*
     * event: MS_PM_EVENT_SLEEP_ENTER
     *        This event will be emit before kernel execute '__ms_io_dev_pm_resume'.
     *
     * event: MS_PM_EVENT_SLEEP_EXIT
     *        This event will be emit after kernel execute '__ms_io_dev_pm_suspend'.
     */
}

/**
 * @brief Tick elapsed while sleep.
 *
 * @return tick elapsed
 */
ms_tick_t ms_bsp_pm_timer_elapsed(void)
{
    return pm_tick_elapsed;
}

/**
 * @brief Adjust system frequency for different ms_pm_run_mode_t.
 *
 * @notice This function will be called before kernel execute '__ms_io_dev_pm_adjust_freq'.
 *
 * @return N/A
 */
void ms_bsp_pm_set_run_mode(ms_pm_run_mode_t run_mode)
{
    int result;
    ms_clk_t sysclk_source;

    result = ms_clk_get_by_name(BSP_CFG_SYSCLK_SRC_NAME, &sysclk_source);
    if (result != 0) {
        return;
    }

    /*
     * An application can call ms_clk_force_freq to set the system frequency,
     * which takes effect immediately if the frequency is raised, or during
     * sleep if the frequency is lowered.
     */
    switch (run_mode) {

    case MS_PM_RUN_MODE_HIGH_SPEED:
    case MS_PM_RUN_MODE_NORMAL_SPEED:
        ms_clk_force_freq(&sysclk_source, BSP_CFG_SYSCLK_FREQ_HIGH);
        break;

    case MS_PM_RUN_MODE_MEDIUM_SPEED:
        ms_clk_force_freq(&sysclk_source, BSP_CFG_SYSCLK_FREQ_MEDIUM);
        break;

    case MS_PM_RUN_MODE_LOW_SPEED:
        ms_clk_force_freq(&sysclk_source, BSP_CFG_SYSCLK_FREQ_LOW);
        break;

    default:
        break;
    }
}
#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
