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
** 文   件   名: stm32_drv_rtc.h
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 RTC 驱动头文件
*********************************************************************************************************/

#ifndef STM32_DRV_RTC_H
#define STM32_DRV_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  RTC 设备注册接口
*********************************************************************************************************/

ms_err_t stm32_rtc_dev_create(void);

/*********************************************************************************************************
  RTC 驱动层接口
*********************************************************************************************************/

void     stm32_rtc_get_capability(ms_rtc_capability_t *capability);

ms_err_t stm32_rtc_set_time(const ms_rtc_time_t *rtc_time);

ms_err_t stm32_rtc_get_time(ms_rtc_time_t *rtc_time);

ms_err_t stm32_rtc_set_alarm(const ms_rtc_time_t *rtc_time, ms_callback_t callback, ms_ptr_t arg);

ms_err_t stm32_rtc_get_alarm(ms_rtc_time_t *rtc_time);

ms_err_t stm32_rtc_set_wakeup_timer(ms_uint32_t sec, ms_callback_t callback, ms_ptr_t arg);

ms_err_t stm32_rtc_get_wakeup_timer(ms_uint32_t *sec);

void     stm32_rtc_alarm_reset(void);

void     stm32_rtc_wakeup_timer_reset(void);


#ifdef __cplusplus
}
#endif

#endif                                                                  /*  STM32_DRV_RTC_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
