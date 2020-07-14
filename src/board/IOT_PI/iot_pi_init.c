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
** 文   件   名: iot_pi_init.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi 初始化
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"
#include "ms_fatfs.h"
#include "ms_net_lwip.h"
#include "ms_net_esp_at.h"
#include "eeprom/ms_drv_xx24xx.h"
#include "includes.h"
#include "stm32_drv.h"
#include <string.h>

#if BSP_CFG_USE_BOOTLOADER == 0U
/**
 * @brief  System Clock Configuration
 * @param  None
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    HAL_StatusTypeDef ret = HAL_OK;

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 100;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    RCC_OscInitStruct.PLL.PLLR = 2;
    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if(ret != HAL_OK)
    {
        while (1);
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
       clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);
    if(ret != HAL_OK)
    {
        while (1);
    }
}
#endif

/**
 * @brief Initial printk interface.
 *
 * @return N/A
 */
void ms_bsp_printk_init(void)
{
#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
    UART_HandleTypeDef UartHandle;

    UartHandle.Instance          = USART1;

    UartHandle.Init.BaudRate     = 115200U;
    UartHandle.Init.WordLength   = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits     = UART_STOPBITS_1;
    UartHandle.Init.Parity       = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode         = UART_MODE_TX_RX;
    UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&UartHandle);
#endif
}

/**
 * @brief Kernel info print.
 *
 * @param[in] buf Pointer to content which need to be print
 * @param[in] len The length of buffer
 *
 * @return N/A
 */
void ms_bsp_printk(const char *buf, ms_size_t len)
{
#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
    ms_size_t i;

    for (i = 0U; i < len; i++) {
        while (!(USART1->SR & UART_FLAG_TXE)) {
        }
        USART1->DR = *buf++;
    }
#else
    ms_trace_write(buf, len);
#endif
}

/**
 * @brief Reboot.
 *
 * @return N/A
 */
void ms_bsp_reboot(void)
{
    __set_FAULTMASK(1);
    NVIC_SystemReset();

    while (MS_TRUE);
}

/**
 * @brief Shutdown.
 *
 * @return N/A
 */
void ms_bsp_shutdown(void)
{
    ms_bsp_reboot();
}

#if (MS_CFG_SHELL_MODULE_EN > 0) &&  (BSP_CFG_CONSOLE_DEV != BSP_CONSOLE_NULL)
/**
 * @brief Shell thread.
 *
 * @param[in] arg Shell thread argument
 *
 * @return N/A
 */
static void shell_thread(ms_ptr_t arg)
{
    extern unsigned long __ms_shell_cmd_start__;
    extern unsigned long __ms_shell_cmd_end__;

    ms_shell_io_t bsp_shell_io = {
            (ms_shell_cmd_t *)&__ms_shell_cmd_start__,
            (ms_shell_cmd_t *)&__ms_shell_cmd_end__ - (ms_shell_cmd_t *)&__ms_shell_cmd_start__,
#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_TRACE)
            ms_trace_getc,
            ms_trace_putc,
            ms_trace_write,
            ms_trace_printf,
#elif (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
            ms_getc,
            ms_putc,
            ms_write_stdout,
            ms_printf,
            ms_gets,
#endif
    };

    while (MS_TRUE) {
        ms_shell_enter(&bsp_shell_io);
    }
}
#endif

#if BSP_CFG_ESP8266_EN > 0 && BSP_CFG_ESP8266_UPDATE_FW == 0
#if (BSP_CFG_ESP8266_MODE & BSP_ESP8266_MANUAL_CFG) != 0
/**
 * WiFi AP list.
 */
static const ms_esp_at_net_ap_t ap_list[] = {
    { "Tenda_yu",       "Yubei8686" },
    { "realtek8192",    "123456789" },
    { "ACOINFO",        "yihui87654321" },
};
#endif

/**
 * @brief ESP AT net initial done callback.
 *
 * @param[in] arg Callback function argument
 *
 * @return N/A
 */
static void ms_esp_at_net_init_done(ms_ptr_t arg)
{
#if BSP_CFG_HW_TEST_EN == 0
    ms_err_t ret = MS_ERR;
    ms_esp_at_net_ap_t ap;

#if (BSP_CFG_ESP8266_MODE & BSP_ESP8266_AUTO_JOIN) != 0
    ret = ms_esp_at_auto_join(4U, &ap);
#endif

#if (BSP_CFG_ESP8266_MODE & BSP_ESP8266_MANUAL_CFG) != 0
    if (ret != MS_ERR_NONE) {
        ms_esp_at_connect_to_ap(1U, ap_list, MS_ARRAY_SIZE(ap_list), &ap);
    }
#elif (BSP_CFG_ESP8266_MODE & BSP_ESP8266_SMART_CFG) != 0
    if (ret != MS_ERR_NONE) {
        ms_esp_at_smart_config(40U, &ap);
    }
#else
    (void)ret;
#endif
#endif
}
#endif

#if MS_CFG_SHELL_MODULE_EN > 0
/**
 * @brief smartcfg command.
 *
 * @param[in] argc Arguments count
 * @param[in] argv Arguments array
 * @param[in] io Pointer to shell io driver
 *
 * @return N/A
 */
static void __ms_shell_esp8266_smartcfg(int argc, char *argv[], const ms_shell_io_t *io)
{
    ms_esp_at_net_ap_t ap;

    ms_esp_at_smart_config(40U, &ap);
}

MS_SHELL_CMD(smartcfg, __ms_shell_esp8266_smartcfg, "ESP8266 smart configure", __ms_shell_cmd_smartcfg);
#endif

/**
 * @brief Boot thread.
 *
 * @param[in] arg Boot thread argument
 *
 * @return N/A
 */
static void boot_thread(ms_ptr_t arg)
{
    ms_printk_set_level(MS_PK_INFO);

    ms_pipe_drv_register();
    ms_shm_drv_register();

#if BSP_CFG_GPIO_EN > 0
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    stm32_gpio_drv_register();
    stm32_gpio_dev_create("/dev/led1", (ms_addr_t)GPIOA, GPIO_PIN_1);
    stm32_gpio_dev_create("/dev/led2", (ms_addr_t)GPIOB, GPIO_PIN_7);
    stm32_gpio_dev_create("/dev/led3", (ms_addr_t)GPIOC, GPIO_PIN_13);
    stm32_gpio_dev_create("/dev/key1", (ms_addr_t)GPIOC, GPIO_PIN_7);
    stm32_gpio_dev_create("/dev/key2", (ms_addr_t)GPIOC, GPIO_PIN_2);
    stm32_gpio_dev_create("/dev/key3", (ms_addr_t)GPIOB, GPIO_PIN_6);
#endif

#if BSP_CFG_I2C_EN > 0
    stm32_i2c_bus_dev_create("/dev/i2c3", 3U);

    ms_xx24xx_drv_register();
    ms_xx24xx_dev_create("/dev/eeprom", "i2c3", 0x50U, EEPROM_24XX02);
#endif

#if BSP_CFG_RTC_EN > 0
    ms_rtc_drv_register();
    stm32_rtc_dev_create();
#endif

#if BSP_CFG_SD_EN > 0
    ms_fatfs_register();
    stm32_sd_drv_register();
#endif

#if BSP_CFG_FLASH_EN > 0
    ms_flashfs_register();
    stm32_flash_mount("/flash");

    ms_apps_update(BSP_CFG_UPDATE_REQUEST_PATH, BSP_CFG_UPDATE_LOG_PATH);
#endif

#if BSP_CFG_ESP8266_EN > 0
#if BSP_CFG_ESP8266_UPDATE_FW == 0
    ms_esp_at_net_init(ms_esp_at_net_init_done, MS_NULL);
#else
    extern void esp8266_update_fw_init(void);
    ms_printk(MS_PK_INFO, "ESP8266 update mode...\n");
    esp8266_update_fw_init();
#endif
#endif

#if BSP_CFG_WDG_EN > 0
    ms_wdg_drv_register();
    ms_wdg_dev_register("/dev/wdg", stm32_wdg_drv());
#endif

#if BSP_CFG_FLASH_EN > 0
    ms_apps_start("ms-boot-param.dtb");
#endif

    ms_process_create("iotpi_sddc", (ms_const_ptr_t)0x08040000, 65536, 4096, 9, 0 , 0, MS_NULL, MS_NULL, MS_NULL);
}

#if BSP_CFG_HW_TEST_EN > 0
/**
 * @brief Hardware test thread.
 *
 * @param[in] arg Boot thread argument
 *
 * @return N/A
 */
static void test_thread(ms_ptr_t arg)
{
    ms_thread_sleep_s(1);

    /*
     * EEPROM test
     */
    ms_printf("**************************************************************************\n");
    ms_printf("                             EEPROM TEST\n");
    ms_printf("**************************************************************************\n");
    {
        int efd = ms_io_open("/dev/eeprom", O_RDWR, 0666);
        ms_eeprom_msg_t emsg;
        ms_uint8_t buf[16];

        emsg.memaddr = 0;
        emsg.buf = buf;
        emsg.len = sizeof(buf);

        memset(buf, 0xaa, sizeof(buf));

        if (ms_io_write(efd, &emsg, sizeof(emsg)) == sizeof(emsg)) {
            ms_printf("EEPROM write ok\n");
        } else {
            ms_printf("EEPROM write failed\n");
        }

        emsg.memaddr = 0;
        emsg.buf = buf;
        emsg.len = sizeof(buf);

        memset(buf, 0, sizeof(buf));

        if (ms_io_read(efd, &emsg, sizeof(emsg)) == sizeof(emsg)) {
            ms_printf("EEPROM read ok\n");
        } else {
            ms_printf("EEPROM read failed\n");
        }

        int i;
        for (i = 0; i < sizeof(buf); i++) {
            if (buf[i] != 0xaa) {
                ms_printf("EEPROM compare failed\n");
                break;
            }
        }

        if (i == sizeof(buf)) {
            ms_printf("EEPROM compare ok\n");
        }

        ms_io_close(efd);
    }

    /*
     * SDCARD test
     */
    ms_printf("**************************************************************************\n");
    ms_printf("                             SDCARD TEST\n");
    ms_printf("**************************************************************************\n");
    {
        MS_DIR *dir;
        ms_dirent_t dirent;
        ms_dirent_t *result;
        int ret;

        dir = ms_io_opendir("/sd0");
        if (dir != MS_NULL) {
            do {
                ret = ms_io_readdir_r(dir, &dirent, &result);
                if ((ret > 0) && (result != MS_NULL)) {
                    if ((strcmp(result->d_name, ".") == 0) || (strcmp(result->d_name, "..") == 0)) {
                        continue;
                    }

                    ms_printf("%s\n", result->d_name);

                } else {
                    break;
                }
            } while (MS_TRUE);

            ms_io_closedir(dir);

        } else {
            ms_printf("SDCARD open failed!\n");
        }
    }

    ms_printf("**************************************************************************\n");
    ms_printf("                             LED KEY TEST\n");
    ms_printf("**************************************************************************\n");
    {
        /* Enable GPIO TX/RX clock */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        typedef struct {
            const char *name;
            GPIO_TypeDef *gpiox;
            ms_uint16_t   pin;
            ms_bool_t     state;
        } gpio_info_t;

        gpio_info_t gpio_info[] = {
                { "PB9",  GPIOB, GPIO_PIN_9,  MS_TRUE },
                { "PB8",  GPIOB, GPIO_PIN_8,  MS_TRUE },
                { "PA2",  GPIOA, GPIO_PIN_2,  MS_TRUE },
                { "PA3",  GPIOA, GPIO_PIN_3,  MS_TRUE },
                { "PA4",  GPIOA, GPIO_PIN_4,  MS_TRUE },
                { "PA5",  GPIOA, GPIO_PIN_5,  MS_TRUE },
                { "PA6",  GPIOA, GPIO_PIN_6,  MS_TRUE },
                { "PA7",  GPIOA, GPIO_PIN_7,  MS_TRUE },
                { "PB10", GPIOB, GPIO_PIN_10, MS_TRUE },
                { "PB12", GPIOB, GPIO_PIN_12, MS_TRUE },
                { "PB14", GPIOB, GPIO_PIN_14, MS_TRUE },
                { "PB15", GPIOB, GPIO_PIN_15, MS_TRUE },
                { "PC6",  GPIOC, GPIO_PIN_6,  MS_TRUE },
                { "PC14", GPIOC, GPIO_PIN_14, MS_TRUE },
                { "PC15", GPIOC, GPIO_PIN_15, MS_TRUE },
                { "PC0",  GPIOC, GPIO_PIN_0,  MS_TRUE },
                { "PC1",  GPIOC, GPIO_PIN_1,  MS_TRUE },
                { "PB13", GPIOB, GPIO_PIN_13, MS_TRUE },
                { "PC3",  GPIOC, GPIO_PIN_3,  MS_TRUE },
                { "PA0",  GPIOA, GPIO_PIN_0,  MS_TRUE },
                { "PB1",  GPIOB, GPIO_PIN_1,  MS_TRUE },
                { "PB2",  GPIOB, GPIO_PIN_2,  MS_TRUE },
                { "PB0",  GPIOB, GPIO_PIN_0,  MS_TRUE },
                { "PC5",  GPIOC, GPIO_PIN_5,  MS_TRUE },
                { "PC4",  GPIOC, GPIO_PIN_4,  MS_TRUE },
        };

        int led1 = ms_io_open("/dev/led1", O_WRONLY, 0666);
        int led2 = ms_io_open("/dev/led2", O_WRONLY, 0666);
        int led3 = ms_io_open("/dev/led3", O_WRONLY, 0666);

        int key1 = ms_io_open("/dev/key1", O_WRONLY, 0666);
        int key2 = ms_io_open("/dev/key2", O_WRONLY, 0666);
        int key3 = ms_io_open("/dev/key3", O_WRONLY, 0666);

        ms_gpio_param_t param;

        param.mode  = MS_GPIO_MODE_OUTPUT_PP;
        param.pull  = MS_GPIO_PULL_UP;
        param.speed = MS_GPIO_SPEED_HIGH;
        ms_io_ioctl(led1, MS_GPIO_CMD_SET_PARAM, &param);
        ms_io_ioctl(led2, MS_GPIO_CMD_SET_PARAM, &param);
        ms_io_ioctl(led3, MS_GPIO_CMD_SET_PARAM, &param);

        param.mode  = MS_GPIO_MODE_IRQ_FALLING;
        param.pull  = MS_GPIO_PULL_UP;
        param.speed = MS_GPIO_SPEED_HIGH;
        ms_io_ioctl(key1, MS_GPIO_CMD_SET_PARAM, &param);
        ms_io_ioctl(key2, MS_GPIO_CMD_SET_PARAM, &param);
        ms_io_ioctl(key3, MS_GPIO_CMD_SET_PARAM, &param);

        fd_set rfds;
        struct timeval tv;

        ms_uint8_t led1_val = 0xff;
        ms_uint8_t led2_val = 0xff;
        ms_uint8_t led3_val = 0xff;

        ms_bool_t gpio_test_start = MS_FALSE;
        int i = 0;

        while (1) {
            FD_ZERO(&rfds);
            FD_SET(key1, &rfds);
            FD_SET(key2, &rfds);
            FD_SET(key3, &rfds);

            tv.tv_sec = 0;
            tv.tv_usec = 5 * 1000;

            if (select(key3 + 1, &rfds, MS_NULL, MS_NULL, &tv) > 0) {
                if (FD_ISSET(key1, &rfds)) {
                    ms_io_write(led1, &led1_val, sizeof(led1_val));
                    led1_val = ~led1_val;
                    ms_printf("key1 press, GPIO TEST START....\n");
                    gpio_test_start = MS_TRUE;

                    gpio_info[i].state = MS_TRUE;
                    HAL_GPIO_WritePin(gpio_info[i].gpiox, gpio_info[i].pin, gpio_info[i].state);

                    i = 0;
                }

                if (FD_ISSET(key2, &rfds)) {
                    ms_io_write(led2, &led2_val, sizeof(led2_val));
                    led2_val = ~led2_val;
                    ms_printf("key2 press\n");
                }

                if (FD_ISSET(key3, &rfds)) {
                    ms_io_write(led3, &led3_val, sizeof(led3_val));
                    led3_val = ~led3_val;
                    ms_printf("key3 press\n");
                }
            } else if (gpio_test_start) {
                if (gpio_info[i].state) {
                    GPIO_InitTypeDef GPIO_InitStruct;

                    bzero(&GPIO_InitStruct, sizeof(GPIO_InitTypeDef));

                    GPIO_InitStruct.Pin       = gpio_info[i].pin;
                    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
                    GPIO_InitStruct.Pull      = GPIO_NOPULL;
                    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
                    HAL_GPIO_Init(gpio_info[i].gpiox, &GPIO_InitStruct);

                    gpio_info[i].state = MS_FALSE;
                    HAL_GPIO_WritePin(gpio_info[i].gpiox, gpio_info[i].pin, gpio_info[i].state);

                    ms_printf("%s on\n", gpio_info[i].name);

                } else {
                    gpio_info[i].state = MS_TRUE;
                    HAL_GPIO_WritePin(gpio_info[i].gpiox, gpio_info[i].pin, gpio_info[i].state);

                    ms_printf("%s off\n", gpio_info[i].name);

                    i++;
                    i %= MS_ARRAY_SIZE(gpio_info);
                }
            }
        }
    }
}
#endif

/**
 * @brief BSP application initial.
 *
 * @return N/A
 */
static void ms_app_init(void)
{
#if (MS_CFG_SHELL_MODULE_EN > 0) &&  (BSP_CFG_CONSOLE_DEV != BSP_CONSOLE_NULL)
    ms_thread_create("t_shell",
                     shell_thread,
                     MS_NULL,
                     2048U,
                     MS_CFG_KERN_LOWEST_PRIO - 1U,
                     70U,
                     MS_THREAD_OPT_SUPER | MS_THREAD_OPT_REENT_EN,
                     MS_NULL);
#endif

    ms_thread_create("t_boot",
                     boot_thread,
                     MS_NULL,
                     4096U,
                     4U,
                     70U,
                     MS_THREAD_OPT_SUPER | MS_THREAD_OPT_REENT_EN,
                     MS_NULL);

#if BSP_CFG_HW_TEST_EN > 0
    ms_thread_create("t_test",
                     test_thread,
                     MS_NULL,
                     2048U,
                     MS_CFG_KERN_LOWEST_PRIO - 2U,
                     70U,
                     MS_THREAD_OPT_SUPER | MS_THREAD_OPT_REENT_EN,
                     MS_NULL);
#endif
}

/**
 * @brief BSP initial.
 *
 * @return N/A
 */
void bsp_init(void)
{
    extern unsigned long __ms_kern_heap_start__;
    extern unsigned long __ms_kern_text_start__;
    extern unsigned long __ms_kern_text_end__;

    ms_mem_layout_t mem_layout[] = {
            [MS_FLASH_REGION] = {
                    BSP_CFG_ROM_BASE,
                    BSP_CFG_ROM_SIZE,
            },
            [MS_KERN_TEXT_REGION] = {
                    (ms_addr_t)&__ms_kern_text_start__,
                    (ms_addr_t)&__ms_kern_text_end__ - (ms_addr_t)&__ms_kern_text_start__,
            },
            [MS_KERN_DATA_REGION] = {
                    BSP_CFG_KERN_RAM_BASE,
                    BSP_CFG_KERN_RAM_SIZE,
            },
            [MS_KERN_HEAP_REGION] = {
                    (ms_addr_t)&__ms_kern_heap_start__,
                    BSP_CFG_KERN_HEAP_SIZE,
            },
            [MS_SHARED_MEM_REGION] = {
                    BSP_CFG_SHARED_RAM_BASE,
                    BSP_CFG_SHARED_RAM_SIZE,
            },
            [MS_PROCESS_MEM_REGION] = {
                    BSP_CFG_APP_RAM_BASE,
                    BSP_CFG_APP_RAM_SIZE,
            },
    };

    ms_rtos_init(mem_layout);

    ms_null_drv_register();

#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
    ms_ptr_t pty_dev;
    ms_pty_drv_register();
    ms_pty_dev_create("/dev/console", 128U, &pty_dev);
#elif (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_TRACE)
    ms_trace_drv_register();
    ms_trace_dev_register("/dev/console");
#else
    ms_null_dev_create("/dev/console");
#endif

    ms_stdfile_init();

#if BSP_CFG_UART_EN > 0
    stm32_uart_drv_register();
    stm32_uart_dev_create("/dev/uart0", 1, 256, 256);
#endif

#if (BSP_CFG_CONSOLE_DEV == BSP_CONSOLE_UART)
    int uart_fd = ms_io_open("/dev/uart0", O_RDWR, 0666);
    ms_uart_param_t param;

    ms_io_ioctl(uart_fd, MS_UART_CMD_GET_PARAM, &param);
    param.baud      = 115200U;
    param.data_bits = MS_UART_DATA_BITS_8B;
    param.stop_bits = MS_UART_STOP_BITS_1B;
    param.parity    = MS_UART_PARITY_NONE;
    ms_io_ioctl(uart_fd, MS_UART_CMD_SET_PARAM, &param);

    ms_pty_dev_start(pty_dev, uart_fd, 1024U, 4U, 0U, 0U);
#endif

    ms_app_init();

    ms_rtos_start();

    while (MS_TRUE) {
    }
}

#if BSP_CFG_I2C_EN > 0

/**
  * @brief I2C MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  *           - DMA configuration for transmission request by peripheral
  *           - NVIC configuration for DMA interrupt request enable
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    if (hi2c->Instance == I2C3) {
        /* Enable GPIO TX/RX clock */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* Enable I2C3 clock */
        __HAL_RCC_I2C3_CLK_ENABLE();

        /* I2C TX GPIO pin configuration  */
        GPIO_InitStruct.Pin       = GPIO_PIN_8;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;

        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* I2C TX GPIO pin configuration  */
        GPIO_InitStruct.Pin       = GPIO_PIN_4;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
        GPIO_InitStruct.Alternate = GPIO_AF9_I2C3;

        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

/**
  * @brief I2C MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO, DMA and NVIC configuration to their default state
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C3) {
        __HAL_RCC_I2C3_FORCE_RESET();
        __HAL_RCC_I2C3_RELEASE_RESET();

        /* Configure I2C Tx as alternate function  */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4);
    }
}

#endif

/*********************************************************************************************************
  END
*********************************************************************************************************/
