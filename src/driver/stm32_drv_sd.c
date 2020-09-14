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
** 文   件   名: stm32_drv_sd.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 SD 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include "includes.h"

#if BSP_CFG_SD_EN > 0

#include "ms_fatfs.h"

#define SD_DEFAULT_BLOCK_SIZE       512U
#define SD_TIMEOUT                  (2U * 1000U)

static ms_handle_t      stm32_sd_sync_semid;
extern SD_HandleTypeDef uSdHandle;

/*
 * Control device
 */
static int __stm32_sd_ioctl(ms_ptr_t ctx, ms_io_file_t *file, int cmd, void *arg)
{
    BSP_SD_CardInfo CardInfo;
    int ret;

    switch (cmd) {
    case MS_BLKDEV_CMD_INIT:
        if (BSP_SD_IsDetected()) {
            ret = 0;
        } else {
            ret = -1;
        }
        break;

    case MS_BLKDEV_CMD_GET_STATUS:
        if (BSP_SD_IsDetected()) {
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
        BSP_SD_GetCardInfo(&CardInfo);
        *(ms_uint32_t *)arg = CardInfo.LogBlockNbr;
        ret = 0;
        break;

    case MS_BLKDEV_CMD_GET_SECT_SZ:
        BSP_SD_GetCardInfo(&CardInfo);
        *(ms_uint32_t *)arg = CardInfo.LogBlockSize;
        ret = 0;
        break;

    case MS_BLKDEV_CMD_GET_BLK_SZ:
        BSP_SD_GetCardInfo(&CardInfo);
        *(ms_uint32_t *)arg = CardInfo.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
        ret = 0;
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
static ms_ssize_t __stm32_sd_readblk(ms_ptr_t ctx, ms_io_file_t *file, ms_size_t blk_no, ms_size_t blk_cnt, ms_ptr_t buf)
{
    ms_ssize_t len;
#if (BSP_CFG_CPU_HAS_CACHE > 0)
     ms_uint32_t aligned_addr;

     /*
      * the SCB_CleanDCache_by_Addr() requires a 32-Byte aligned address
      * adjust the address and the D-Cache size to clean accordingly.
      */
     aligned_addr = (ms_uint32_t)buf & ~0x1fU;
     SCB_CleanInvalidateDCache_by_Addr((uint32_t *)aligned_addr,
             blk_cnt * SD_DEFAULT_BLOCK_SIZE + ((ms_uint32_t)buf - aligned_addr));
#endif

    if (BSP_SD_ReadBlocks_DMA(buf, blk_no, blk_cnt) == MSD_OK) {
        if (ms_semc_wait(stm32_sd_sync_semid, SD_TIMEOUT) == MS_ERR_NONE) {
            while (BSP_SD_GetCardState() != SD_TRANSFER_OK) {
                ms_thread_sleep(1);
            }
            len = blk_cnt * SD_DEFAULT_BLOCK_SIZE;
        } else {
            len = -1;
        }
    } else {
        len = -1;
    }

    return len;
}

/*
 * Write block of device
 */
static ms_ssize_t __stm32_sd_writeblk(ms_ptr_t ctx, ms_io_file_t *file, ms_size_t blk_no, ms_size_t blk_cnt, ms_const_ptr_t buf)
{
    ms_ssize_t len;
    ms_ptr_t wbuf = MS_NULL;
#if (BSP_CFG_CPU_HAS_CACHE > 0)
    ms_uint32_t aligned_addr;
#endif

    /*
     * the SCB_CleanDCache_by_Addr() requires a 32-Byte aligned address
     * adjust the address and the D-Cache size to clean accordingly.
     */
    if ((ms_uint32_t)buf & 0x1fU) {
        wbuf = ms_kmalloc_align(blk_cnt * SD_DEFAULT_BLOCK_SIZE, MS_ARCH_CACHE_LINE_SIZE);
        if (wbuf != MS_NULL) {
            ms_arch_memcpy(wbuf, buf, blk_cnt * SD_DEFAULT_BLOCK_SIZE);
            buf = wbuf;
        } else {
            ms_printk(MS_PK_ERR, "__stm32_sd_writeblk allocate memory failed!\r\n");
        }
    }

#if (BSP_CFG_CPU_HAS_CACHE > 0)
    aligned_addr = (ms_uint32_t)buf & ~0x1fU;
    SCB_CleanDCache_by_Addr((uint32_t *)aligned_addr,
            blk_cnt * SD_DEFAULT_BLOCK_SIZE + ((ms_uint32_t)buf - aligned_addr));
#endif

    if (BSP_SD_WriteBlocks_DMA((ms_ptr_t)buf, blk_no, blk_cnt) == MSD_OK) {
        if (ms_semc_wait(stm32_sd_sync_semid, SD_TIMEOUT) == MS_ERR_NONE) {
            while (BSP_SD_GetCardState() != SD_TRANSFER_OK) {
                ms_thread_sleep(1);
            }
            len = blk_cnt * SD_DEFAULT_BLOCK_SIZE;
        } else {
            len = -1;
        }
    } else {
        len = -1;
    }

    if (wbuf != MS_NULL) {
        ms_kfree(wbuf);
    }

    return len;
}

/**
 * @brief Tx Transfer completed callbacks
 * @retval None
 */
void BSP_SD_WriteCpltCallback(void)
{
    (void)ms_semc_post(stm32_sd_sync_semid);
}

/**
 * @brief Rx Transfer completed callbacks
 * @retval None
 */
void BSP_SD_ReadCpltCallback(void)
{
    (void)ms_semc_post(stm32_sd_sync_semid);
}

/**
 * @brief BSP SD Abort callbacks
 */
void BSP_SD_AbortCallback(void)
{
    (void)ms_semc_post(stm32_sd_sync_semid);
}

/*
 * Device operating function set
 */
static const ms_io_driver_ops_t stm32_sd_drv_ops = {
        .type     = MS_IO_DRV_TYPE_BLK,
        .ioctl    = __stm32_sd_ioctl,
        .readblk  = __stm32_sd_readblk,
        .writeblk = __stm32_sd_writeblk,
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_sd_drv = {
        .nnode = {
            .name = "stm32_sd",
        },
        .ops = &stm32_sd_drv_ops,
};

static void __stm32_sd_detect(ms_ptr_t arg)
{
    static ms_io_device_t sd_blk_dev[1];
    static ms_bool_t registed = MS_FALSE;
           ms_bool_t detected;
           ms_err_t  err;

    detected = BSP_SD_IsDetected();
    if (detected && !registed) {

        BSP_SD_Init();

        err = ms_io_device_register(&sd_blk_dev[0], "/dev/sd_blk0", "stm32_sd", MS_NULL);
        if (err == MS_ERR_NONE) {
            registed = MS_TRUE;
        }

        ms_io_mount("/sd0", "/dev/sd_blk0", MS_FATFS_NAME, (ms_ptr_t)0);

    } else if (!detected && registed) {

        ms_io_unmount("/sd0", MS_NULL);

        err = ms_io_device_unregister(&sd_blk_dev[0]);
        if (err == MS_ERR_NONE) {
            registed = MS_FALSE;
        }

        BSP_SD_DeInit();
    }
}

/*
 * Register sdcard block device driver
 */
ms_err_t stm32_sd_drv_register(void)
{
    static ms_io_job_t sd_detect_job;

    BSP_SD_Init();

    ms_semc_create("sd_semc", 0, UINT32_MAX, MS_WAIT_TYPE_PRIO, &stm32_sd_sync_semid);

    ms_io_driver_register(&stm32_sd_drv);

    ms_io_job_init(&sd_detect_job, "sd_detect", __stm32_sd_detect, MS_NULL);
    ms_io_job_start(&sd_detect_job, 0, MS_TICK_PER_SEC, MS_IO_JOB_OPT_PERIODIC);

    return MS_ERR_NONE;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
