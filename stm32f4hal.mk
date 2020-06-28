#*********************************************************************************************************
#
#                                 北京翼辉信息技术有限公司
#
#                                   微型安全实时操作系统
#
#                                       MS-RTOS(TM)
#
#                               Copyright All Rights Reserved
#
#--------------文件信息--------------------------------------------------------------------------------
#
# 文   件   名: stm32f4hal.mk
#
# 创   建   人: Jiao.jinxing
#
# 文件创建日期: 2020 年 01 月 02 日
#
# 描        述: STM32F4 HAL Makefile
#*********************************************************************************************************

#*********************************************************************************************************
# STM32F4xx HAL source list
#*********************************************************************************************************
STM32F4HAL_SRCS := \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_can.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cec.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_crc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cryp.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cryp_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dac.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dac_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dcmi.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dcmi_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dfsdm.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma2d.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dsi.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_eth.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_fmpi2c.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_fmpi2c_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_hash.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_hash_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_hcd.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_irda.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_iwdg.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_lptim.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_ltdc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_ltdc_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_mmc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_nand.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_nor.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pccard.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_qspi.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sai.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sai_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sd.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sdram.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_smartcard.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_smbus.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spdifrx.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sram.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_usart.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_wwdg.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_adc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_crc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_dac.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_dma.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_dma2d.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_exti.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fmc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fsmc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_gpio.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_i2c.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_lptim.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_pwr.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_rcc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_rng.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_rtc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_spi.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_tim.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usart.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c \
src/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_utils.c

#*********************************************************************************************************
# STM32F4xx HAL header file search path
#*********************************************************************************************************
STM32F4HAL_INC_PATH := \
-I"./src/Drivers/STM32F4xx_HAL_Driver/Inc" \
-I"./src/Drivers/CMSIS/Device/ST/STM32F4xx/Include" \
-I"./src/Drivers/CMSIS/Include" \
-I"$(MSRTOS_BASE_PATH)/littlefs/src" \
-I"$(MSRTOS_BASE_PATH)/fatfs/src" \
-I"$(MSRTOS_BASE_PATH)/st_usb_stack/src/usb_host/Core/Inc" \
-I"$(MSRTOS_BASE_PATH)/st_usb_stack/src/usb_host/Class/MSC/Inc" \
-I"$(MSRTOS_BASE_PATH)/st_usb_stack/src/usb_host/Class/HID/Inc" \
-I"$(MSRTOS_BASE_PATH)/esp_at_net/src" \
-I"$(MSRTOS_BASE_PATH)/esp_at_net/src/esp-at-lib/esp_at_lib/src/include" \
-I"$(MSRTOS_BASE_PATH)/esp_at_net/src/esp-at-lib/esp_at_lib/src/include/system/port/msrtos" \
-I"./src/driver" \
-I"./src/common" \
-I"./src" \
-I"."

#*********************************************************************************************************
# STM32F4xx HAL pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
STM32F4HAL_DSYMBOL := \
-DUSE_HAL_DRIVER \
-DUSE_FULL_LL_DRIVER \
-DARM_MATH_CM4 \
-DDATA_IN_ExtSDRAM

#*********************************************************************************************************
# STM32F4xx HAL depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
STM32F4HAL_DEPEND_LIB_PATH := \
-L"$(MSRTOS_BASE_PATH)/littlefs/$(OUTDIR)" \
-L"$(MSRTOS_BASE_PATH)/fatfs/$(OUTDIR)" \
-L"$(MSRTOS_BASE_PATH)/st_usb_stack/$(OUTDIR)" \
-L"$(MSRTOS_BASE_PATH)/esp_at_net/$(OUTDIR)" \
-L"$(MSRTOS_BASE_PATH)/lwip_net/$(OUTDIR)"

#*********************************************************************************************************
# End
#*********************************************************************************************************
