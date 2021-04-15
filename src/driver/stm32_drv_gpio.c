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
** 文   件   名: stm32_drv_gpio.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 GPIO 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"

#if BSP_CFG_GPIO_EN > 0

#define GPIO_NUMBER                             16

#define IS_GPIO_ALL_PIN(PIN)                    (((PIN) == GPIO_PIN_0)  || \
                                                 ((PIN) == GPIO_PIN_1)  || \
                                                 ((PIN) == GPIO_PIN_2)  || \
                                                 ((PIN) == GPIO_PIN_3)  || \
                                                 ((PIN) == GPIO_PIN_4)  || \
                                                 ((PIN) == GPIO_PIN_5)  || \
                                                 ((PIN) == GPIO_PIN_6)  || \
                                                 ((PIN) == GPIO_PIN_7)  || \
                                                 ((PIN) == GPIO_PIN_8)  || \
                                                 ((PIN) == GPIO_PIN_9)  || \
                                                 ((PIN) == GPIO_PIN_10) || \
                                                 ((PIN) == GPIO_PIN_11) || \
                                                 ((PIN) == GPIO_PIN_12) || \
                                                 ((PIN) == GPIO_PIN_13) || \
                                                 ((PIN) == GPIO_PIN_14) || \
                                                 ((PIN) == GPIO_PIN_15))

#define IS_GPIO_EXTI_MODE(MODE)                 (((MODE) == GPIO_MODE_IT_RISING)  || \
                                                 ((MODE) == GPIO_MODE_IT_FALLING) || \
                                                 ((MODE) == GPIO_MODE_IT_RISING_FALLING))

/*
 * Private info
 */
typedef struct {
    ms_pollfd_t *slots[1];

    ms_addr_t   base;
    ms_uint16_t pin;

    ms_uint8_t  mode;
    ms_uint8_t  pull;
    ms_uint8_t  speed;

    void      (*isr)(ms_ptr_t arg);
    ms_ptr_t    arg;
} privinfo_t;

/*
 * gpio device
 */
typedef struct {
    privinfo_t      priv;
    ms_io_device_t  dev;
} stm32_gpio_dev_t;

static privinfo_t *gpio_exti_line_priv[GPIO_NUMBER];

static IRQn_Type   gpio_exti_line_irqn[GPIO_NUMBER] = {
    EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn,
    EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
    EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
};

/*
 * Open device
 */
static int __stm32_gpio_open(ms_ptr_t ctx, ms_io_file_t *file, int oflag, ms_mode_t mode)
{
    int ret;

    if (ms_atomic_inc(MS_IO_DEV_REF(file)) == 1) {
        ret = 0;

    } else {
        ms_atomic_dec(MS_IO_DEV_REF(file));
        ms_thread_set_errno(EBUSY);
        ret = -1;
    }

    return ret;
}

/*
 * Close device
 */
static int __stm32_gpio_close(ms_ptr_t ctx, ms_io_file_t *file)
{
    privinfo_t *priv = ctx;
    ms_uint16_t pin = priv->pin;
    GPIO_TypeDef *gpio_x = (GPIO_TypeDef *)priv->base;

    if (ms_atomic_dec(MS_IO_DEV_REF(file)) == 0) {
        HAL_GPIO_DeInit(gpio_x, pin);
    }

    return 0;
}

/*
 * Read device
 */
static ms_ssize_t __stm32_gpio_read(ms_ptr_t ctx, ms_io_file_t *file, ms_ptr_t buf, ms_size_t len)
{
    privinfo_t *priv = ctx;
    ms_uint16_t pin = priv->pin;
    GPIO_TypeDef *gpio_x = (GPIO_TypeDef *)priv->base;
    GPIO_PinState state;
    ms_uint8_t *value = (ms_uint8_t *)buf;

    state = HAL_GPIO_ReadPin(gpio_x, pin);
    *value = (state == GPIO_PIN_RESET) ? 0 : 1;

    return sizeof(ms_uint8_t);
}

/*
 * Write device
 */
static ms_ssize_t __stm32_gpio_write(ms_ptr_t ctx, ms_io_file_t *file, ms_const_ptr_t buf, ms_size_t len)
{
    privinfo_t *priv = ctx;
    ms_uint16_t pin = priv->pin;
    GPIO_TypeDef *gpio_x = (GPIO_TypeDef *)priv->base;
    GPIO_PinState state;

    state = (*(ms_uint8_t *)buf) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(gpio_x, pin, state);

    return sizeof(ms_uint8_t);
}

/*
 * GPIO pin to pos [0...15]
 */
static ms_uint8_t __stm32_gpio_pin_to_pos(ms_uint16_t pin)
{
    ms_uint8_t pos;

    for (pos = 0; pos < GPIO_NUMBER; pos++) {
        if (pin == (((ms_uint16_t)0x01) << pos)) {
            break;
        }
    }

    return pos;
}

/*
 * Covert general flags to stm32 flags
 */
static ms_err_t __stm32_gpio_param_to_hal_param(const ms_gpio_param_t *param, GPIO_InitTypeDef *init)
{
    /*
     * Covert mode
     */
    switch (param->mode) {
    case MS_GPIO_MODE_INPUT:
        init->Mode = GPIO_MODE_INPUT;
        break;

    case MS_GPIO_MODE_OUTPUT_PP:
        init->Mode = GPIO_MODE_OUTPUT_PP;
        break;

    case MS_GPIO_MODE_OUTPUT_OD:
        init->Mode = GPIO_MODE_OUTPUT_OD;
        break;

    case MS_GPIO_MODE_IRQ_RISING:
        init->Mode = GPIO_MODE_IT_RISING;
        break;

    case MS_GPIO_MODE_IRQ_FALLING:
        init->Mode = GPIO_MODE_IT_FALLING;
        break;

    case MS_GPIO_MODE_IRQ_BOTH:
        init->Mode = GPIO_MODE_IT_RISING_FALLING;
        break;

    default:
        return MS_ERR;
    }

    /*
     * Covert pull
     */
    switch (param->pull) {
    case MS_GPIO_PULL_NONE:
        init->Pull = GPIO_NOPULL;
        break;

    case MS_GPIO_PULL_UP:
        init->Pull = GPIO_PULLUP;
        break;

    case MS_GPIO_PULL_DOWN:
        init->Pull = GPIO_PULLDOWN;
        break;

    default:
        return MS_ERR;
    }

    /*
     * Covert speed
     */
    switch (param->speed) {
    case MS_GPIO_SPEED_LOW:
        init->Speed = GPIO_SPEED_FREQ_LOW;
        break;

    case MS_GPIO_SPEED_MEDIUM:
        init->Speed = GPIO_SPEED_FREQ_MEDIUM;
        break;

    case MS_GPIO_SPEED_HIGH:
        init->Speed = GPIO_SPEED_FREQ_HIGH;
        break;

    case MS_GPIO_SPEED_VERY_HIGH:
        init->Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        break;

    default:
        return MS_ERR;
    }

    return MS_ERR_NONE;
}

/*
 * Control device
 */
static int __stm32_gpio_ioctl(ms_ptr_t ctx, ms_io_file_t *file, int cmd, void *arg)
{
    privinfo_t *priv = ctx;
    ms_uint16_t pin = priv->pin;
    GPIO_TypeDef *gpio_x = (GPIO_TypeDef *)priv->base;
    GPIO_InitTypeDef init;
    ms_gpio_param_t *param;
    int ret;

    switch (cmd) {

    case MS_GPIO_CMD_SET_PARAM:
        if (ms_access_ok(arg, sizeof(ms_gpio_param_t), MS_ACCESS_R)) {
            param = (ms_gpio_param_t *)arg;

            bzero(&init, sizeof(GPIO_InitTypeDef));

            init.Pin = pin;
            if (__stm32_gpio_param_to_hal_param(param, &init) == MS_ERR_NONE) {
                HAL_GPIO_Init(gpio_x, &init);

                if (IS_GPIO_EXTI_MODE(init.Mode)) {
                    ms_uint8_t pos = __stm32_gpio_pin_to_pos(pin);

                    gpio_exti_line_priv[pos] = priv;

                    HAL_NVIC_SetPriority(gpio_exti_line_irqn[pos], 0x0F, 0x00);
                    HAL_NVIC_EnableIRQ(gpio_exti_line_irqn[pos]);
                }

                /*
                 * Save param in priv
                 */
                priv->mode  = param->mode;
                priv->pull  = param->pull;
                priv->speed = param->speed;

                ret = 0;
            } else {
                ms_thread_set_errno(EINVAL);
                ret = -1;
            }
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_GPIO_CMD_GET_PARAM:
        if (ms_access_ok(arg, sizeof(ms_gpio_param_t), MS_ACCESS_W)) {
            param = (ms_gpio_param_t *)arg;
            param->mode  = priv->mode;
            param->pull  = priv->pull;
            param->speed = priv->speed;
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
 * Device notify
 */
static int __stm32_gpio_poll_notify(privinfo_t *priv, ms_pollevent_t event)
{
    return ms_io_poll_notify_helper(priv->slots, MS_ARRAY_SIZE(priv->slots), event);
}

/*
 * This function called by all external interrupt handles
 */
void HAL_GPIO_EXTI_Callback(ms_uint16_t GPIO_Pin)
{
    privinfo_t *priv = gpio_exti_line_priv[__stm32_gpio_pin_to_pos(GPIO_Pin)];

    if (priv->isr != MS_NULL){
        priv->isr(priv->arg);
    }

    __stm32_gpio_poll_notify(priv, POLLIN);
}

/*
 * This function handles EXTI0 interrupt
 */
void EXTI0_IRQHandler(void)
{
    (void)ms_int_enter();

    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);

    (void)ms_int_exit();
}
/*
 * This function handles EXTI1 interrupt
 */
void EXTI1_IRQHandler(void)
{
    (void)ms_int_enter();

    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);

    (void)ms_int_exit();
}
/*
 * This function handles EXTI2 interrupt
 */
void EXTI2_IRQHandler(void)
{
    (void)ms_int_enter();

    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);

    (void)ms_int_exit();
}
/*
 * This function handles EXTI3 interrupt
 */
void EXTI3_IRQHandler(void)
{
    (void)ms_int_enter();

    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);

    (void)ms_int_exit();
}
/*
 * This function handles EXTI4 interrupt
 */
void EXTI4_IRQHandler(void)
{
    (void)ms_int_enter();

    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);

    (void)ms_int_exit();
}

/*
 * This function handles EXTI9-EXTI5 interrupts
 */
void EXTI9_5_IRQHandler(void)
{
    (void)ms_int_enter();

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    }

    (void)ms_int_exit();
}

/*
 * This function handles EXTI15-EXTI10 interrupts
 */
void EXTI15_10_IRQHandler(void)
{
    (void)ms_int_enter();

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    }

    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10) != RESET) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    }

    (void)ms_int_exit();
}

/*
 * Check device readable 
 */
static ms_bool_t __stm32_gpio_readable_check(ms_ptr_t ctx)
{
    privinfo_t *priv = ctx;
    ms_bool_t ret;

    /*
     * Check EXTI line pending bit
     */
    if (__HAL_GPIO_EXTI_GET_IT(priv->pin) != RESET) {
        ret = MS_TRUE;
    } else {
        ret = MS_FALSE;
    }

    return ret;
}

/*
 * Poll device
 */
static int __stm32_gpio_poll(ms_ptr_t ctx, ms_io_file_t *file, ms_pollfd_t *fds, ms_bool_t setup)
{
    privinfo_t *priv = ctx;

    return ms_io_poll_helper(fds, priv->slots, MS_ARRAY_SIZE(priv->slots), setup, ctx,
                             __stm32_gpio_readable_check, MS_NULL, MS_NULL);;
}

/*
 * Power management
 */
#if MS_CFG_KERN_PM_EN > 0
static ms_err_t __stm32_gpio_suspend(ms_ptr_t ctx, ms_pm_sleep_mode_t sleep_mode)
{
    //TODO:

    return MS_ERR_NONE;
}

static void __stm32_gpio_resume(ms_ptr_t ctx, ms_pm_sleep_mode_t sleep_mode)
{
    //TODO:
}
#endif

/*
 * Device operating function set
 */
static const ms_io_driver_ops_t stm32_gpio_drv_ops = {
        .type    = MS_IO_DRV_TYPE_CHR,
        .open    = __stm32_gpio_open,
        .close   = __stm32_gpio_close,
        .read    = __stm32_gpio_read,
        .write   = __stm32_gpio_write,
        .ioctl   = __stm32_gpio_ioctl,
        .poll    = __stm32_gpio_poll,
#if MS_CFG_KERN_PM_EN > 0
        .suspend = __stm32_gpio_suspend,
        .resume  = __stm32_gpio_resume,
#endif
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_gpio_drv = {
        .nnode = {
            .name = "stm32_gpio",
        },
        .ops = &stm32_gpio_drv_ops,
};

/*
 * Register gpio device driver
 */
ms_err_t stm32_gpio_drv_register(void)
{
    return ms_io_driver_register(&stm32_gpio_drv);
}

/*
 * Create gpio device file
 */
ms_err_t stm32_gpio_dev_create(const char *path, ms_addr_t base, ms_uint16_t pin)
{
    stm32_gpio_dev_t *dev;
    ms_err_t err;

    if (IS_GPIO_ALL_INSTANCE((void *)base) && IS_GPIO_ALL_PIN(pin)) {
        dev = ms_kmalloc(sizeof(stm32_gpio_dev_t));
        if (dev != MS_NULL) {
            /*
             * Make sure clear priv.slots
             */
            bzero(&dev->priv, sizeof(dev->priv));

            dev->priv.base = base;
            dev->priv.pin  = pin;

            err = ms_io_device_register(&dev->dev, path, "stm32_gpio", &dev->priv);

        } else {
            err = MS_ERR;
        }

    } else {
        err = MS_ERR;
    }

    return err;
}

/*
 * Install GPIO ISR
 */
void stm32_gpio_isr_install(ms_uint16_t pin, void (*isr)(ms_ptr_t), ms_ptr_t arg)
{
    ms_uint8_t  pos  = __stm32_gpio_pin_to_pos(pin);
    privinfo_t *priv = gpio_exti_line_priv[pos];

    priv->arg = arg;
    priv->isr = isr;
}

/*
 * Enable or disable GPIO interrupt
 */
void stm32_gpio_int_enable(ms_uint16_t pin, ms_bool_t enable)
{
    ms_uint8_t   pos = __stm32_gpio_pin_to_pos(pin);
    ms_arch_sr_t sr  = ms_arch_int_disable();

    if (enable) {
        HAL_NVIC_EnableIRQ(gpio_exti_line_irqn[pos]);

    } else {
        HAL_NVIC_DisableIRQ(gpio_exti_line_irqn[pos]);
    }

    ms_arch_int_resume(sr);
}

#endif                                                                  /*  BSP_CFG_GPIO_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
