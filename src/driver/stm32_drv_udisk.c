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
** 文   件   名: stm32_drv_udisk.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 UDISK 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include "includes.h"

#if BSP_CFG_UDISK_EN > 0

#include "ms_fatfs.h"

#include "usbh_core.h"
#include "usbh_msc.h"

#define USB_DEFAULT_BLOCK_SIZE  512U

static ms_uint32_t          scratch[FF_MAX_SS / 4U];
extern USBH_HandleTypeDef   hUSBHost;
extern ms_handle_t          stm32_usb_connect_semid;

#define lun                 0U

/*
 * Control device
 */
static int __stm32_udisk_ioctl(ms_ptr_t ctx, ms_io_file_t *file, int cmd, void *arg)
{
    MSC_LUNTypeDef info;
    int ret;

    switch (cmd) {
    case MS_BLKDEV_CMD_INIT:
        ret = 0;
        break;

    case MS_BLKDEV_CMD_GET_STATUS:
        if (USBH_MSC_UnitIsReady(&hUSBHost, lun)) {
            *(ms_uint32_t *)arg = MS_BLKDEV_STATUS_OK;
        } else {
            *(ms_uint32_t *)arg = MS_BLKDEV_STATUS_NOINIT;
        }
        ret = 0;
        break;

    case MS_BLKDEV_CMD_SYNC:
        ret = 0;
        break;

    case MS_BLKDEV_CMD_GET_SECT_NR:
        if (USBH_MSC_GetLUNInfo(&hUSBHost, lun, &info) == USBH_OK) {
            *(ms_uint32_t *)arg = info.capacity.block_nbr;
            ret = 0;
        } else {
            ret = -1;
        }
        break;

    case MS_BLKDEV_CMD_GET_SECT_SZ:
        if (USBH_MSC_GetLUNInfo(&hUSBHost, lun, &info) == USBH_OK) {
            *(ms_uint32_t *)arg = info.capacity.block_size;
            ret = 0;
        } else {
            ret = -1;
        }
        break;

    case MS_BLKDEV_CMD_GET_BLK_SZ:
        if (USBH_MSC_GetLUNInfo(&hUSBHost, lun, &info) == USBH_OK) {
            *(ms_uint32_t *)arg = info.capacity.block_size / USB_DEFAULT_BLOCK_SIZE;
            ret = 0;
        } else {
            ret = -1;
        }
        break;

    case MS_BLKDEV_CMD_TRIM:
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
 * Read block of device
 */
static ms_ssize_t __stm32_udisk_readblk(ms_ptr_t ctx, ms_io_file_t *file, ms_size_t blk_no, ms_size_t blk_cnt, ms_ptr_t buf)
{
    USBH_StatusTypeDef status = USBH_OK;
    ms_ssize_t len;

    if (((ms_addr_t)buf & 3) && (((HCD_HandleTypeDef *)hUSBHost.pData)->Init.dma_enable)) {
        while ((blk_cnt--) && (status == USBH_OK)) {
            status = USBH_MSC_Read(&hUSBHost, lun, blk_no + blk_cnt, (uint8_t *)scratch, 1);
            if (status == USBH_OK) {
                memcpy(&((ms_uint8_t *)buf)[blk_cnt * FF_MAX_SS], scratch, FF_MAX_SS);
            } else {
                break;
            }
        }

    } else {
        status = USBH_MSC_Read(&hUSBHost, lun, blk_no, buf, blk_cnt);
    }

    if (status == USBH_OK) {
        len = blk_cnt * USB_DEFAULT_BLOCK_SIZE;
    } else {
        len = -1;
    }

    return len;
}

/*
 * Write block of device
 */
static ms_ssize_t __stm32_udisk_writeblk(ms_ptr_t ctx, ms_io_file_t *file, ms_size_t blk_no, ms_size_t blk_cnt, ms_const_ptr_t buf)
{
    USBH_StatusTypeDef status = USBH_OK;
    ms_ssize_t len;

    if (((ms_addr_t)buf & 3) && (((HCD_HandleTypeDef *)hUSBHost.pData)->Init.dma_enable)) {
        while (blk_cnt--) {
            memcpy(scratch, &((ms_uint8_t *)buf)[blk_cnt * FF_MAX_SS], FF_MAX_SS);
            status = USBH_MSC_Write(&hUSBHost, lun, blk_no + blk_cnt, (uint8_t *)scratch, 1);
            if (status == USBH_FAIL) {
                break;
            }
        }

    } else {
        status = USBH_MSC_Write(&hUSBHost, lun, blk_no, (uint8_t *)buf, blk_cnt);
    }

    if (status == USBH_OK) {
        len = blk_cnt * USB_DEFAULT_BLOCK_SIZE;
    } else {
        len = -1;
    }

    return len;
}

/*
 * Device operating function set
 */
static const ms_io_driver_ops_t stm32_udisk_drv_ops = {
        .type     = MS_IO_DRV_TYPE_BLK,
        .ioctl    = __stm32_udisk_ioctl,
        .readblk  = __stm32_udisk_readblk,
        .writeblk = __stm32_udisk_writeblk,
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_udisk_drv = {
        .nnode = {
            .name = "stm32_udisk",
        },
        .ops = &stm32_udisk_drv_ops,
};

/*
 * Register udisk block device driver
 */
ms_err_t stm32_udisk_drv_register(void)
{
    return ms_io_driver_register(&stm32_udisk_drv);
}

/*
 * Create udisk block device file
 */
ms_err_t stm32_udisk_dev_create(void)
{
    static ms_io_device_t udisk_blk_dev[1];

    return ms_io_device_register(&udisk_blk_dev[0], "/dev/udisk_blk0", "stm32_udisk", MS_NULL);
}

/*
 * Mount udisk block device
 */
ms_err_t stm32_udisk_dev_mount(void)
{
    static ms_io_mnt_t udisk_mnt[1];
    ms_err_t err;

    ms_printk(MS_PK_INFO, "Wait USB DISK insert...");

    err = ms_semc_wait(stm32_usb_connect_semid, MS_TIMEOUT_FOREVER);

    if (err == MS_ERR_NONE) {
        ms_thread_sleep_s(2);

        err = ms_io_mount(&udisk_mnt[0], "/udisk0", "/dev/udisk_blk0", MS_FATFS_NAME, (ms_ptr_t)0);
    } 

    return err;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
