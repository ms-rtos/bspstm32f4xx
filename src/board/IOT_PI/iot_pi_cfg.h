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
** 文   件   名: iot_pi_cfg.h
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi 配置头文件
*********************************************************************************************************/

#ifndef IOT_PI_CFG_H
#define IOT_PI_CFG_H

/*********************************************************************************************************
  运行地址配置
*********************************************************************************************************/

#define BSP_RUN_IN_FLASH            0
#define BSP_RUN_IN_SDRAM            1
#define BSP_RUN_IN_QSPI_FLASH       2

#define BSP_CFG_RUN_IN              BSP_RUN_IN_FLASH

/*********************************************************************************************************
  BOOTLOADER 配置
*********************************************************************************************************/

#define BSP_CFG_USE_BOOTLOADER      0

/*********************************************************************************************************
  CPU 主频配置
*********************************************************************************************************/

#define BSP_CFG_CPU_HZ              (100 * 1000 * 1000)
#define BSP_CFG_CPU_HAS_CACHE       0

/*********************************************************************************************************
  功能相关配置
*********************************************************************************************************/

#define BSP_CONSOLE_TRACE           0
#define BSP_CONSOLE_UART            1
#define BSP_CONSOLE_NULL            2

#define BSP_CFG_CONSOLE_DEV         BSP_CONSOLE_TRACE

/*********************************************************************************************************
  地址配置
*********************************************************************************************************/

#define BSP_CFG_ROM_BASE            (0x08000000)
#define BSP_CFG_ROM_SIZE            (512 * 1024 * 1024)

#define BSP_CFG_RAM_BASE            (0x20000000)
#define BSP_CFG_RAM_SIZE            (256 * 1024)

#if BSP_CFG_USE_BOOTLOADER > 0U
#define BSP_CFG_KERN_ROM_BASE       (0x08020000)
#else
#define BSP_CFG_KERN_ROM_BASE       (BSP_CFG_ROM_BASE)
#endif
#define BSP_CFG_KERN_ROM_SIZE       (256 * 1024)

#define BSP_CFG_KERN_RAM_BASE       (BSP_CFG_RAM_BASE)
#define BSP_CFG_KERN_RAM_SIZE       (128 * 1024)

#define BSP_CFG_KERN_HEAP_SIZE      (64 * 1024)

#define BSP_CFG_APP_RAM_BASE        (BSP_CFG_KERN_RAM_BASE + BSP_CFG_KERN_RAM_SIZE)
#define BSP_CFG_APP_RAM_SIZE        (128 * 1024)

#define BSP_CFG_SHARED_RAM_BASE     (0)
#define BSP_CFG_SHARED_RAM_SIZE     (0)

#define BSP_CFG_BOOT_STACK_SIZE     (2048)

/*********************************************************************************************************
  升级配置
*********************************************************************************************************/

#define BSP_CFG_UPDATE_REQUEST_PATH "/sd0/update/update_req"
#define BSP_CFG_UPDATE_LOG_PATH     "/sd0/update/update_log"

/*********************************************************************************************************
  驱动配置
*********************************************************************************************************/

#define BSP_CFG_NET_EN              0
#define BSP_CFG_ESP8266_EN          1
#define BSP_CFG_UDISK_EN            0
#define BSP_CFG_WDG_EN              0
#define BSP_CFG_TOUCH_EN            0
#define BSP_CFG_SPI_NOR_EN          0
#define BSP_CFG_SD_EN               1
#define BSP_CFG_FB_EN               0
#define BSP_CFG_FLASH_EN            0
#define BSP_CFG_GPIO_EN             1
#define BSP_CFG_I2C_EN              1
#define BSP_CFG_RTC_EN              0
#define BSP_CFG_UART_EN             1
#define BSP_CFG_HW_TEST_EN          0

#if BSP_CFG_HW_TEST_EN > 0
#undef  BSP_CFG_CONSOLE_DEV
#define BSP_CFG_CONSOLE_DEV         BSP_CONSOLE_UART
#endif

/*********************************************************************************************************
  看门狗配置
*********************************************************************************************************/

#define BSP_CFG_WDG_DEADLINE        (5 * MS_CFG_KERN_TICK_HZ)

/*********************************************************************************************************
  ESP8266 配置
*********************************************************************************************************/

#define BSP_ESP8266_AUTO_JOIN       1
#define BSP_ESP8266_SMART_CFG       2
#define BSP_ESP8266_MANUAL_CFG      4

#define BSP_CFG_ESP8266_MODE        (BSP_ESP8266_AUTO_JOIN)

#define BSP_CFG_ESP8266_UPDATE_FW   0

/*********************************************************************************************************
  FLASH 配置
*********************************************************************************************************/

#define ADDR_FLASH_SECTOR_0         (0x08000000) /* Base address of Sector 0,  16 Kbytes  */
#define ADDR_FLASH_SECTOR_1         (0x08004000) /* Base address of Sector 1,  16 Kbytes  */
#define ADDR_FLASH_SECTOR_2         (0x08008000) /* Base address of Sector 2,  16 Kbytes  */
#define ADDR_FLASH_SECTOR_3         (0x0800C000) /* Base address of Sector 3,  16 Kbytes  */
#define ADDR_FLASH_SECTOR_4         (0x08010000) /* Base address of Sector 4,  64 Kbytes  */
#define ADDR_FLASH_SECTOR_5         (0x08020000) /* Base address of Sector 5,  128 Kbytes */
#define ADDR_FLASH_SECTOR_6         (0x08040000) /* Base address of Sector 6,  128 Kbytes */
#define ADDR_FLASH_SECTOR_7         (0x08060000) /* Base address of Sector 7,  128 Kbytes */
#define ADDR_FLASH_SECTOR_8         (0x08080000) /* Base address of Sector 8,  128 Kbytes */
#define ADDR_FLASH_SECTOR_9         (0x080A0000) /* Base address of Sector 9,  128 Kbytes */
#define ADDR_FLASH_SECTOR_10        (0x080C0000) /* Base address of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11        (0x080E0000) /* Base address of Sector 11, 128 Kbytes */

#define BSP_CFG_FLASHFS_MAIN_TBL_ADDR       ADDR_FLASH_SECTOR_2
#define BSP_CFG_FLASHFS_SEC_TBL_ADDR        ADDR_FLASH_SECTOR_3

#define BSP_CFG_FLASHFS_MAX_FILE            8
#define BSP_CFG_FLASHFS_UNIT_SIZE           (16 * 1024)

/*********************************************************************************************************
  RTC 配置
*********************************************************************************************************/

#define BSP_CFG_RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */
#define BSP_CFG_RTC_SYNCH_PREDIV   0x00FF /* LSE as RTC clock */

#endif                                                                  /*  IOT_PI_CFG_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
