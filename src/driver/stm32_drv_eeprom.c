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
** 文   件   名: stm32_drv_eeprom.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 EEPROM 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"

#if BSP_CFG_EEPROM_EN > 0

/*
 * Private info
 */
typedef struct {
    ms_handle_t     lockid;
    ms_uint32_t     eeprom_size;
} privinfo_t;

/*
 * Open device
 */
static int __stm32_eeprom_open(ms_ptr_t ctx, ms_io_file_t *file, int oflag, ms_mode_t mode)
{
    ms_atomic_inc(MS_IO_DEV_REF(file));

    return 0;
}

/*
 * Close device
 */
static int __stm32_eeprom_close(ms_ptr_t ctx, ms_io_file_t *file)
{
    ms_atomic_dec(MS_IO_DEV_REF(file));

    return 0;
}

/*
 * Read data from eeprom
 */
static int __stm32_eeprom_read_data(privinfo_t *priv, ms_eeprom_msg_t *msg)
{
    ms_uint16_t len = msg->len;
    int ret;

    if ((msg->memaddr >= priv->eeprom_size) ||
        (msg->memaddr + msg->len) > priv->eeprom_size) {
        ms_thread_set_errno(EINVAL);
        ret = -1;

    } else {
        if (BSP_EEPROM_ReadBuffer(msg->buf, msg->memaddr, &len) == EEPROM_OK) {
            ret = 0;

        } else {
            ms_thread_set_errno(EIO);
            ret = -1;
        }
    }

    return ret;
}

/*
 * Write data from eeprom
 */
static int __stm32_eeprom_write_data(privinfo_t *priv, ms_eeprom_msg_t *msg)
{
    int ret;

    if ((msg->memaddr >= priv->eeprom_size) ||
        (msg->memaddr + msg->len) > priv->eeprom_size) {
        ms_thread_set_errno(EINVAL);
        ret = -1;

    } else {
        if (BSP_EEPROM_WriteBuffer(msg->buf, msg->memaddr, msg->len) == EEPROM_OK) {
            ret = 0;

        } else {
            ms_thread_set_errno(EIO);
            ret = -1;
        }
    }

    return ret;
}

/*
 * Read device
 */
static ms_ssize_t __stm32_eeprom_read(ms_ptr_t ctx, ms_io_file_t *file, ms_ptr_t buf, ms_size_t len)
{
    ms_ssize_t ret;

    if (len % sizeof(ms_eeprom_msg_t) == 0) {
        privinfo_t      *priv = ctx;
        ms_eeprom_msg_t *msg = (ms_eeprom_msg_t *)buf;
        ms_uint32_t      n_msg = len / sizeof(ms_eeprom_msg_t);
        ms_uint32_t      i;

        ret = 0;

        ms_mutex_lock(priv->lockid, MS_TIMEOUT_FOREVER);

        for (i = 0; i < n_msg; i++, msg++) {
            if (!ms_access_ok((ms_const_ptr_t)msg->buf, msg->len, MS_ACCESS_W)) {
                ms_thread_set_errno(EFAULT);
                break;
            }

            if (__stm32_eeprom_read_data(priv, msg) != 0) {
                break;
            }
        }

        ms_mutex_unlock(priv->lockid);

        if (i == n_msg) {
            ret = len;
        } else {
            ret = -1;
        }
    } else {
        ms_thread_set_errno(EINVAL);
        ret = -1;
    }

    return ret;
}

/*
 * Write device
 */
static ms_ssize_t __stm32_eeprom_write(ms_ptr_t ctx, ms_io_file_t *file, ms_const_ptr_t buf, ms_size_t len)
{
    ms_ssize_t ret;

    if (len % sizeof(ms_eeprom_msg_t) == 0) {
        privinfo_t      *priv = ctx;
        ms_eeprom_msg_t *msg = (ms_eeprom_msg_t *)buf;
        ms_uint32_t      n_msg = len / sizeof(ms_eeprom_msg_t);
        ms_uint32_t      i;

        ret = 0;

        ms_mutex_lock(priv->lockid, MS_TIMEOUT_FOREVER);

        for (i = 0; i < n_msg; i++, msg++) {
            if (!ms_access_ok((ms_const_ptr_t)msg->buf, msg->len, MS_ACCESS_R)) {
                ms_thread_set_errno(EFAULT);
                break;
            }

            if (!ms_eeprom_write_ok(msg->memaddr, msg->len)) {
                ms_thread_set_errno(EACCES);
                break;
            }

            if (__stm32_eeprom_write_data(priv, msg) != MS_ERR_NONE) {
                break;
            }
        }

        ms_mutex_unlock(priv->lockid);

        if (i == n_msg) {
            ret = len;
        } else {
            ret = -1;
        }
    } else {
        ms_thread_set_errno(EINVAL);
        ret = -1;
    }

    return ret;
}

/*
 * Control device
 */
static int __stm32_eeprom_ioctl(ms_ptr_t ctx, ms_io_file_t *file, int cmd, ms_ptr_t arg)
{
    privinfo_t *priv = ctx;
    int ret;

    switch (cmd) {
    case MS_EEPROM_CMD_GET_GEOMETRY:
        if (ms_access_ok(arg, sizeof(ms_eeprom_geometry_t), MS_ACCESS_W)) {
            ms_eeprom_geometry_t *geometry = (ms_eeprom_geometry_t *)arg;
            geometry->size = priv->eeprom_size;
            ret = 0;

        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
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
static const ms_io_driver_ops_t stm32_eeprom_drv_ops = {
        .type   = MS_IO_DRV_TYPE_CHR,
        .open   = __stm32_eeprom_open,
        .close  = __stm32_eeprom_close,
        .write  = __stm32_eeprom_write,
        .read   = __stm32_eeprom_read,
        .ioctl  = __stm32_eeprom_ioctl,
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_eeprom_drv = {
        .nnode = {
            .name = "stm32_eeprom",
        },
        .ops = &stm32_eeprom_drv_ops,
};

/*
 * Register eeprom device driver
 */
ms_err_t stm32_eeprom_drv_register(void)
{
    return ms_io_driver_register(&stm32_eeprom_drv);
}

/*
 * Create eeprom device file
 */
ms_err_t stm32_eeprom_dev_register(const char *path)
{
    static privinfo_t priv_info;
    static ms_io_device_t dev;
    ms_err_t err;

    err = ms_mutex_create("eeprom_lock", MS_WAIT_TYPE_PRIO, &priv_info.lockid);
    if (err == MS_ERR_NONE) {

        if (BSP_EEPROM_Init() == EEPROM_OK) {

            priv_info.eeprom_size = BSP_EEPROM_GetCapacity();

            err = ms_io_device_register(&dev, path, "stm32_eeprom", &priv_info);

        } else {
            err = MS_ERR;
        }
    }

    return err;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
