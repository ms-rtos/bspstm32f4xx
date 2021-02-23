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
** 文   件   名: stm32_drv_fb.c
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: STM32 芯片 FrameBuffer 驱动
*********************************************************************************************************/
#define __MS_IO
#include "ms_config.h"
#include "ms_rtos.h"
#include "ms_io_core.h"

#include <string.h>

#include "includes.h"

#if BSP_CFG_FB_EN > 0

/*
 * Private info
 */
typedef struct {
    ms_fb_var_screeninfo_t  var;
    ms_fb_fix_screeninfo_t  fix;
} privinfo_t;

static uint16_t stm32_fb_lcd_int_active_line;
static uint16_t stm32_fb_lcd_int_porch_line;
static ms_pid_t stm32_fb_pid;

#if BSP_CFG_LCD_BPP == 16U
#define LCD_BYTE_PER_PIXEL  2U
#else
#define LCD_BYTE_PER_PIXEL  4U
#endif

#if defined(DMA2D)
DMA2D_HandleTypeDef         hdma2d;

static HAL_StatusTypeDef __stm32_dma2d_set_mode(DMA2D_HandleTypeDef *hdma2d, ms_uint32_t mode, ms_uint32_t color, ms_uint32_t offset)
{
    assert_param(IS_DMA2D_ALL_INSTANCE(hdma2d->Instance));

    MODIFY_REG(hdma2d->Instance->CR, DMA2D_CR_MODE, mode);
    MODIFY_REG(hdma2d->Instance->OPFCCR, DMA2D_OPFCCR_CM, color);
    MODIFY_REG(hdma2d->Instance->OOR, DMA2D_OOR_LO, offset);

    return HAL_OK;
}

static void __stm32_dma2d_xfer_done_cb(DMA2D_HandleTypeDef *hdma2d)
{
    ms_process_sigqueue(stm32_fb_pid, MS_FB_SIGNAL_DMA2D_DONE, MS_TIMEOUT_NO_WAIT);
}

static void __stm32_dma2d_xfer_error_cb(DMA2D_HandleTypeDef *hdma2d)
{
}

static void __stm32_dma2d_data_copy(DMA2D_HandleTypeDef *hdma2d, ms_fb_blitop_arg_t *blitop)
{
    ms_uint32_t dma2dTransferMode = DMA2D_M2M_BLEND;
    ms_uint32_t dma2dColorMode = 0;

    ms_bool_t blendingImage = (blitop->operation == MS_FB_BLIT_OP_COPY_ARGB8888 ||
                               blitop->operation == MS_FB_BLIT_OP_COPY_ARGB8888_WITH_ALPHA ||
                               blitop->operation == MS_FB_BLIT_OP_COPY_WITH_ALPHA);

    ms_bool_t blendingText = (blitop->operation == MS_FB_BLIT_OP_COPY_A4 ||
                              blitop->operation == MS_FB_BLIT_OP_COPY_A8);

    ms_uint8_t bitDepth = BSP_CFG_LCD_BPP;

    switch (blitop->operation) {
    case MS_FB_BLIT_OP_COPY_A4:
        dma2dColorMode = CM_A4;
        break;

    case MS_FB_BLIT_OP_COPY_A8:
        dma2dColorMode = CM_A8;
        break;

    case MS_FB_BLIT_OP_COPY_WITH_ALPHA:
        dma2dTransferMode = DMA2D_M2M_BLEND;
        dma2dColorMode = (bitDepth == 16) ? CM_RGB565 : CM_RGB888;
        break;

    case MS_FB_BLIT_OP_COPY_ARGB8888:
    case MS_FB_BLIT_OP_COPY_ARGB8888_WITH_ALPHA:
        dma2dColorMode = CM_ARGB8888;
        break;

    default:
        dma2dTransferMode = DMA2D_M2M;
        dma2dColorMode = (bitDepth == 16) ? CM_RGB565 : CM_RGB888;
        break;
    }

    /* HAL_DMA2D_ConfigLayer() depends on hdma2d->Init */
    hdma2d->Init.Mode = dma2dTransferMode;
    hdma2d->Init.ColorMode = (bitDepth == 16) ? DMA2D_RGB565 : DMA2D_RGB888;
    hdma2d->Init.OutputOffset = blitop->dst_loop_stride - blitop->steps;

    __stm32_dma2d_set_mode(hdma2d, dma2dTransferMode,
                           (bitDepth == 16) ? DMA2D_RGB565 : DMA2D_RGB888,
                           blitop->dst_loop_stride - blitop->steps);

    hdma2d->LayerCfg[1].InputColorMode = dma2dColorMode;
    hdma2d->LayerCfg[1].InputOffset = blitop->src_loop_stride - blitop->steps;

    if (blendingImage || blendingText) {
        if (blitop->alpha < 255) {
            hdma2d->LayerCfg[1].AlphaMode = DMA2D_COMBINE_ALPHA;
            hdma2d->LayerCfg[1].InputAlpha = blitop->alpha;
        } else {
            hdma2d->LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
        }

        if (blendingText) {
            if (bitDepth == 16) {
                ms_uint32_t red = (((blitop->color & 0xF800) >> 11) * 255) / 31;
                ms_uint32_t green = (((blitop->color & 0x7E0) >> 5) * 255) / 63;
                ms_uint32_t blue = (((blitop->color & 0x1F)) * 255) / 31;
                ms_uint32_t alpha = blitop->alpha;

                hdma2d->LayerCfg[1].InputAlpha = (alpha << 24) | (red << 16) | (green << 8) | blue;

            } else {
                hdma2d->LayerCfg[1].InputAlpha = (blitop->color & 0xFFFFFF) | (blitop->alpha << 24);
            }
        }

        hdma2d->LayerCfg[0].InputOffset = blitop->dst_loop_stride - blitop->steps;
        hdma2d->LayerCfg[0].InputColorMode = (bitDepth == 16) ? CM_RGB565 : CM_RGB888;

        HAL_DMA2D_ConfigLayer(hdma2d, 0);
    }

    HAL_DMA2D_ConfigLayer(hdma2d, 1);

    // If the framebuffer is placed in Write Through cached memory (e.g. SRAM) then we need
    // to flush the Dcache prior to letting DMA2D accessing it.
    ms_arch_sr_t sr = ms_arch_int_disable();
    SCB_CleanInvalidateDCache();
    ms_arch_int_resume(sr);

    if (blendingImage || blendingText) {
        HAL_DMA2D_BlendingStart_IT(hdma2d, (ms_addr_t)blitop->src,
                                   (ms_addr_t)blitop->dst, (ms_addr_t)blitop->dst,
                                   blitop->steps, blitop->loops);
    } else {
        HAL_DMA2D_Start_IT(hdma2d, (ms_addr_t)blitop->src,
                           (ms_addr_t)blitop->dst, blitop->steps, blitop->loops);
    }
}

static void __stm32_dma2d_data_fill(DMA2D_HandleTypeDef *hdma2d, ms_fb_blitop_arg_t *blitop)
{
    ms_uint8_t  bitDepth = BSP_CFG_LCD_BPP;
    ms_uint32_t dma2dTransferMode;
    ms_uint32_t dma2dColorMode = (bitDepth == 16) ? CM_RGB565 : CM_RGB888;
    ms_uint32_t color = 0;

    if (bitDepth == 16) {
        ms_uint32_t red = (((blitop->color & 0xF800) >> 11) * 255) / 31;
        ms_uint32_t green = (((blitop->color & 0x7E0) >> 5) * 255) / 63;
        ms_uint32_t blue = (((blitop->color & 0x1F)) * 255) / 31;
        ms_uint32_t alpha = blitop->alpha;

        color = (alpha << 24) | (red << 16) | (green << 8) | blue;

    } else {
        color = (blitop->alpha << 24) | (blitop->color & 0xFFFFFF);
    }

    switch (blitop->operation) {
    case MS_FB_BLIT_OP_FILL_WITH_ALPHA:
        dma2dTransferMode = DMA2D_M2M_BLEND;
        break;

    default:
        dma2dTransferMode = DMA2D_R2M;
        break;
    }

    /* HAL_DMA2D_ConfigLayer() depends on hdma2d->Init */
    hdma2d->Init.Mode = dma2dTransferMode;
    hdma2d->Init.ColorMode = (bitDepth == 16) ? DMA2D_RGB565 : DMA2D_RGB888;
    hdma2d->Init.OutputOffset = blitop->dst_loop_stride - blitop->steps;

    __stm32_dma2d_set_mode(hdma2d, dma2dTransferMode,
                           (bitDepth == 16) ? DMA2D_RGB565 : DMA2D_RGB888,
                            blitop->dst_loop_stride - blitop->steps);

    if (dma2dTransferMode == DMA2D_M2M_BLEND) {
        hdma2d->LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
        hdma2d->LayerCfg[1].InputAlpha = color;
        hdma2d->LayerCfg[1].InputColorMode = CM_A8;
        hdma2d->LayerCfg[0].InputOffset = blitop->dst_loop_stride - blitop->steps;
        hdma2d->LayerCfg[0].InputColorMode = (bitDepth == 16) ? CM_RGB565 : CM_RGB888;
        HAL_DMA2D_ConfigLayer(hdma2d, 0);

    } else {
        hdma2d->LayerCfg[1].InputColorMode = dma2dColorMode;
        hdma2d->LayerCfg[1].InputOffset = 0;
    }

    HAL_DMA2D_ConfigLayer(hdma2d, 1);

    // If the framebuffer is placed in Write Through cached memory (e.g. SRAM) then we need
    // to flush the Dcache prior to letting DMA2D accessing it.
    ms_arch_sr_t sr = ms_arch_int_disable();
    SCB_CleanInvalidateDCache();
    ms_arch_int_resume(sr);

    if (dma2dTransferMode == DMA2D_M2M_BLEND) {
        HAL_DMA2D_BlendingStart_IT(hdma2d, (ms_addr_t)blitop->dst,
                                   (ms_addr_t)blitop->dst, (ms_addr_t)blitop->dst,
                                   blitop->steps, blitop->loops);
    } else {
        HAL_DMA2D_Start_IT(hdma2d, color, (ms_addr_t)blitop->dst,
                           blitop->steps, blitop->loops);
    }
}
#endif

void HAL_LTDC_LineEventCallback(LTDC_HandleTypeDef *hltdc)
{
    if (LTDC->LIPCR == stm32_fb_lcd_int_active_line) {
        //entering active area
        HAL_LTDC_ProgramLineEvent(hltdc, stm32_fb_lcd_int_porch_line);

        ms_process_sigqueue(stm32_fb_pid, MS_FB_SIGNAL_ACTIVE_AREA_ENTER, MS_TIMEOUT_NO_WAIT);

    } else {
        //exiting active area
        HAL_LTDC_ProgramLineEvent(hltdc, stm32_fb_lcd_int_active_line);

        ms_process_sigqueue(stm32_fb_pid, MS_FB_SIGNAL_ACTIVE_AREA_EXIT, MS_TIMEOUT_NO_WAIT);
    }
}

/*
 * Open device
 */
static int __stm32_fb_open(ms_ptr_t ctx, ms_io_file_t *file, int oflag, ms_mode_t mode)
{
    privinfo_t *priv = ctx;
    int ret;

    if (ms_atomic_inc(MS_IO_DEV_REF(file)) == 1) {
        if (BSP_LCD_Init() == LCD_OK) {
#if BSP_CFG_LCD_BPP == 16U
            BSP_LCD_LayerRgb565Init(0, (ms_uint32_t)priv->fix.smem_start);
#else
            BSP_LCD_LayerDefaultInit(0, (ms_uint32_t)priv->fix.smem_start);
#endif
            BSP_LCD_SelectLayer(0);
            BSP_LCD_Clear(LCD_COLOR_WHITE);
            BSP_LCD_SetLayerVisible(0, ENABLE);
            BSP_LCD_DisplayOn();

#if defined(DMA2D)
            hdma2d.Instance = DMA2D;

            hdma2d.Init.Mode = DMA2D_M2M;
#if BSP_CFG_LCD_BPP == 16U
            hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
            hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
#else
            hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
            hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
#endif
            hdma2d.Init.OutputOffset = 0;
            hdma2d.LayerCfg[1].InputOffset = 0;
            hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
            hdma2d.LayerCfg[1].InputAlpha = 0;

            HAL_DMA2D_Init(&hdma2d);

            HAL_DMA2D_ConfigLayer(&hdma2d, 1);

            hdma2d.XferCpltCallback  = __stm32_dma2d_xfer_done_cb;
            hdma2d.XferErrorCallback = __stm32_dma2d_xfer_error_cb;
#endif

            stm32_fb_pid = ms_process_self();
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
static int __stm32_fb_close(ms_ptr_t ctx, ms_io_file_t *file)
{
    if (ms_atomic_dec(MS_IO_DEV_REF(file)) == 0) {
#if defined(DMA2D)
        HAL_DMA2D_DeInit(&hdma2d);
        NVIC_DisableIRQ(DMA2D_IRQn);
#endif
        NVIC_DisableIRQ(LTDC_IRQn);

        BSP_LCD_DisplayOff();
        (void)BSP_LCD_DeInit();
    }

    return 0;
}

/*
 * Control device
 */
static int __stm32_fb_ioctl(ms_ptr_t ctx, ms_io_file_t *file, int cmd, void *arg)
{
    privinfo_t *priv = ctx;
    int ret;

    switch (cmd) {
    case MS_FB_CMD_GET_VSCREENINFO:
        if (ms_access_ok(arg, sizeof(ms_fb_var_screeninfo_t), MS_ACCESS_W)) {
            memcpy(arg, &priv->var, sizeof(priv->var));
            ret = 0;
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_FB_CMD_GET_FSCREENINFO:
        if (ms_access_ok(arg, sizeof(ms_fb_fix_screeninfo_t), MS_ACCESS_W)) {
            memcpy(arg, &priv->fix, sizeof(priv->fix));
            ret = 0;
        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_FB_CMD_GET_FB:
        ret = 0;
        break;

    case MS_FB_CMD_SET_FB:
        ret = 0;
        break;

    case MS_FB_CMD_CFG_INTS:
        NVIC_SetPriority(DMA2D_IRQn, 9);
        NVIC_SetPriority(LTDC_IRQn, 9);
        ret = 0;
        break;

    case MS_FB_CMD_ENABLE_INTS:
        NVIC_EnableIRQ(DMA2D_IRQn);
        NVIC_EnableIRQ(LTDC_IRQn);
        ret = 0;
        break;

    case MS_FB_CMD_DISABLE_INTS:
        NVIC_DisableIRQ(DMA2D_IRQn);
        NVIC_DisableIRQ(LTDC_IRQn);
        ret = 0;
        break;

    case MS_FB_CMD_ENABLE_LCDC_INT:
        stm32_fb_lcd_int_active_line = (LTDC->BPCR & 0x7FF) - 1;
        stm32_fb_lcd_int_porch_line  = (LTDC->AWCR & 0x7FF) - 1;

        /* Sets the Line Interrupt position */
        LTDC->LIPCR = stm32_fb_lcd_int_active_line;
        /* Line Interrupt Enable            */
        LTDC->IER |= LTDC_IER_LIE;
        ret = 0;
        break;

#if defined(DMA2D)
    case MS_FB_CMD_DATA_COPY_OP:
        if (ms_access_ok(arg, sizeof(ms_fb_data_copy_arg_t), MS_ACCESS_R)) {
            ms_fb_data_copy_arg_t *copy_arg = arg;

            __stm32_dma2d_data_copy(&hdma2d, copy_arg);
            ret = 0;

        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;

    case MS_FB_CMD_DATA_FILL_OP:
        if (ms_access_ok(arg, sizeof(ms_fb_data_fill_arg_t), MS_ACCESS_R)) {
            ms_fb_data_fill_arg_t *fill_arg = arg;

            __stm32_dma2d_data_fill(&hdma2d, fill_arg);
            ret = 0;

        } else {
            ms_thread_set_errno(EFAULT);
            ret = -1;
        }
        break;
#endif

    default:
        ms_thread_set_errno(EINVAL);
        ret = -1;
        break;
    }

    return ret;
}

/*
 * Device operating function set
 */
static const ms_io_driver_ops_t stm32_fb_drv_ops = {
        .type   = MS_IO_DRV_TYPE_CHR,
        .open   = __stm32_fb_open,
        .ioctl  = __stm32_fb_ioctl,
        .close  = __stm32_fb_close,
};

/*
 * Device driver
 */
static ms_io_driver_t stm32_fb_drv = {
        .nnode = {
            .name = "stm32_fb",
        },
        .ops  = &stm32_fb_drv_ops,
};

/*
 * Register frame buffer device driver
 */
ms_err_t stm32_fb_drv_register(void)
{
    return ms_io_driver_register(&stm32_fb_drv);
}

/*
 * Register frame buffer device file
 */
ms_err_t stm32_fb_dev_register(const char *path)
{
    static privinfo_t priv;
    static ms_io_device_t dev;

#if defined(DMA2D)
    priv.fix.capability = MS_FB_BLIT_OP_FILL
                        | MS_FB_BLIT_OP_FILL_WITH_ALPHA
                        | MS_FB_BLIT_OP_COPY
                        | MS_FB_BLIT_OP_COPY_WITH_ALPHA
                        | MS_FB_BLIT_OP_COPY_ARGB8888
                        | MS_FB_BLIT_OP_COPY_ARGB8888_WITH_ALPHA
                        | MS_FB_BLIT_OP_COPY_A4
                        | MS_FB_BLIT_OP_COPY_A8;
#endif

    priv.var.xres            = BSP_CFG_LCD_WIDTH;
    priv.var.yres            = BSP_CFG_LCD_HEIGHT;
    priv.var.xres_virtual    = BSP_CFG_LCD_WIDTH;
    priv.var.yres_virtual    = BSP_CFG_LCD_HEIGHT;
    priv.var.xoffset         = 0U;
    priv.var.yoffset         = 0U;
    priv.var.bits_per_pixel  = BSP_CFG_LCD_BPP;

#if BSP_CFG_LCD_BPP == 16U
    priv.var.red.offset      = 11U;
    priv.var.red.length      = 5U;
    priv.var.red.msb_right   = 0U;

    priv.var.green.offset    = 5U;
    priv.var.green.length    = 6U;
    priv.var.green.msb_right = 0U;

    priv.var.blue.offset     = 0U;
    priv.var.blue.length     = 5U;
    priv.var.green.msb_right = 0U;
#else
    priv.var.red.offset      = 16U;
    priv.var.red.length      = 8U;
    priv.var.red.msb_right   = 0U;

    priv.var.green.offset    = 8U;
    priv.var.green.length    = 8U;
    priv.var.green.msb_right = 0U;

    priv.var.blue.offset     = 0U;
    priv.var.blue.length     = 8U;
    priv.var.green.msb_right = 0U;
#endif

    priv.fix.smem_start      = BSP_CFG_FB_RAM_BASE;
    priv.fix.smem_len        = LCD_BYTE_PER_PIXEL * BSP_CFG_LCD_WIDTH * BSP_CFG_LCD_HEIGHT;
    priv.fix.line_length     = LCD_BYTE_PER_PIXEL * BSP_CFG_LCD_WIDTH;

#if defined(BSP_CFG_FB1_RAM_BASE)
    priv.fix.smem1_start      = BSP_CFG_FB1_RAM_BASE;
    priv.fix.smem1_len        = LCD_BYTE_PER_PIXEL * BSP_CFG_LCD_WIDTH * BSP_CFG_LCD_HEIGHT;
    priv.fix.capability      |= MS_FB_CAP_DOUBLE_FB;
#endif

    return ms_io_device_register(&dev, path, "stm32_fb", &priv);
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/
