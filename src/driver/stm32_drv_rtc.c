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

#if BSP_CFG_RTC_EN > 0

static RTC_HandleTypeDef stm32_rtc_handle;

static ms_err_t __stm32_rtc_set_time(const ms_rtc_time_t *rtc_time)
{
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;

    /*
     * Set Date
     */
    sdatestructure.Year    = rtc_time->year;
    sdatestructure.Month   = rtc_time->month;
    sdatestructure.Date    = rtc_time->date;
    sdatestructure.WeekDay = rtc_time->weekday;
    HAL_RTC_SetDate(&stm32_rtc_handle, &sdatestructure, RTC_FORMAT_BIN);

    /*
     * Set Time
     */
    stimestructure.Hours          = rtc_time->hour;
    stimestructure.Minutes        = rtc_time->minute;
    stimestructure.Seconds        = rtc_time->second;
    stimestructure.TimeFormat     = RTC_HOURFORMAT_24;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime(&stm32_rtc_handle, &stimestructure, RTC_FORMAT_BIN);

    return MS_ERR_NONE;
}

static ms_err_t __stm32_rtc_get_time(ms_rtc_time_t *rtc_time)
{
    RTC_DateTypeDef sdatestructureget;
    RTC_TimeTypeDef stimestructureget;

    /*
     * Get the RTC current Time
     */
    HAL_RTC_GetTime(&stm32_rtc_handle, &stimestructureget, RTC_FORMAT_BIN);

    /*
     * Get the RTC current Date
     */
    HAL_RTC_GetDate(&stm32_rtc_handle, &sdatestructureget, RTC_FORMAT_BIN);

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
static void RTC_CalendarConfig(void)
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
    HAL_RTC_SetDate(&stm32_rtc_handle, &sdatestructure, RTC_FORMAT_BIN);

    /*
     * Set Time: 02:00:00
     */
    stimestructure.Hours          = 2;
    stimestructure.Minutes        = 0;
    stimestructure.Seconds        = 0;
    stimestructure.TimeFormat     = RTC_HOURFORMAT_24;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;
    HAL_RTC_SetTime(&stm32_rtc_handle, &stimestructure, RTC_FORMAT_BIN);

    /*
     * Writes a data in a RTC Backup data Register1
     */
    HAL_RTCEx_BKUPWrite(&stm32_rtc_handle, RTC_BKP_DR1, 0x32F2);
}

static ms_err_t __stm32_rtc_init(void)
{
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
    stm32_rtc_handle.Instance            = RTC;
    stm32_rtc_handle.Init.HourFormat     = RTC_HOURFORMAT_24;
    stm32_rtc_handle.Init.AsynchPrediv   = BSP_CFG_RTC_ASYNCH_PREDIV;
    stm32_rtc_handle.Init.SynchPrediv    = BSP_CFG_RTC_SYNCH_PREDIV;
    stm32_rtc_handle.Init.OutPut         = RTC_OUTPUT_DISABLE;
    stm32_rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    stm32_rtc_handle.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    __HAL_RTC_RESET_HANDLE_STATE(&stm32_rtc_handle);
    HAL_RTC_Init(&stm32_rtc_handle);

    /*
     * Read the Back Up Register 1 Data
     */
    if (HAL_RTCEx_BKUPRead(&stm32_rtc_handle, RTC_BKP_DR1) != 0x32F2) {
        /*
         * Configure RTC Calendar
         */
        RTC_CalendarConfig();
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

ms_rtc_drv_t *stm32_rtc_drv(void)
{
    return &__stm32_rtc_drv;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
