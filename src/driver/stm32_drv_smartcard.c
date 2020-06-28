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
** 文   件   名: stm32_drv_smartcard.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 SMARTCARD 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"

#if BSP_CFG_SMARTCARD_EN > 0

/*
 * @brief stm32 smartcard info
 */
typedef struct {
    ms_ptr_t            context;
} privinfo_t;

/*
 * @brief stm32 smartcard device
 */
typedef struct {
    ms_io_device_t      dev;
    privinfo_t          priv;
} stm32_smartcard_dev_t;

/*
 * Open device
 */
static int __stm32_smartcard_open(ms_ptr_t ctx, ms_io_file_t *file, int oflag, ms_mode_t mode)
{
    int         ret;
    privinfo_t *priv = ctx;

    if (ms_atomic_inc(MS_IO_DEV_REF(file)) == 2) {

        if (BSP_SmartCard_Init(&priv->context) == MS_ERR_NONE) {
            ret = 0;

        } else {
            ms_atomic_dec(MS_IO_DEV_REF(file));
            ms_thread_set_errno(EBUSY);
            ret = -1;
        }

    } else {
        ms_atomic_dec(MS_IO_DEV_REF(file));
        ms_thread_set_errno(EBUSY);
        ret = -1;
    }

    return ret;
}

/*
 * Close device
 */
static int __stm32_smartcard_close(ms_ptr_t ctx, ms_io_file_t *file)
{
    privinfo_t *priv = ctx;

    if (ms_atomic_dec(MS_IO_DEV_REF(file)) == 0) {
        BSP_SmartCard_DeInit(priv->context);
    }

    return 0;
}

/*
 * process one message
 */
static int __stm32_smartcard_send_msg(privinfo_t *priv, ms_smartcard_command_t *command, ms_smartcard_responce_t *responce)
{
    int ret;

    if ((command != MS_NULL) && (responce != MS_NULL)) {
        if (responce->len >= command->body.le) {
            if (BSP_SmartCard_SendMsg(priv->context, command, responce) == MS_ERR_NONE) {
                ret = 0;
            } else {
                ms_thread_set_errno(EIO);
                ret = -1;
            }

        } else {
            ms_thread_set_errno(EINVAL);
            ret = -1;
        }

    } else {
        ms_thread_set_errno(EFAULT);
        ret = -1;
    }

    return ret;
}

static int __stm32_smartcard_ioctl(ms_ptr_t ctx, ms_io_file_t *file, int cmd, void *arg)
{
    int         ret;
    privinfo_t *priv = ctx;

    switch (cmd) {

    case MS_SMARTCARD_CMD_POWER_ON:
        if (BSP_SmartCard_PowerOn(priv->context) == MS_ERR_NONE) {
            ret = 0;
        } else {
            ms_thread_set_errno(EIO);
            ret = -1;
        }
        break;

    case MS_SMARTCARD_CMD_GET_ATR:
        if (ms_access_ok(arg, sizeof(ms_smartcard_atr_t), MS_ACCESS_W)) {
            ms_smartcard_atr_t *sc_art = (ms_smartcard_atr_t*)(arg);
            BSP_SmartCard_AtrGet(priv->context, sc_art);
            ret = 0;
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_SMARTCARD_CMD_SEND_MSG:
        if (ms_access_ok(arg, sizeof(ms_smartcard_msg_t), MS_ACCESS_RW)) {
            ms_smartcard_msg_t *sc_msg = (ms_smartcard_msg_t*)(arg);
            ms_smartcard_command_t  *sc_cmd = &sc_msg->command;
            ms_smartcard_responce_t *sc_rep = &sc_msg->responce;

            if (ms_access_ok(sc_cmd->body.data, sc_cmd->body.lc, MS_ACCESS_R) &&
                ms_access_ok(sc_rep->data, sc_rep->len, MS_ACCESS_W)) {

                ret = __stm32_smartcard_send_msg(priv, sc_cmd, sc_rep);

            } else {
                ms_thread_set_errno(EFAULT);
                ret = -1;
            }
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_SMARTCARD_CMD_POWER_OFF:
        BSP_SmartCard_PowerOff(priv->context);
        ret = 0;
        break;

    default:
        ms_thread_set_errno(EINVAL);
        ret = -1;
        break;
    }

    return ret;
}

/*
 * Device operating function set
 */
static const ms_io_driver_ops_t stm32_smartcard_drv_ops = {
        .type   = MS_IO_DRV_TYPE_CHR,
        .open   = __stm32_smartcard_open,
        .close  = __stm32_smartcard_close,
        .ioctl  = __stm32_smartcard_ioctl,
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_smartcard_drv = {
        .nnode = {
            .name = "stm32_smartcard",
        },
        .ops = &stm32_smartcard_drv_ops,
};

/*
 * Register smartcard device driver
 */
ms_err_t stm32_smartcard_drv_register(void)
{
    return ms_io_driver_register(&stm32_smartcard_drv);
}

/*
 * Create smartcard device file
 */
ms_err_t stm32_smartcard_dev_create(const char *path)
{
    ms_err_t err;
    stm32_smartcard_dev_t *dev;

    if (path != MS_NULL) {

        dev = ms_kzalloc(sizeof(stm32_smartcard_dev_t));
        if (dev != MS_NULL) {

            err = ms_io_device_register((ms_io_device_t *)dev, path, "stm32_smartcard", &dev->priv);
            if (err != MS_ERR_NONE) {
                ms_kfree(dev);
                err = MS_ERR;
            }

        } else {
            err = MS_ERR;
        }

    } else {
        err = MS_ERR;
    }

    return err;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
