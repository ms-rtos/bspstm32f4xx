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
** 文   件   名: stm32_drv_flash.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 FLASH 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"

#if BSP_CFG_FLASH_EN > 0

#if BSP_CFG_RUN_IN == BSP_RUN_IN_FLASH

static ms_err_t __stm32_flash_get_part_info(ms_ptr_t priv, const ms_flashfs_partition_t **part)
{
    *part = ms_bsp_flash_part_info();

    return MS_ERR_NONE;
}

static ms_err_t __stm32_flash_erase_sector(ms_ptr_t priv, ms_uint32_t sector_id)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t sector_err;
    ms_err_t err;

    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector        = ms_bsp_flash_data_sector(sector_id);
    EraseInitStruct.NbSectors     = 1U;

    err = (HAL_FLASHEx_Erase(&EraseInitStruct, &sector_err) == HAL_OK) ? MS_ERR_NONE : MS_ERR;

    HAL_FLASH_Lock();

    return err;
}

static ms_err_t __stm32_flash_program(ms_ptr_t priv, ms_addr_t addr, ms_const_ptr_t buf, ms_uint32_t len)
{
    ms_ptr_t addr_bak = (ms_ptr_t)addr;
    const ms_uint32_t *wbuf = buf;
    ms_size_t wlen = len >> 2U;
    const ms_uint8_t *cbuf;
    ms_err_t err = MS_ERR;

    HAL_FLASH_Unlock();

    while (wlen > 0) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, *wbuf++) != HAL_OK) {
            break;
        }

        wlen--;
        addr += 4U;
    }

    if (wlen == 0) {
        wlen = len & 0x3U;

        cbuf = (const ms_uint8_t *)wbuf;
        while (wlen > 0) {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr, *cbuf++) != HAL_OK) {
                break;
            }

            wlen--;
            addr += 1U;
        }
    }

    if (wlen == 0) {
        if (memcmp(addr_bak, buf, len) == 0) {
            err = MS_ERR_NONE;
        }
    }

    HAL_FLASH_Lock();

    return err;
}

static ms_err_t __stm32_flash_erase_program_tbl(ms_ptr_t priv, ms_addr_t tbl_addr, ms_const_ptr_t buf, ms_uint32_t len)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t sector_err;
    ms_err_t err;

    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector        = ms_bsp_flash_addr_to_sector(tbl_addr);
    EraseInitStruct.NbSectors     = 1U;

    err = (HAL_FLASHEx_Erase(&EraseInitStruct, &sector_err) == HAL_OK) ? MS_ERR_NONE : MS_ERR;

    HAL_FLASH_Lock();

    if (err == MS_ERR_NONE) {
        err = __stm32_flash_program(priv, tbl_addr, buf, len);
    }

    return err;
}

static ms_err_t __stm32_flash_invalid_tbl(ms_ptr_t priv, ms_addr_t tbl_addr)
{
    ms_uint32_t data = 0;

    return __stm32_flash_program(priv, tbl_addr, &data, sizeof(data));
}

static ms_flashfs_drv_t stm32_flash_drv = {
        .get_part_info      = __stm32_flash_get_part_info,
        .erase_sector       = __stm32_flash_erase_sector,
        .program            = __stm32_flash_program,
        .erase_program_tbl  = __stm32_flash_erase_program_tbl,
        .invalid_tbl        = __stm32_flash_invalid_tbl,
};

ms_err_t stm32_flash_mount(const char *mnt_path)
{
    ms_flashfs_mount_param_t mount_param;

    mount_param.main_tbl_addr        = BSP_CFG_FLASHFS_MAIN_TBL_ADDR;
    mount_param.sec_tbl_addr         = BSP_CFG_FLASHFS_SEC_TBL_ADDR;
    mount_param.drv                  = &stm32_flash_drv;
    mount_param.priv                 = MS_NULL;
    mount_param.readonly             = MS_FALSE;
    mount_param.calc_crc32           = ms_crc32;
    mount_param.mkfs_param.max_file  = BSP_CFG_FLASHFS_MAX_FILE;
    mount_param.mkfs_param.unit_size = BSP_CFG_FLASHFS_UNIT_SIZE;
    mount_param.update_req_path      = BSP_CFG_UPDATE_REQ_FILE;

    return ms_io_mount(mnt_path, MS_NULL, MS_FLASHFS_NAME, &mount_param);
}

#else

ms_err_t stm32_flash_mount(const char *mnt_path)
{
    ms_flashfs_mount_param_t mount_param;

    mount_param.main_tbl_addr        = BSP_CFG_FLASHFS_MAIN_TBL_ADDR;
    mount_param.sec_tbl_addr         = BSP_CFG_FLASHFS_SEC_TBL_ADDR
    mount_param.drv                  = MS_NULL;
    mount_param.priv                 = MS_NULL;
    mount_param.readonly             = MS_TRUE;
    mount_param.calc_crc32           = ms_crc32;
    mount_param.update_req_path      = BSP_CFG_UPDATE_REQ_FILE;

    return ms_io_mount(mnt_path, MS_NULL, MS_FLASHFS_NAME, &mount_param);
}

#endif

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
