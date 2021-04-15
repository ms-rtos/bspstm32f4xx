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
** 文   件   名: stm32_drv_spi.c
**
** 创   建   人:
**
** 文件创建日期: 2020 年 06 月 01 日
**
** 描        述: STM32 芯片 SPI 总线驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_kern.h"
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>
#include "includes.h"
#include "stm32_drv_spi.h"

#if BSP_CFG_SPI_EN > 0

#define __DIV_ROUND_UP(n, d)          (((n) + (d) - 1) / (d))
#define __STM32_SPI_MAX_CHANNEL       (6U)

/*
 * Private info
 */
typedef struct {
    SPI_HandleTypeDef spi_handler;
    ms_uint8_t        trans_mode;
    IRQn_Type         irq;
    ms_spi_param_t    param;
    ms_handle_t       sembid;
} privinfo_t;

/*
 * SPI baud rate to prescaler
 */
static void __stm32_spi_prescaler_set(SPI_TypeDef *instance, ms_uint32_t baud_rate, ms_uint32_t *prescaler)
{
    int div;

    if (instance == (SPI_TypeDef *)SPI2_BASE || instance == (SPI_TypeDef *)SPI3_BASE) {
        div = __DIV_ROUND_UP(HAL_RCC_GetPCLK1Freq(), baud_rate);
    } else {
        div = __DIV_ROUND_UP(HAL_RCC_GetPCLK2Freq(), baud_rate);
    }

    if (div <= 2) {
        *prescaler = SPI_BAUDRATEPRESCALER_2;
    } else if (div > 2 && div <= 4) {
        *prescaler = SPI_BAUDRATEPRESCALER_4;
    } else if (div > 4 && div <= 8) {
        *prescaler = SPI_BAUDRATEPRESCALER_8;
    } else if (div > 8 && div <= 16) {
        *prescaler = SPI_BAUDRATEPRESCALER_16;
    } else if (div > 16 && div <= 32) {
        *prescaler = SPI_BAUDRATEPRESCALER_32;
    } else if (div > 32 && div <= 64) {
        *prescaler = SPI_BAUDRATEPRESCALER_64;
    } else if (div > 64 && div <= 128) {
        *prescaler = SPI_BAUDRATEPRESCALER_128;
    } else if (div > 128) {
        *prescaler = SPI_BAUDRATEPRESCALER_256;
    }
}
/*
 * SPI interrupt request setting
 */
static void __stm32_spi_bus_int_set(privinfo_t *priv, ms_bool_t enable)
{
    if (enable) {
        HAL_NVIC_EnableIRQ(priv->irq);
    } else {
        HAL_NVIC_DisableIRQ(priv->irq);
    }
}

/*
 *  Convert ms_spi_param_t to native structure spi_init_t
 */
static int __stm32_spi_bus_param_convert(ms_spi_param_t *param, SPI_InitTypeDef *config)
{
    config->CRCPolynomial = 7;

    if (param->mode == MS_SPI_MODE_MASTER) {
        config->Mode = SPI_MODE_MASTER;
    } else if (param->mode == MS_SPI_MODE_SLAVE) {
        config->Mode = SPI_MODE_SLAVE;
    } else {
        return MS_ERR;
    }

    if (param->direction == MS_SPI_DIRECTION_2LINES) {
        config->Direction = SPI_DIRECTION_2LINES;
    } else if (param->direction == MS_SPI_DIRECTION_2LINES_RXONLY) {
        config->Direction = SPI_DIRECTION_2LINES_RXONLY;
    } else if (param->direction == MS_SPI_DIRECTION_1LINE) {
        config->Direction = SPI_DIRECTION_1LINE;
    } else {
        return MS_ERR;
    }

    if (param->data_size == MS_SPI_DATA_SIZE_8BIT) {
        config->DataSize = SPI_DATASIZE_8BIT;
    } else if (param->data_size == MS_SPI_DATA_SIZE_16BIT) {
        config->DataSize = SPI_DATASIZE_16BIT;
    } else {
        return MS_ERR;
    }

    if (param->frame_mode & MS_SPI_TI_MODE_ENABLE) {
        config->TIMode = SPI_TIMODE_ENABLE;
    } else {
        config->TIMode = SPI_TIMODE_DISABLE;

        if (param->frame_mode & MS_SPI_CLK_POLARITY_HIGH) {
            config->CLKPolarity = SPI_POLARITY_HIGH;
        } else {
            config->CLKPolarity = SPI_POLARITY_LOW;
        }

        if (param->frame_mode & MS_SPI_CLK_PHASE_2EDGE) {
            config->CLKPhase = SPI_PHASE_2EDGE;
        } else {
            config->CLKPhase = SPI_PHASE_1EDGE;
        }

        if (param->frame_mode & MS_SPI_FIRST_BIT_LSB) {
            config->FirstBit = SPI_FIRSTBIT_LSB;
        } else {
            config->FirstBit = SPI_FIRSTBIT_MSB;
        }
    }

    if (param->frame_mode & MS_SPI_CRC_CALC_ENABLE) {
        config->CRCCalculation = SPI_CRCCALCULATION_ENABLE;
    } else {
        config->CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    }

    if (param->nss == MS_SPI_NSS_SOFT) {
        config->NSS = SPI_NSS_SOFT;
    } else if (param->nss == MS_SPI_NSS_HARD_INPUT) {
        config->NSS = SPI_NSS_HARD_INPUT;
    } else if (param->nss == MS_SPI_NSS_HARD_OUTPUT) {
        config->NSS = SPI_NSS_HARD_OUTPUT;
    } else {
        return MS_ERR;
    }

    return MS_ERR_NONE;
}

/*
 * SPI transmission in poll mode
 */
static ms_ssize_t __stm32_spi_bus_trans_poll(privinfo_t *priv, ms_spi_cs_func_t cs, const ms_spi_msg_t *msg, ms_size_t n_msg)
{
    ms_uint32_t i;

    for (i = 0; i < n_msg; i++, msg++) {
        if (msg->flags & MS_SPI_M_BEGIN) {
            if (cs != MS_NULL) {
                cs(MS_TRUE);
            }
        }

        if (msg->flags & MS_SPI_M_WRITE && msg->flags & MS_SPI_M_READ) {
            if (HAL_SPI_TransmitReceive(&priv->spi_handler, (uint8_t *)msg->tx_buf, msg->rx_buf, msg->len, 2000) != MS_ERR_NONE ) {
                goto error;
            }

        } else if (msg->flags & MS_SPI_M_WRITE) {
            if (HAL_SPI_Transmit(&priv->spi_handler, (uint8_t *)msg->tx_buf, msg->len, 2000) != MS_ERR_NONE) {
                goto error;
            }

        } else if (msg->flags & MS_SPI_M_READ) {
            if (HAL_SPI_Receive(&priv->spi_handler, msg->rx_buf, msg->len, 2000) != MS_ERR_NONE) {
                goto error;
            }
        }

        if (msg->flags & MS_SPI_M_END) {
            if (cs != MS_NULL) {
                cs(MS_FALSE);
            }
        }
    }

    return i;

error:
    if (cs != MS_NULL) {
        cs(MS_FALSE);
    }

    return i;
}

/*
 * SPI transmission in IT mode
 */
static ms_ssize_t __stm32_spi_bus_trans_it(privinfo_t *priv, ms_spi_cs_func_t cs, const ms_spi_msg_t *msg, ms_size_t n_msg)
{
    ms_uint32_t i;

    for (i = 0; i < n_msg; i++, msg++) {
        if (msg->flags & MS_SPI_M_BEGIN) {
            if (cs != MS_NULL) {
                cs(MS_TRUE);
            }
        }

        if (msg->flags & MS_SPI_M_WRITE && msg->flags & MS_SPI_M_READ) {
            if (HAL_SPI_TransmitReceive_IT(&priv->spi_handler, (uint8_t *)msg->tx_buf, msg->rx_buf, msg->len) != MS_ERR_NONE ) {
                goto error;
            }

        } else if (msg->flags & MS_SPI_M_WRITE) {
            if (HAL_SPI_Transmit_IT(&priv->spi_handler, (uint8_t *)msg->tx_buf, msg->len) != MS_ERR_NONE) {
                goto error;
            }

        } else if (msg->flags & MS_SPI_M_READ) {
            if (HAL_SPI_Receive_IT(&priv->spi_handler, msg->rx_buf, msg->len) != MS_ERR_NONE) {
                goto error;
            }
        }

        ms_semb_wait(priv->sembid, MS_TIMEOUT_FOREVER);

        if (msg->flags & MS_SPI_M_END) {
            if (cs != MS_NULL) {
                cs(MS_FALSE);
            }
        }
    }

    return i;

error:
    if (cs != MS_NULL) {
        cs(MS_FALSE);
    }

    return i;
}

/*
 * SPI transmission in DMA mode
 */
static ms_ssize_t __stm32_spi_bus_trans_dma(privinfo_t *priv, ms_spi_cs_func_t cs, const ms_spi_msg_t *msg, ms_size_t n_msg)
{
    ms_uint32_t i;

    for (i = 0; i < n_msg; i++, msg++) {
        if (msg->flags & MS_SPI_M_BEGIN) {
            if (cs != MS_NULL) {
                cs(MS_TRUE);
            }
        }

        if (msg->len >= MS_ARCH_CACHE_LINE_SIZE) {
            if (msg->flags & MS_SPI_M_WRITE && msg->flags & MS_SPI_M_READ) {
                if (HAL_SPI_TransmitReceive_DMA(&priv->spi_handler, (uint8_t *)msg->tx_buf, msg->rx_buf, msg->len) != MS_ERR_NONE ) {
                    goto error;
                }

            } else if (msg->flags & MS_SPI_M_WRITE) {
                if (HAL_SPI_Transmit_DMA(&priv->spi_handler, (uint8_t *)msg->tx_buf, msg->len) != MS_ERR_NONE) {
                    goto error;
                }

            } else if (msg->flags & MS_SPI_M_READ) {
                if (HAL_SPI_Receive_DMA(&priv->spi_handler, msg->rx_buf, msg->len) != MS_ERR_NONE) {
                    goto error;
                }
            }

            ms_semb_wait(priv->sembid, MS_TIMEOUT_FOREVER);

        } else {
            if (msg->flags & MS_SPI_M_WRITE && msg->flags & MS_SPI_M_READ) {
                if (HAL_SPI_TransmitReceive(&priv->spi_handler, (uint8_t *)msg->tx_buf, msg->rx_buf, msg->len, 2000) != MS_ERR_NONE ) {
                    goto error;
                }

            } else if (msg->flags & MS_SPI_M_WRITE) {
                if (HAL_SPI_Transmit(&priv->spi_handler, (uint8_t *)msg->tx_buf, msg->len, 2000) != MS_ERR_NONE) {
                    goto error;
                }

            } else if (msg->flags & MS_SPI_M_READ) {
                if (HAL_SPI_Receive(&priv->spi_handler, msg->rx_buf, msg->len, 2000) != MS_ERR_NONE) {
                    goto error;
                }
            }
        }

        if (msg->flags & MS_SPI_M_END) {
            if (cs != MS_NULL) {
                cs(MS_FALSE);
            }
        }
    }

    return i;

error:
    if (cs != MS_NULL) {
        cs(MS_FALSE);
    }

    return i;
}

static ms_ssize_t __stm32_spi_bus_trans(ms_ptr_t bus_ctx, ms_spi_cs_func_t cs, const ms_spi_msg_t *msg, ms_size_t n_msg)
{
    privinfo_t *priv = bus_ctx;
    ms_ssize_t  ret;

    if (priv->trans_mode == SPI_IT_MODE) {
        ret = __stm32_spi_bus_trans_it(priv, cs, msg, n_msg);

    } else if (priv->trans_mode == SPI_DMA_MODE) {
        ret = __stm32_spi_bus_trans_dma(priv, cs, msg, n_msg);

    } else {
        ret = __stm32_spi_bus_trans_poll(priv, cs, msg, n_msg);
    }

    return ret;
}
/*
 * SPI ioctl
 */
static int __stm32_spi_bus_ioctl(ms_ptr_t bus_ctx, int cmd, ms_ptr_t arg)
{
    privinfo_t *priv = bus_ctx;
    int ret;

    switch (cmd) {
    case MS_SPI_CMD_GET_PARAM:
        if (ms_access_ok(arg, sizeof(ms_spi_param_t), MS_ACCESS_W)) {
            *(ms_spi_param_t *)arg = priv->param;
            ret = 0;
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_SPI_CMD_SET_PARAM:
        if (ms_access_ok(arg, sizeof(ms_spi_param_t), MS_ACCESS_W)) {
            priv->param = *(ms_spi_param_t *)arg;

            if (__stm32_spi_bus_param_convert(&priv->param, &(priv->spi_handler.Init)) == MS_ERR_NONE) {
                __stm32_spi_prescaler_set(priv->spi_handler.Instance, priv->param.baud_rate,
                                            (ms_uint32_t *)&(priv->spi_handler.Init.BaudRatePrescaler));
                __stm32_spi_bus_int_set(priv, MS_FALSE);
                HAL_SPI_DeInit(&priv->spi_handler);
                HAL_SPI_Init(&priv->spi_handler);
                if (priv->trans_mode == SPI_IT_MODE ||
                    priv->trans_mode == SPI_DMA_MODE) {
                    __stm32_spi_bus_int_set(priv, MS_TRUE);
                }

                ret = 0;
            } else {
                ms_thread_set_errno(EINVAL);
                ret = -1;
            }
        } else {
            ms_thread_set_errno(EINVAL);
            ret = -1;
        }
        break;

    default:
        ms_thread_set_errno(EINVAL);
        ret = -1;
    }

    return ret;
}

/*
 * SPI operations
 */
static ms_spi_bus_ops_t __stm32_spi_bus_ops = {
        .trans = __stm32_spi_bus_trans,
        .ioctl = __stm32_spi_bus_ioctl,
};

/*
 *  SPI context
 */
static privinfo_t __stm32_spi_bus_privinfo[__STM32_SPI_MAX_CHANNEL] = {
#ifdef SPI1_BASE
        {
                .spi_handler = {
                        .Instance = (SPI_TypeDef *) SPI1_BASE
                },
                .irq = SPI1_IRQn,
        },
#ifdef SPI2_BASE
        {
                .spi_handler = {
                        .Instance = (SPI_TypeDef *) SPI2_BASE
                },
                .irq = SPI2_IRQn,
        },
#ifdef SPI3_BASE
        {
                .spi_handler = {
                        .Instance = (SPI_TypeDef *) SPI3_BASE
                },
                .irq = SPI3_IRQn,
        },
#ifdef SPI4_BASE
        {
                .spi_handler = {
                        .Instance = (SPI_TypeDef *) SPI4_BASE
                },
                .irq = SPI4_IRQn,
        },
#ifdef SPI5_BASE
        {
                .spi_handler = {
                        .Instance = (SPI_TypeDef *) SPI5_BASE
                },
                .irq = SPI5_IRQn,
        },
#ifdef SPI6_BASE
        {
                .spi_handler = {
                        .Instance = (SPI_TypeDef *) SPI6_BASE
                },
                .irq = SPI6_IRQn,
        },
#endif
#endif
#endif
#endif
#endif
#endif
};

/*
 * SPI bus info
 */
static ms_spi_bus_t __stm32_spi_bus[__STM32_SPI_MAX_CHANNEL] = {
#ifdef SPI1_BASE
        {
                .nnode = {
                        .name = "spi1",
                },
                .ops   = &__stm32_spi_bus_ops,
                .ctx   = &__stm32_spi_bus_privinfo[0],
        },
#ifdef SPI2_BASE
        {
                .nnode = {
                        .name = "spi2",
                },
                .ops   = &__stm32_spi_bus_ops,
                .ctx   = &__stm32_spi_bus_privinfo[1],
        },
#ifdef SPI3_BASE
        {
                .nnode = {
                        .name = "spi3",
                },
                .ops   = &__stm32_spi_bus_ops,
                .ctx   = &__stm32_spi_bus_privinfo[2],
        },
#ifdef SPI4_BASE
        {
                .nnode = {
                        .name = "spi4",
                },
                .ops   = &__stm32_spi_bus_ops,
                .ctx   = &__stm32_spi_bus_privinfo[3],
        },
#ifdef SPI5_BASE
        {
                .nnode = {
                        .name = "spi5",
                },
                .ops   = &__stm32_spi_bus_ops,
                .ctx   = &__stm32_spi_bus_privinfo[4],
        },
#ifdef SPI6_BASE
        {
                .nnode = {
                        .name = "spi6",
                },
                .ops   = &__stm32_spi_bus_ops,
                .ctx   = &__stm32_spi_bus_privinfo[5],
        },
#endif
#endif
#endif
#endif
#endif
#endif
};

/*
 * Tx Transfer completed callback.
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    privinfo_t *priv = MS_CONTAINER_OF(hspi, privinfo_t, spi_handler);
    (void)ms_semb_post(priv->sembid);
}

/*
 * Rx Transfer completed callback.
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    privinfo_t *priv = MS_CONTAINER_OF(hspi, privinfo_t, spi_handler);
    (void)ms_semb_post(priv->sembid);
}

/*
 * Tx & Rx Transfer completed callback.
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    privinfo_t *priv = MS_CONTAINER_OF(hspi, privinfo_t, spi_handler);
    (void)ms_semb_post(priv->sembid);
}

/*
 * Handle SPI interrupt request.
 */
void stm32_spi_irq_handler(ms_uint8_t channel)
{
    HAL_SPI_IRQHandler(&__stm32_spi_bus_privinfo[channel - 1U].spi_handler);
}

/*
 * Create SPI device file
 */
ms_err_t stm32_spi_bus_dev_create(const char *path, ms_uint8_t channel, ms_uint8_t trans_mode)
{
    ms_spi_bus_t *spi_bus;
    privinfo_t *priv;
    ms_err_t err;

    if ((path == MS_NULL) || (channel > __STM32_SPI_MAX_CHANNEL) || (channel == 0U)) {
        return MS_ERR;
    }

    spi_bus = &__stm32_spi_bus[channel - 1];

    priv = (privinfo_t *)spi_bus->ctx;

    err = ms_semb_create("spi_semb", MS_FALSE, MS_WAIT_TYPE_PRIO, &priv->sembid);
    if (err == MS_ERR_NONE) {
        /*
         * Initial SPI bus device
         */
        priv->param.baud_rate  = 25000000;
        priv->param.mode       = MS_SPI_MODE_MASTER;
        priv->param.direction  = MS_SPI_DIRECTION_2LINES;
        priv->param.data_size  = MS_SPI_DATA_SIZE_8BIT;
        priv->param.frame_mode = MS_SPI_TI_MODE_DISABLE;
        priv->param.nss        = MS_SPI_NSS_SOFT;
        priv->trans_mode       = trans_mode;

        /*
         * Convert ms_spi_param_t to native structure SPI_InitTypeDef
         */
        __stm32_spi_bus_param_convert(&priv->param, &(priv->spi_handler.Init));

        /*
         * set spi prescaler
         */
        __stm32_spi_prescaler_set(priv->spi_handler.Instance, priv->param.baud_rate,
                                    (ms_uint32_t *)&(priv->spi_handler.Init.BaudRatePrescaler));
        /*
         * Hardware initialization
         */
        if (HAL_SPI_Init(&priv->spi_handler) == HAL_OK) {
            if (priv->trans_mode == SPI_IT_MODE ||
                priv->trans_mode == SPI_DMA_MODE) {
                HAL_NVIC_SetPriority(priv->irq, 1, 3);
                __stm32_spi_bus_int_set(priv, MS_TRUE);
            } else {
                __stm32_spi_bus_int_set(priv, MS_FALSE);
            }

            err = ms_spi_bus_register(spi_bus);
            if (err == MS_ERR_NONE) {
                err = ms_spi_bus_dev_create(path, spi_bus);
            }
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
