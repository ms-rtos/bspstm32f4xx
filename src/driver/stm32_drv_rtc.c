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
** 文   件   名: stm32_drv_rtc.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 RTC 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"
#include "driver/stm32_drv_rtc.h"

#if BSP_CFG_RTC_EN > 0

static ms_err_t __stm32_rtc_set_time(ms_ptr_t drv_ctx, const ms_rtc_time_t *rtc_time)
{
    RTC_HandleTypeDef *rtc_handle = drv_ctx;
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;

    /*
     * Set Date
     */
    sdatestructure.Year    = rtc_time->year;
    sdatestructure.Month   = rtc_time->month;
    sdatestructure.Date    = rtc_time->date;
    sdatestructure.WeekDay = rtc_time->weekday;
    HAL_RTC_SetDate(rtc_handle, &sdatestructure, RTC_FORMAT_BIN);

    /*
     * Set Time
     */
    stimestructure.Hours          = rtc_time->hour;
    stimestructure.Minutes        = rtc_time->minute;
    stimestructure.Seconds        = rtc_time->second;
    stimestructure.TimeFormat     = RTC_HOURFORMAT_24;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime(rtc_handle, &stimestructure, RTC_FORMAT_BIN);

    return MS_ERR_NONE;
}

static ms_err_t __stm32_rtc_get_time(ms_ptr_t drv_ctx, ms_rtc_time_t *rtc_time)
{
    RTC_HandleTypeDef *rtc_handle = drv_ctx;
    RTC_DateTypeDef sdatestructureget;
    RTC_TimeTypeDef stimestructureget;

    /*
     * Get the RTC current Time
     */
    HAL_RTC_GetTime(rtc_handle, &stimestructureget, RTC_FORMAT_BIN);

    /*
     * Get the RTC current Date
     */
    HAL_RTC_GetDate(rtc_handle, &sdatestructureget, RTC_FORMAT_BIN);

    rtc_time->year    = sdatestructureget.Year;         /* base 2000 */
    rtc_time->month   = sdatestructureget.Month;        /* MS_RTC_MONTH_xxx */
    rtc_time->date    = sdatestructureget.Date;         /* 1-31 */
    rtc_time->weekday = sdatestructureget.WeekDay;      /* MS_RTC_WEEKDAY_xxx */
    rtc_time->hour    = stimestructureget.Hours;        /* 0-23 */
    rtc_time->minute  = stimestructureget.Minutes;      /* 0-59 */
    rtc_time->second  = stimestructureget.Seconds;      /* 0-59 */

    return MS_ERR_NONE;
}

/**
  * @brief  Configure the current time and date.
  * @param  None
  * @retval None
  */
static void RTC_CalendarConfig(RTC_HandleTypeDef *rtc_handle)
{
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;

    /*
     * Set Date: Tuesday February 18th 2016
     */
    sdatestructure.Year    = 16;
    sdatestructure.Month   = MS_RTC_MONTH_FEBRUARY;
    sdatestructure.Date    = 18;
    sdatestructure.WeekDay = MS_RTC_WEEKDAY_TUESDAY;
    HAL_RTC_SetDate(rtc_handle, &sdatestructure, RTC_FORMAT_BIN);

    /*
     * Set Time: 02:00:00
     */
    stimestructure.Hours          = 2;
    stimestructure.Minutes        = 0;
    stimestructure.Seconds        = 0;
    stimestructure.TimeFormat     = RTC_HOURFORMAT_24;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime(rtc_handle, &stimestructure, RTC_FORMAT_BIN);

    /*
     * Writes a data in a RTC Backup data Register1
     */
    HAL_RTCEx_BKUPWrite(rtc_handle, RTC_BKP_DR1, 0x32F2);
}

static ms_err_t __stm32_rtc_init(ms_ptr_t drv_ctx)
{
    RTC_HandleTypeDef *rtc_handle = drv_ctx;

    /*
     * Configure RTC prescaler and RTC data registers
     */
    /*
     * RTC configured as follows:
     *  - Hour Format    = Format 24
     *  - Asynch Prediv  = Value according to source clock
     *  - Synch Prediv   = Value according to source clock
     *  - OutPut         = Output Disable
     *  - OutPutPolarity = High Polarity
     *  - OutPutType     = Open Drain
     */
    rtc_handle->Instance            = RTC;
    rtc_handle->Init.HourFormat     = RTC_HOURFORMAT_24;
    rtc_handle->Init.AsynchPrediv   = BSP_CFG_RTC_ASYNCH_PREDIV;
    rtc_handle->Init.SynchPrediv    = BSP_CFG_RTC_SYNCH_PREDIV;
    rtc_handle->Init.OutPut         = RTC_OUTPUT_DISABLE;
    rtc_handle->Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    rtc_handle->Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    __HAL_RTC_RESET_HANDLE_STATE(rtc_handle);
    HAL_RTC_Init(rtc_handle);

    /*
     * Read the Back Up Register 1 Data
     */
    if (HAL_RTCEx_BKUPRead(rtc_handle, RTC_BKP_DR1) != 0x32F2) {
        /*
         * Configure RTC Calendar
         */
        RTC_CalendarConfig(rtc_handle);
    } else {
        /*
         * Clear source Reset Flag
         */
        __HAL_RCC_CLEAR_RESET_FLAGS();
    }

    return MS_ERR_NONE;
}

static ms_rtc_drv_t __stm32_rtc_drv = {
        .init     = __stm32_rtc_init,
        .set_time = __stm32_rtc_set_time,
        .get_time = __stm32_rtc_get_time,
};

static RTC_HandleTypeDef __stm32_rtc_handle;

ms_err_t stm32_rtc_dev_create(void)
{
    return ms_rtc_dev_create(MS_RTC_DEV_PATH, &__stm32_rtc_drv, &__stm32_rtc_handle);
}

/*********************************************************************************************************
  RTC 驱动层接口
*********************************************************************************************************/

static ms_callback_t rtc_alarm_callback;
static ms_ptr_t      rtc_alarm_arg;
static ms_callback_t rtc_wakeup_timer_callback;
static ms_ptr_t      rtc_wakeup_timer_arg;

#define RTC_WAKEUP_COUNT_MAX  (0xFFFF)

void stm32_rtc_get_capability(ms_rtc_capability_t *cap)
{
    if (cap == MS_NULL) {
        return;
    }

    cap->capability       = MS_RTC_CAPABILITY_CALENDAR | MS_RTC_CAPABILITY_ALARM | MS_RTC_CAPABILITY_WAKEUP;
    cap->alarm_cap        = MS_RTC_ALARM_CAP_HOUR | MS_RTC_ALARM_CAP_MINUTE | MS_RTC_ALARM_CAP_SECOND;
    cap->wakeup_cap       = MS_RTC_ALARM_CAP_SECOND;
}

ms_err_t stm32_rtc_set_time(const ms_rtc_time_t *rtc_time)
{
    if (rtc_time == MS_NULL) {
        return MS_ERR;
    }

    return __stm32_rtc_set_time(&__stm32_rtc_handle, rtc_time);
}

ms_err_t stm32_rtc_get_time(ms_rtc_time_t *rtc_time)
{
    if (rtc_time == MS_NULL) {
        return MS_ERR;
    }

    return __stm32_rtc_get_time(&__stm32_rtc_handle, rtc_time);
}

ms_err_t stm32_rtc_set_alarm(const ms_rtc_time_t *rtc_time, ms_callback_t callback, ms_ptr_t arg)
{
    RTC_AlarmTypeDef RTC_AlarmSturuct;

    if (rtc_time == MS_NULL) {
        return MS_ERR;
    }

    HAL_RTC_DeactivateAlarm(&__stm32_rtc_handle, RTC_ALARM_A);

    rtc_alarm_callback = callback;
    rtc_alarm_arg = arg;

    RTC_AlarmSturuct.AlarmTime.Hours      = rtc_time->hour;
    RTC_AlarmSturuct.AlarmTime.Minutes    = rtc_time->minute;
    RTC_AlarmSturuct.AlarmTime.Seconds    = rtc_time->second;
    RTC_AlarmSturuct.AlarmTime.SubSeconds = 0;
    RTC_AlarmSturuct.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;

    RTC_AlarmSturuct.AlarmMask            = RTC_ALARMMASK_DATEWEEKDAY;
    RTC_AlarmSturuct.AlarmSubSecondMask   = RTC_ALARMSUBSECONDMASK_NONE;
    RTC_AlarmSturuct.AlarmDateWeekDaySel  = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
    RTC_AlarmSturuct.AlarmDateWeekDay     = RTC_WEEKDAY_MONDAY;
    RTC_AlarmSturuct.Alarm                = RTC_ALARM_A;

    if (HAL_RTC_SetAlarm_IT(&__stm32_rtc_handle, &RTC_AlarmSturuct, RTC_FORMAT_BIN) != HAL_OK) {
        return MS_ERR;
    }

    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0x01, 0x02);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

    return MS_ERR_NONE;
}

ms_err_t stm32_rtc_get_alarm(ms_rtc_time_t *rtc_time)
{
    RTC_AlarmTypeDef RTC_AlarmSturuct;

    if (rtc_time == MS_NULL) {
        return MS_ERR;
    }

    if (HAL_RTC_GetAlarm(&__stm32_rtc_handle, &RTC_AlarmSturuct, RTC_ALARM_A, RTC_FORMAT_BIN) != HAL_OK) {
        return MS_ERR;
    }

    return MS_ERR_NONE;
}

ms_err_t stm32_rtc_set_wakeup_timer(ms_uint32_t sec, ms_callback_t callback, ms_ptr_t arg)
{
    if (sec > RTC_WAKEUP_COUNT_MAX) {
        return MS_ERR;
    }

    HAL_RTCEx_DeactivateWakeUpTimer(&__stm32_rtc_handle);

    rtc_wakeup_timer_callback = callback;
    rtc_wakeup_timer_arg = arg;

    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&__stm32_rtc_handle, RTC_FLAG_WUTF);

    HAL_RTCEx_SetWakeUpTimer_IT(&__stm32_rtc_handle, (sec - 1), RTC_WAKEUPCLOCK_CK_SPRE_16BITS);

    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0x02, 0x02);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

    return MS_ERR_NONE;
}

ms_err_t stm32_rtc_get_wakeup_timer(ms_uint32_t *sec)
{
    if (sec == MS_NULL) {
        return MS_ERR;
    }

    *sec = HAL_RTCEx_GetWakeUpTimer(&__stm32_rtc_handle);

    return MS_ERR_NONE;
}

void stm32_rtc_alarm_reset(void)
{
    HAL_RTC_DeactivateAlarm(&__stm32_rtc_handle, RTC_ALARM_A);

    rtc_alarm_callback = MS_NULL;
}

void stm32_rtc_wakeup_timer_reset(void)
{
    HAL_RTCEx_DeactivateWakeUpTimer(&__stm32_rtc_handle);

    rtc_wakeup_timer_callback = MS_NULL;
}

void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&__stm32_rtc_handle);
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    if (rtc_alarm_callback != MS_NULL) {
        rtc_alarm_callback(rtc_alarm_arg);
    }
}

void RTC_WKUP_IRQHandler(void)
{
    HAL_RTCEx_WakeUpTimerIRQHandler(&__stm32_rtc_handle);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    if (rtc_wakeup_timer_callback != MS_NULL) {
        rtc_wakeup_timer_callback(rtc_wakeup_timer_arg);
    }
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
