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
** 文   件   名: stm32_drv_usb_host.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 USB HOST 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include "includes.h"

#if BSP_CFG_UDISK_EN > 0

#include "usbh_core.h"
#include "usbh_msc.h"

ms_handle_t                 stm32_usb_connect_semid;
USBH_HandleTypeDef          hUSBHost;
extern HCD_HandleTypeDef    hhcd;

/**
  * @brief  This function handles USB-On-The-Go FS/HS global interrupt request.
  *
  * @retval N/A
  */
#ifdef USE_USB_FS
void OTG_FS_IRQHandler(void)
#else
void OTG_HS_IRQHandler(void)
#endif
{
    (void)ms_int_enter();

    HAL_HCD_IRQHandler(&hhcd);

    (void)ms_int_exit();
}

/**
 * @brief  User Process
 *
 * @param  phost: Host Handle
 * @param  id: Host Library user message ID
 *
  * @retval N/A
 */
void USBH_UserProcess(USBH_HandleTypeDef * phost, uint8_t id)
{
    switch (id) {
    case HOST_USER_SELECT_CONFIGURATION:
        break;

    case HOST_USER_DISCONNECTION:
        break;

    case HOST_USER_CLASS_ACTIVE:
        break;

    case HOST_USER_CONNECTION:
        (void)ms_semc_post(stm32_usb_connect_semid);
        break;

    default:
        break;
    }
}

ms_err_t stm32_usb_host_init(void)
{
    ms_err_t err;

    err = ms_semc_create("usb_semc", 0, UINT32_MAX, MS_WAIT_TYPE_PRIO, &stm32_usb_connect_semid);
    if (err == MS_ERR_NONE) {
        /*
         * Init Host Library
         */
        if (USBH_Init(&hUSBHost, USBH_UserProcess, 0) == USBH_OK) {
            /*
             * Add Supported Class
             */
            if (USBH_RegisterClass(&hUSBHost, USBH_MSC_CLASS) == USBH_OK) {
                /*
                 * Start Host Process
                 */
                if (USBH_Start(&hUSBHost) != USBH_OK) {
                    err == MS_ERR;
                }
            } else {
                err == MS_ERR;
            }
        } else {
            err == MS_ERR;
        }
    }

    return err;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
