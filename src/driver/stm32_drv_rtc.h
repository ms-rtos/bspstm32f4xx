/*********************************************************************************************************
**
**                                ���������Ϣ�������޹�˾
**
**                                  ΢�Ͱ�ȫʵʱ����ϵͳ
**
**                                      MS-RTOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: stm32_drv_rtc.h
**
** ��   ��   ��: Jiao.jinxing
**
** �ļ���������: 2020 �� 04 �� 07 ��
**
** ��        ��: STM32 оƬ RTC ����ͷ�ļ�
*********************************************************************************************************/

#ifndef STM32_DRV_RTC_H
#define STM32_DRV_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  RTC �豸ע��ӿ�
*********************************************************************************************************/

ms_err_t stm32_rtc_dev_create(void);

/*********************************************************************************************************
  RTC ������ӿ�
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
