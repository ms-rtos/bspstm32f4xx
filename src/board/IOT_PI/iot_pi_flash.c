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
** 文   件   名: iot_pi_flash.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi FLASH 布局
*********************************************************************************************************/
#include "ms_config.h"
#include "ms_rtos.h"
#include "includes.h"

#if BSP_CFG_FLASH_EN > 0

static const ms_uint32_t flash_sectors_size[] = {
        128U * 1024U,     /* ADDR_FLASH_SECTOR_6  */
        128U * 1024U,     /* ADDR_FLASH_SECTOR_7  */
        128U * 1024U,     /* ADDR_FLASH_SECTOR_8  */
        128U * 1024U,     /* ADDR_FLASH_SECTOR_9  */
        128U * 1024U,     /* ADDR_FLASH_SECTOR_10 */
        128U * 1024U,     /* ADDR_FLASH_SECTOR_11 */
};

static const ms_uint32_t flash_sectors_id[] = {
        FLASH_SECTOR_6,  /* ADDR_FLASH_SECTOR_6  */
        FLASH_SECTOR_7,  /* ADDR_FLASH_SECTOR_7  */
        FLASH_SECTOR_8,  /* ADDR_FLASH_SECTOR_8  */
        FLASH_SECTOR_9,  /* ADDR_FLASH_SECTOR_9  */
        FLASH_SECTOR_10, /* ADDR_FLASH_SECTOR_10 */
        FLASH_SECTOR_11, /* ADDR_FLASH_SECTOR_11 */
};

static ms_flashfs_partition_t flash_partition = {
        .flash_base   = ADDR_FLASH_SECTOR_6,
        .n_sector     = MS_ARRAY_SIZE(flash_sectors_size),
        .sectors_size = flash_sectors_size,
};

/**
 * @brief  Gets the sector of a given address
 *
 * @param  None
 *
 * @retval The sector of a given address
 */
ms_uint32_t ms_bsp_flash_addr_to_sector(ms_addr_t addr)
{
    ms_uint32_t sector;

    if ((addr < ADDR_FLASH_SECTOR_1) && (addr >= ADDR_FLASH_SECTOR_0)) {
        sector = FLASH_SECTOR_0;

    } else if ((addr < ADDR_FLASH_SECTOR_2) && (addr >= ADDR_FLASH_SECTOR_1)) {
        sector = FLASH_SECTOR_1;

    } else if ((addr < ADDR_FLASH_SECTOR_3) && (addr >= ADDR_FLASH_SECTOR_2)) {
        sector = FLASH_SECTOR_2;

    } else if ((addr < ADDR_FLASH_SECTOR_4) && (addr >= ADDR_FLASH_SECTOR_3)) {
        sector = FLASH_SECTOR_3;

    } else if ((addr < ADDR_FLASH_SECTOR_5) && (addr >= ADDR_FLASH_SECTOR_4)) {
        sector = FLASH_SECTOR_4;

    } else if ((addr < ADDR_FLASH_SECTOR_6) && (addr >= ADDR_FLASH_SECTOR_5)) {
        sector = FLASH_SECTOR_5;

    } else if ((addr < ADDR_FLASH_SECTOR_7) && (addr >= ADDR_FLASH_SECTOR_6)) {
        sector = FLASH_SECTOR_6;

    } else if ((addr < ADDR_FLASH_SECTOR_8) && (addr >= ADDR_FLASH_SECTOR_7)) {
        sector = FLASH_SECTOR_7;

    } else if ((addr < ADDR_FLASH_SECTOR_9) && (addr >= ADDR_FLASH_SECTOR_8)) {
        sector = FLASH_SECTOR_8;

    } else if ((addr < ADDR_FLASH_SECTOR_10) && (addr >= ADDR_FLASH_SECTOR_9)) {
        sector = FLASH_SECTOR_9;

    } else if ((addr < ADDR_FLASH_SECTOR_11) && (addr >= ADDR_FLASH_SECTOR_10)) {
        sector = FLASH_SECTOR_10;

    } else {
        sector = FLASH_SECTOR_11;

    }

    return sector;
}

const ms_flashfs_partition_t *ms_bsp_flash_part_info(void)
{
    return &flash_partition;
}

ms_uint32_t ms_bsp_flash_data_sector(ms_uint32_t data_sector_id)
{
    return flash_sectors_id[data_sector_id];
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
