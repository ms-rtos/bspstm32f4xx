/*********************************************************************************************************
**
**                                江苏林洋能源股份有限公司
**
**                                       adc驱动
**
**                                      MS-RTOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: stm32_drv_adc.c
**
** 创   建   人: Wang.Jie
**
** 文件创建日期: 2020 年 08 月 10 日
**
** 描        述: STM32 芯片 ADC 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"
#include "stm32_drv_adc.h"

#if  BSP_CFG_ADC_EN > 0

#define NUCLEO_ADCx_CLK_ENABLE()             __HAL_RCC_ADC1_CLK_ENABLE()
#define NUCLEO_ADCx_CLK_DISABLE()            __HAL_RCC_ADC1_CLK_DISABLE()

#define NUCLEO_ADCx_GPIO_PORT                GPIOC
#define NUCLEO_ADCx_GPIO_PIN                 GPIO_PIN_3
#define NUCLEO_ADCx_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOC_CLK_ENABLE()
#define NUCLEO_ADCx_GPIO_CLK_DISABLE()       __HAL_RCC_GPIOC_CLK_DISABLE()
#define NUCLEO_ADCx_CHANNEL                  ADC_CHANNEL_13

static ADC_HandleTypeDef hnucleo_Adc;

/*
 * Private info
 */
typedef struct {
    ms_addr_t       base;
} privinfo_t;

/*
 * stm32 adc_ex device
 */
typedef struct {
    privinfo_t      priv;
    ms_io_device_t  dev;
} stm32_adc_ex_dev_t;

/*
 * @brief Global data area
 */
static stm32_adc_ex_dev_t __stm32_adc_ex_devs;

/**
 * @brief  Initializes ADC MSP.
 */
static void ADCx_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /*** Configure the GPIOs ***/
    /* Enable GPIO clock */
    NUCLEO_ADCx_GPIO_CLK_ENABLE();

    /* Configure the selected ADC Channel as analog input */
    GPIO_InitStruct.Pin = NUCLEO_ADCx_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(NUCLEO_ADCx_GPIO_PORT, &GPIO_InitStruct);

    /*** Configure the ADC peripheral ***/
    /* Enable ADC clock */
    NUCLEO_ADCx_CLK_ENABLE();
}

/*
 * Open device
 */
static int __stm32_adc_ex_open(ms_ptr_t ctx, ms_io_file_t *file, int oflag, ms_mode_t mode)
{
    privinfo_t *priv = ctx;
    ms_uint8_t status = HAL_ERROR;
    ADC_ChannelConfTypeDef sConfig;

    if (ms_atomic_inc(MS_IO_DEV_REF(file)) == 1) {
        /* ADC Config */
        hnucleo_Adc.Instance = (ADC_TypeDef *)priv->base;
        hnucleo_Adc.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV4; /* (must not exceed 36MHz) */
        hnucleo_Adc.Init.Resolution = ADC_RESOLUTION12b;
        hnucleo_Adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
        hnucleo_Adc.Init.ContinuousConvMode = DISABLE;
        hnucleo_Adc.Init.DiscontinuousConvMode = DISABLE;
        hnucleo_Adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
        hnucleo_Adc.Init.EOCSelection = EOC_SINGLE_CONV;
        hnucleo_Adc.Init.NbrOfConversion = 1;
        hnucleo_Adc.Init.DMAContinuousRequests = DISABLE;

        ADCx_MspInit(&hnucleo_Adc);
        HAL_ADC_Init(&hnucleo_Adc);

        /* Select the ADC Channel to be converted */
        sConfig.Channel = NUCLEO_ADCx_CHANNEL;
        sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
        sConfig.Rank = 1;
        status = HAL_ADC_ConfigChannel(&hnucleo_Adc, &sConfig);
    } else {
        ms_atomic_dec(MS_IO_DEV_REF(file));
        ms_thread_set_errno(EBUSY);
        status = -1;
    }
    /* Return Joystick initialization status */
    return status;
}

static void ADCx_MspDeInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /*** DeInit the ADC peripheral ***/
    /* Disable ADC clock */
    NUCLEO_ADCx_CLK_DISABLE();

    /* Configure the selected ADC Channel as analog input */
    GPIO_InitStruct.Pin = NUCLEO_ADCx_GPIO_PIN;
    HAL_GPIO_DeInit(NUCLEO_ADCx_GPIO_PORT, GPIO_InitStruct.Pin);

    /* Disable GPIO clock has to be done by the application*/
    /* NUCLEO_ADCx_GPIO_CLK_DISABLE(); */
}

/*
 * Close device
 */
static int __stm32_adc_ex_close(ms_ptr_t ctx, ms_io_file_t *file)
{
    privinfo_t *priv = ctx;

    if (ms_atomic_dec(MS_IO_DEV_REF(file)) == 0) {

        hnucleo_Adc.Instance = (ADC_TypeDef *)priv->base;

        HAL_ADC_DeInit(&hnucleo_Adc);
        ADCx_MspDeInit(&hnucleo_Adc);
    }

    return 0;
}

/*
 * Read device
 */
static ms_ssize_t __stm32_adc_ex_read(ms_ptr_t ctx, ms_io_file_t *file, ms_ptr_t buf, ms_size_t len)
{
    privinfo_t *priv = ctx;
    ms_ssize_t ret = 0;
    ms_uint8_t *cbuf=buf;
    ms_uint16_t  keyconvertedvalue = 0;

    if (len >= 2) {
        hnucleo_Adc.Instance = (ADC_TypeDef *)priv->base;

        /* Start the conversion process */
        HAL_ADC_Start(&hnucleo_Adc);

        /* Wait for the end of conversion */
        HAL_ADC_PollForConversion(&hnucleo_Adc, 10);

        /* Check if the continuous conversion of regular channel is finished */
        if (((HAL_ADC_GetState(&hnucleo_Adc) & HAL_ADC_STATE_EOC_REG) == HAL_ADC_STATE_EOC_REG)) {
            /* Get the converted value of regular channel */
            keyconvertedvalue = HAL_ADC_GetValue(&hnucleo_Adc);
            cbuf[0] = keyconvertedvalue & 0xFF;
            cbuf[1] = (keyconvertedvalue >> 8) & 0xFF;
            ret = 2;
        } else {
            ms_thread_set_errno(EIO);
            ret = -1;
        }
    } else {
        ms_thread_set_errno(EFAULT);
        ret = -1;
    }

    return ret;
}

/*
 * Device operating function set
 */
static const ms_io_driver_ops_t stm32_adc_ex_drv_ops = {
    .type   = MS_IO_DRV_TYPE_CHR,
    .open   = __stm32_adc_ex_open,
    .close  = __stm32_adc_ex_close,
    .read   = __stm32_adc_ex_read,
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_adc_ex_drv = {
        .nnode = {
            .name = "stm32_adc_ex",
        },
        .ops = &stm32_adc_ex_drv_ops,
};

/*
 * Register ADC device driver
 */
ms_err_t stm32_adc_ex_drv_register(void)
{
    return ms_io_driver_register(&stm32_adc_ex_drv);
}

/*
 * Create UART device file
 */
ms_err_t stm32_adc_ex_dev_create(const char *path)
{
    stm32_adc_ex_dev_t *adc_ex_dev;
    ms_err_t err = MS_ERR_NONE;

    adc_ex_dev = &__stm32_adc_ex_devs;
    bzero(&adc_ex_dev->priv, sizeof(adc_ex_dev->priv));

    adc_ex_dev->priv.base = (ms_addr_t)ADC1;

    err = ms_io_device_register(&adc_ex_dev->dev, path, "stm32_adc_ex", &adc_ex_dev->priv);

    return err;
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
