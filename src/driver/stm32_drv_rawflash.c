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
** 文   件   名: stm32_drv_rawflash.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 RAW FLASH 驱动
*********************************************************************************************************/
#define __MS_IO
#include "config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"
#include <string.h>
#include "includes.h"

#define MS_RAWFLASH_DRV_NAME       "stm32_rawflash"

/*
 * Private info
 */
typedef struct {
    ms_uint32_t block_base;
    ms_uint32_t block_count;
    ms_uint32_t block_size;
} privinfo_t;

/*
 * rawflash device
 */
typedef struct {
    privinfo_t      priv;
    ms_io_device_t  dev;
} stm32_rawflash_dev_t;

/*
 * Open device
 */
static int __stm32_rawflash_open(ms_ptr_t ctx, ms_io_file_t *file, int oflag, ms_mode_t mode)
{
    ms_atomic_inc(MS_IO_DEV_REF(file));

    return 0;
}

/*
 * Close device
 */
static int __stm32_rawflash_close(ms_ptr_t ctx, ms_io_file_t *file)
{
    ms_atomic_dec(MS_IO_DEV_REF(file));

    return 0;
}

/*
 * Read or write one message data from rawflash
 */
static int __stm32_rawflash_rw_msg(privinfo_t *priv, ms_rawflash_msg_t *msg, ms_uint8_t access_mode)
{
    ms_size_t rawflash_size = priv->block_count * priv->block_size;
    int ret;

    if (msg->memaddr >= rawflash_size ||
        (msg->memaddr + msg->len) > rawflash_size) {
        ms_thread_set_errno(EINVAL);
        ret = -1;

    } else {
        ms_uint8_t *pbuf = msg->buf;
        ms_uint32_t addr = priv->block_base * priv->block_size + msg->memaddr;
        ms_uint32_t len;
        ms_uint32_t next_addr;
        ms_uint32_t already_len = 0; /* the length of already rw */

        ret = 0;

        while (already_len < msg->len) {
            next_addr = (addr + priv->block_size) / priv->block_size * priv->block_size;

            len = next_addr - addr;
            if ((already_len + len) > msg->len) {
                len = msg->len - already_len;
            }

            if (access_mode == MS_ACCESS_R) {
                if (BSP_QSPI_Read(pbuf, addr, len) != QSPI_OK) {
                    ms_thread_set_errno(EIO);
                    ret = -1;
                    break;
                }
            } else {
                if (BSP_QSPI_Write(pbuf, addr, len) != QSPI_OK) {
                    ms_thread_set_errno(EIO);
                    ret = -1;
                    break;
                }
            }

            addr = next_addr;
            pbuf += len;
            already_len += len;
        }
    }

    return ret;
}

static ms_ssize_t __stm32_rawflash_rw(privinfo_t *priv, ms_rawflash_msg_t *msg, ms_size_t len, ms_uint8_t access_mode)
{
    ms_ssize_t ret;
    ms_uint8_t buf_access_mode;

    if (access_mode == MS_ACCESS_W) {
        buf_access_mode = MS_ACCESS_R;
    } else {
        buf_access_mode = MS_ACCESS_W;
    }

    if (len % sizeof(ms_rawflash_msg_t) == 0) {
        ms_uint32_t n_msg = len / sizeof(ms_rawflash_msg_t);
        ms_uint32_t i;

        for (i = 0; i < n_msg; i++) {
            /*
             * access permission check
             */
            if (!ms_access_ok((void*)msg->buf, msg->len, buf_access_mode)) {
                ms_thread_set_errno(EFAULT);
                break;
            }

            /*
             * process reading or writing
             */
            if (__stm32_rawflash_rw_msg(priv, msg, access_mode) != 0) {
                break;
            }

            msg++;
        }

        if (i == n_msg) {
            ret = len;
        } else {
            ret = -1;
        }

    } else {
        ms_thread_set_errno(EFAULT);
        ret = -1;
    }

    return ret;
}

/*
 * Read device
 */
static ms_ssize_t __stm32_rawflash_read(ms_ptr_t ctx, ms_io_file_t *file, ms_ptr_t buf, ms_size_t len)
{
    privinfo_t *priv = ctx;

    return __stm32_rawflash_rw(priv, (ms_rawflash_msg_t *)buf, len, MS_ACCESS_R);
}

/*
 * Write device
 */
static ms_ssize_t __stm32_rawflash_write(ms_ptr_t ctx, ms_io_file_t *file, ms_const_ptr_t buf, ms_size_t len)
{
    privinfo_t *priv = ctx;

    return __stm32_rawflash_rw(priv, (ms_rawflash_msg_t *)buf, len, MS_ACCESS_W);
}

/*
 * Control device
 */
static int __stm32_rawflash_ioctl(ms_ptr_t ctx, ms_io_file_t *file, int cmd, void *arg)
{
    privinfo_t *priv = ctx;
    int ret;

    switch (cmd) {
    case MS_RAWFLASH_CMD_GET_GEOMETRY:
        if (ms_access_ok(arg, sizeof(ms_rawflash_geometry_t), MS_ACCESS_W)) {
            ms_rawflash_geometry_t *geometry = (ms_rawflash_geometry_t *)arg;
            geometry->sector_size  = priv->block_size;
            geometry->sector_count = priv->block_count;
            ret = 0;

        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_RAWFLASH_CMD_ERASE_SECTOR:
        if (ms_access_ok(arg, sizeof(ms_rawflash_erase_t), MS_ACCESS_R)) {
            ms_uint32_t i;
            ms_rawflash_erase_t *erase_msg = (ms_rawflash_erase_t *)arg;
            for (i = 0; i < erase_msg->count; i++) {
                if (erase_msg->sector + i < priv->block_count) {
                    BSP_QSPI_Erase_Block((priv->block_base + erase_msg->sector + i) * priv->block_size);
                } else {
                    break;
                }
            }
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
static ms_io_driver_ops_t stm32_rawflash_drv_ops = {
        .type   = MS_IO_DRV_TYPE_CHR,
        .open   = __stm32_rawflash_open,
        .close  = __stm32_rawflash_close,
        .write  = __stm32_rawflash_write,
        .read   = __stm32_rawflash_read,
        .ioctl  = __stm32_rawflash_ioctl,
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_rawflash_drv = {
    .nnode = {
        .name = MS_RAWFLASH_DRV_NAME,
    },
    .ops = &stm32_rawflash_drv_ops,
};

/*
 * Register rawflash device driver
 */
ms_err_t stm32_rawflash_drv_register(void)
{
    return ms_io_driver_register(&stm32_rawflash_drv);
}

/*
 * Create rawflash device file
 */
ms_err_t stm32_rawflash_dev_register(const char *path)
{
    static stm32_rawflash_dev_t dev;
    ms_err_t err;

    if (path != MS_NULL) {
        QSPI_RAW_Info  raw_info;

        if (BSP_QSPI_RAW_GetInfo(&raw_info) == QSPI_OK) {

            dev.priv.block_base  = raw_info.SectorBase;
            dev.priv.block_count = raw_info.SectorCount;
            dev.priv.block_size  = raw_info.SectorSize;
            err = ms_io_device_register(&dev.dev, path, MS_RAWFLASH_DRV_NAME, &dev.priv);

        } else {
            err = MS_ERR;
        }

    } else {
        err = MS_ERR;
    }

    return err;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
