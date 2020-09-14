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
** 文   件   名: stm32_drv_spi_nor.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 SPI NOR 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include "includes.h"

#if BSP_CFG_SPI_NOR_EN > 0

#include "ms_littlefs.h"

#if BSP_CFG_RUN_IN != BSP_RUN_IN_QSPI_FLASH

/*
 * Read a region in a block. Negative error codes are propogated to the user.
 */
static int __stm32_spi_nor_block_read(const struct lfs_config *c, lfs_block_t block,
                                      lfs_off_t off, void *buffer, lfs_size_t size)
{
    int ret;

    if (BSP_QSPI_Read((uint8_t *)buffer, (block * c->block_size + off), size) == QSPI_OK) {
        ret = LFS_ERR_OK;
    } else {
        ret = LFS_ERR_CORRUPT;
    }

    return ret;
}

/*
 * Program a region in a block. The block must have previously
 * been erased. Negative error codes are propogated to the user.
 * May return LFS_ERR_CORRUPT if the block should be considered bad.
 */
static int __stm32_spi_nor_block_prog(const struct lfs_config *c, lfs_block_t block,
                                      lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int ret;

    if (BSP_QSPI_Write((uint8_t *)buffer, (block * c->block_size + off), size) == QSPI_OK) {
        ret = LFS_ERR_OK;
    } else {
        ret = LFS_ERR_CORRUPT;
    }

    return ret;
}

/*
 * Erase a block. A block must be erased before being programmed.
 * The state of an erased block is undefined. Negative error codes
 * are propogated to the user.
 * May return LFS_ERR_CORRUPT if the block should be considered bad.
 *
 */
static int __stm32_spi_nor_block_erase(const struct lfs_config *c, lfs_block_t block)
{
    int ret;

    if (BSP_QSPI_Erase_Block(block * c->block_size) == QSPI_OK) {
        ret = LFS_ERR_OK;
    } else {
        ret = LFS_ERR_CORRUPT;
    }

    return ret;
}

/*
 * Sync the state of the underlying block device. Negative error codes
 * are propogated to the user.
 */
static int __stm32_spi_nor_block_sync(const struct lfs_config *c)
{
    return 0;
}

/*
 * configuration of the filesystem is provided by this struct
 */
static struct lfs_config stm32_spi_nor_cfg = {
    /*
     * block device operations
     */
    .read  = __stm32_spi_nor_block_read,
    .prog  = __stm32_spi_nor_block_prog,
    .erase = __stm32_spi_nor_block_erase,
    .sync  = __stm32_spi_nor_block_sync,
};

/*
 * Create spi nor flash device file and mount
 */
ms_err_t stm32_spi_nor_dev_init(const char *path, const char *mnt_path)
{
#if BSP_CFG_QSPI_FLASH_MEM_MAP_EN > 0
    ms_err_t err;

    if (BSP_QSPI_Init() == QSPI_OK) {
        if (BSP_QSPI_EnableMemoryMappedMode() == QSPI_OK) {
            err = MS_ERR_NONE;
        } else {
            err = MS_ERR;
        }
    } else {
        err = MS_ERR;
    }

    return err;


#else
    static ms_io_device_t spi_nor_dev;
    QSPI_Info info;
    ms_err_t err;

    if (BSP_QSPI_Init() == QSPI_OK) {

        if (BSP_QSPI_GetInfo(&info) == QSPI_OK) {

            stm32_spi_nor_cfg.read_size      = 1U;
            stm32_spi_nor_cfg.prog_size      = info.ProgPageSize;
            stm32_spi_nor_cfg.block_size     = info.EraseSectorSize;
            stm32_spi_nor_cfg.block_count    = info.EraseSectorsNumber;
            stm32_spi_nor_cfg.cache_size     = info.ProgPageSize;
            stm32_spi_nor_cfg.block_cycles   = 500U;
            stm32_spi_nor_cfg.lookahead_size = 8U * ((stm32_spi_nor_cfg.block_count + 63U) / 64U);

            err = ms_io_device_register(&spi_nor_dev, path, "ms_null", &stm32_spi_nor_cfg);
            if (err == MS_ERR_NONE) {
                err = ms_io_mount(mnt_path, path, MS_LITTLEFS_NAME, MS_NULL);
            }
        } else {
            err = MS_ERR;
        }
    } else {
        err = MS_ERR;
    }

    return err;
#endif
}

#endif
#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
