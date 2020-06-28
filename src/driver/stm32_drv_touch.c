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
** 文   件   名: stm32_drv_touch.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片触摸屏驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"

#if BSP_CFG_TOUCH_EN > 0

/*
 * Private info
 */
typedef struct {
    ms_pollfd_t *slots[1];
} privinfo_t;

/*
 * Open device
 */
static int __stm32_touch_open(ms_ptr_t ctx, ms_io_file_t *file, int oflag, ms_mode_t mode)
{
    int ret;

    if (ms_atomic_inc(MS_IO_DEV_REF(file)) == 2) {
        if (BSP_TS_Init(BSP_CFG_LCD_WIDTH, BSP_CFG_LCD_HEIGHT) == TS_OK) {
            ret = 0;
        } else {
            ms_atomic_dec(MS_IO_DEV_REF(file));
            ms_thread_set_errno(EIO);
            ret = -1;
        }

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
static int __stm32_touch_close(ms_ptr_t ctx, ms_io_file_t *file)
{
    if (ms_atomic_dec(MS_IO_DEV_REF(file)) == 0) {
        (void)BSP_TS_DeInit();
    }

    return 0;
}

/*
 * Control device
 */
static int __stm32_touch_ioctl(ms_ptr_t ctx, ms_io_file_t *file, int cmd, void *arg)
{
    ms_thread_set_errno(EINVAL);

    return -1;
}

/*
 * Read device
 */
static ssize_t __stm32_touch_read(ms_ptr_t ctx, ms_io_file_t *file, ms_ptr_t buf, size_t len)
{
    ms_touch_event_t *event = (ms_touch_event_t *)buf;
    TS_StateTypeDef state;
    ssize_t ret;

    if (BSP_TS_GetState(&state) == TS_OK) {
        event->touch_detected = state.touchDetected;
        event->touch_x[0] = state.touchX[0];
        event->touch_y[0] = state.touchY[0];
        ret = sizeof(ms_touch_event_t);

    } else {
        ms_thread_set_errno(EIO);
        ret = -1;
    }

    return ret;
}

/*
 * Check device readable
 */
static ms_bool_t __stm32_touch_readable_check(ms_ptr_t ctx)
{
    privinfo_t *priv = ctx;

    (void)priv;

    return MS_FALSE;
}

/*
 * Device notify
 */
#if 0
static int __stm32_touch_poll_notify(privinfo_t *priv, ms_pollevent_t event)
{
    return ms_io_poll_notify_heaper(priv->slots, MS_ARRAY_SIZE(priv->slots), event);
}
#endif

/*
 * Poll device
 */
static int __stm32_touch_poll(ms_ptr_t ctx, ms_io_file_t *file, ms_pollfd_t *fds, ms_bool_t setup)
{
    privinfo_t *priv = ctx;

    return ms_io_poll_heaper(fds, priv->slots, MS_ARRAY_SIZE(priv->slots), setup, ctx,
                             __stm32_touch_readable_check, MS_NULL, MS_NULL);;
}

/*
 * Device operating function set
 */
static const ms_io_driver_ops_t stm32_touch_drv_ops = {
        .type   = MS_IO_DRV_TYPE_CHR,
        .open   = __stm32_touch_open,
        .read   = __stm32_touch_read,
        .ioctl  = __stm32_touch_ioctl,
        .close  = __stm32_touch_close,
        .poll   = __stm32_touch_poll,
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_touch_drv = {
        .nnode = {
            .name = "stm32_touch",
        },
        .ops = &stm32_touch_drv_ops,
};

/*
 * Register touch screen device driver
 */
ms_err_t stm32_touch_drv_register(void)
{
    return ms_io_driver_register(&stm32_touch_drv);
}

/*
 * Register touch screen device file
 */
ms_err_t stm32_touch_dev_register(const char *path)
{
    static privinfo_t priv;
    static ms_io_device_t dev;

    return ms_io_device_register(&dev, path, "stm32_touch", &priv);
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
