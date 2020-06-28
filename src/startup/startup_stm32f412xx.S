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
** 文   件   名: startup_stm32f412xx.S
**
** 创   建   人: Jiao.jinxing
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: stm32f412xx 芯片启动代码
*********************************************************************************************************/

/*********************************************************************************************************
  Configuration
*********************************************************************************************************/

#include "config.h"

#undef __PERFORMANCE_IMPLEMENTATION
#define __ARMV7M_SCB_BASE           0xe000ed00
#define __ARMV7M_SCB_VTOR           0x8
#define __ARMV7M_SCB_DCCMVAC        0x268
#define __ARMV7M_SCB_ICIMVAU        0x258
#define __ARMV7M_CACHE_LINE_SIZE    32

    .syntax unified
    .arch armv7-m

/*********************************************************************************************************
  Exception vector table
*********************************************************************************************************/

    .section .isr_vector, "a"
    .align 2
    .globl __isr_vector
__isr_vector:
    .word   __ms_boot_stack_start__                         /* Top of Stack                             */
    .word   Reset_Handler                                   /* Reset Handler                            */
    .word   NMI_Handler                                     /* NMI Handler                              */
    .word   HardFault_Handler                               /* Hard Fault Handler                       */
    .word   MemManage_Handler                               /* MPU Fault Handler                        */
    .word   BusFault_Handler                                /* Bus Fault Handler                        */
    .word   UsageFault_Handler                              /* Usage Fault Handler                      */
    .word   0                                               /* Reserved                                 */
    .word   0                                               /* Reserved                                 */
    .word   0                                               /* Reserved                                 */
    .word   0                                               /* Reserved                                 */
    .word   SVC_Handler                                     /* SVCall Handler                           */
    .word   DebugMon_Handler                                /* Reserved                                 */
    .word   0                                               /* Reserved                                 */
    .word   PendSV_Handler                                  /* PendSV Handler                           */
    .word   SysTick_Handler                                 /* SysTick Handler                          */

    /* External Interrupts */
    .word   WWDG_IRQHandler                   /* Window WatchDog                             */
    .word   PVD_IRQHandler                    /* PVD through EXTI Line detection             */
    .word   TAMP_STAMP_IRQHandler             /* Tamper and TimeStamps through the EXTI line */
    .word   RTC_WKUP_IRQHandler               /* RTC Wakeup through the EXTI line            */
    .word   FLASH_IRQHandler                  /* FLASH                                       */
    .word   RCC_IRQHandler                    /* RCC                                         */
    .word   EXTI0_IRQHandler                  /* EXTI Line0                                  */
    .word   EXTI1_IRQHandler                  /* EXTI Line1                                  */
    .word   EXTI2_IRQHandler                  /* EXTI Line2                                  */
    .word   EXTI3_IRQHandler                  /* EXTI Line3                                  */
    .word   EXTI4_IRQHandler                  /* EXTI Line4                                  */
    .word   DMA1_Stream0_IRQHandler           /* DMA1 Stream 0                               */
    .word   DMA1_Stream1_IRQHandler           /* DMA1 Stream 1                               */
    .word   DMA1_Stream2_IRQHandler           /* DMA1 Stream 2                               */
    .word   DMA1_Stream3_IRQHandler           /* DMA1 Stream 3                               */
    .word   DMA1_Stream4_IRQHandler           /* DMA1 Stream 4                               */
    .word   DMA1_Stream5_IRQHandler           /* DMA1 Stream 5                               */
    .word   DMA1_Stream6_IRQHandler           /* DMA1 Stream 6                               */
    .word   ADC_IRQHandler                    /* ADC1, ADC2 and ADC3s                        */
    .word   CAN1_TX_IRQHandler                /* CAN1 TX                                     */
    .word   CAN1_RX0_IRQHandler               /* CAN1 RX0                                    */
    .word   CAN1_RX1_IRQHandler               /* CAN1 RX1                                    */
    .word   CAN1_SCE_IRQHandler               /* CAN1 SCE                                    */
    .word   EXTI9_5_IRQHandler                /* External Line[9:5]s                         */
    .word   TIM1_BRK_TIM9_IRQHandler          /* TIM1 Break and TIM9                         */
    .word   TIM1_UP_TIM10_IRQHandler          /* TIM1 Update and TIM10                       */
    .word   TIM1_TRG_COM_TIM11_IRQHandler     /* TIM1 Trigger and Commutation and TIM11      */
    .word   TIM1_CC_IRQHandler                /* TIM1 Capture Compare                        */
    .word   TIM2_IRQHandler                   /* TIM2                                        */
    .word   TIM3_IRQHandler                   /* TIM3                                        */
    .word   TIM4_IRQHandler                   /* TIM4                                        */
    .word   I2C1_EV_IRQHandler                /* I2C1 Event                                  */
    .word   I2C1_ER_IRQHandler                /* I2C1 Error                                  */
    .word   I2C2_EV_IRQHandler                /* I2C2 Event                                  */
    .word   I2C2_ER_IRQHandler                /* I2C2 Error                                  */
    .word   SPI1_IRQHandler                   /* SPI1                                        */
    .word   SPI2_IRQHandler                   /* SPI2                                        */
    .word   USART1_IRQHandler                 /* USART1                                      */
    .word   USART2_IRQHandler                 /* USART2                                      */
    .word   USART3_IRQHandler                 /* USART3                                      */
    .word   EXTI15_10_IRQHandler              /* External Line[15:10]s                       */
    .word   RTC_Alarm_IRQHandler              /* RTC Alarm (A and B) through EXTI Line       */
    .word   OTG_FS_WKUP_IRQHandler            /* USB OTG FS Wakeup through EXTI line         */
    .word   TIM8_BRK_TIM12_IRQHandler         /* TIM8 Break and TIM12                        */
    .word   TIM8_UP_TIM13_IRQHandler          /* TIM8 Update and TIM13                       */
    .word   TIM8_TRG_COM_TIM14_IRQHandler     /* TIM8 Trigger and Commutation and TIM14      */
    .word   TIM8_CC_IRQHandler                /* TIM8 Capture Compare                        */
    .word   DMA1_Stream7_IRQHandler           /* DMA1 Stream7                                */
    .word   0                                 /* Reserved                                    */
    .word   SDIO_IRQHandler                   /* SDIO                                        */
    .word   TIM5_IRQHandler                   /* TIM5                                        */
    .word   SPI3_IRQHandler                   /* SPI3                                        */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   TIM6_IRQHandler                   /* TIM6                                        */
    .word   TIM7_IRQHandler                   /* TIM7                                        */
    .word   DMA2_Stream0_IRQHandler           /* DMA2 Stream 0                               */
    .word   DMA2_Stream1_IRQHandler           /* DMA2 Stream 1                               */
    .word   DMA2_Stream2_IRQHandler           /* DMA2 Stream 2                               */
    .word   DMA2_Stream3_IRQHandler           /* DMA2 Stream 3                               */
    .word   DMA2_Stream4_IRQHandler           /* DMA2 Stream 4                               */
    .word   DFSDM1_FLT0_IRQHandler            /* DFSDM1 Filter0                              */
    .word   DFSDM1_FLT1_IRQHandler            /* DFSDM1 Filter1                              */
    .word   CAN2_TX_IRQHandler                /* CAN2 TX                                     */
    .word   CAN2_RX0_IRQHandler               /* CAN2 RX0                                    */
    .word   CAN2_RX1_IRQHandler               /* CAN2 RX1                                    */
    .word   CAN2_SCE_IRQHandler               /* CAN2 SCE                                    */
    .word   OTG_FS_IRQHandler                 /* USB OTG FS                                  */
    .word   DMA2_Stream5_IRQHandler           /* DMA2 Stream 5                               */
    .word   DMA2_Stream6_IRQHandler           /* DMA2 Stream 6                               */
    .word   DMA2_Stream7_IRQHandler           /* DMA2 Stream 7                               */
    .word   USART6_IRQHandler                 /* USART6                                      */
    .word   I2C3_EV_IRQHandler                /* I2C3 event                                  */
    .word   I2C3_ER_IRQHandler                /* I2C3 error                                  */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   RNG_IRQHandler                    /* RNG                                         */
    .word   FPU_IRQHandler                    /* FPU                                         */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   SPI4_IRQHandler                   /* SPI4                                        */
    .word   SPI5_IRQHandler                   /* SPI5                                        */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   QUADSPI_IRQHandler                /* QuadSPI                                     */
    .word   0                                 /* Reserved                                    */
    .word   0                                 /* Reserved                                    */
    .word   FMPI2C1_EV_IRQHandler             /* FMPI2C1 Event                               */
    .word   FMPI2C1_ER_IRQHandler             /* FMPI2C1 Error                               */

    .size    __isr_vector, . - __isr_vector

/*********************************************************************************************************
  mov32 - loads a 32-bit value into a register without a data access
*********************************************************************************************************/

    .macro  mov32 rd, imm32
    movw    \rd, #:lower16:\imm32
    .if     \imm32 & 0xffff0000
    movt    \rd, #:upper16:\imm32
    .endif
    .endm

/*********************************************************************************************************
  Reset Handler
*********************************************************************************************************/

    .text
    .thumb

    .thumb_func
    .align 2
    .globl   Reset_Handler
    .weak    Reset_Handler
    .type    Reset_Handler, %function
Reset_Handler:

/*********************************************************************************************************
  Switch to Handler_Mode
*********************************************************************************************************/

    /* Copy __isr_vector to __ms_boot_stack_end__ */

    ldr     r1, =__ms_boot_stack_end__  /* Allocate a temporary vector */
    ldr     r0, =__isr_vector

    ldr     r2, [r0, #0 * 4]
    str     r2, [r1, #0 * 4]

    ldr     r2, [r0, #1 * 4]
    str     r2, [r1, #1 * 4]

    ldr     r2, [r0, #2 * 4]
    str     r2, [r1, #2 * 4]

    ldr     r2, [r0, #3 * 4]
    str     r2, [r1, #3 * 4]

    ldr     r2, [r0, #4 * 4]
    str     r2, [r1, #4 * 4]

    ldr     r2, [r0, #5 * 4]
    str     r2, [r1, #5 * 4]

    ldr     r2, [r0, #6 * 4]
    str     r2, [r1, #6 * 4]

    ldr     r2, [r0, #7 * 4]
    str     r2, [r1, #7 * 4]

    ldr     r2, [r0, #8 * 4]
    str     r2, [r1, #8 * 4]

    ldr     r2, [r0, #9 * 4]
    str     r2, [r1, #9 * 4]

    ldr     r2, [r0, #10 * 4]
    str     r2, [r1, #10 * 4]

    /* SVC handle jump to 0f */
    ldr     r2, =0f
    orr     r2, #1                              /* Set thumb bit */
    str     r2, [r1, #11 * 4]

    ldr     r2, [r0, #12 * 4]
    str     r2, [r1, #12 * 4]

    ldr     r2, [r0, #13 * 4]
    str     r2, [r1, #13 * 4]

    ldr     r2, [r0, #14 * 4]
    str     r2, [r1, #14 * 4]

    ldr     r2, [r0, #15 * 4]
    str     r2, [r1, #15 * 4]
    dsb
    isb

    mov32   r0, __ARMV7M_SCB_BASE               /* SCB_BASE */
    str     r1, [r0, #__ARMV7M_SCB_VTOR]        /* Write VTOR */
    dsb
    isb

    svc     #0
0:

/*********************************************************************************************************
  Restore the early vector
*********************************************************************************************************/

    ldr     r1, =__vector_start__
    str     r1, [r0, #__ARMV7M_SCB_VTOR]        /* Restore VTOR */
    dsb                                         /* [1] */

/*********************************************************************************************************
  Loop to copy data from read only memory to RAM. The ranges
  of copy from/to are specified by following symbols evaluated in
  linker script.
  __etext: End of code section, i.e., begin of data sections to copy from.
  __data_start__/__data_end__: RAM address range that data should be
  __noncachedata_start__/__noncachedata_end__ : none cachable region
  __ram_function_start__/__ram_function_end__ : ramfunction region
  copied to. Both must be aligned to 4 bytes boundary.
*********************************************************************************************************/

    ldr    r1, =_sidata
    ldr    r2, =_sdata
    ldr    r3, =_edata

/*********************************************************************************************************
  Avoid clear zero .ms_segger_rtt section if using bootloader
*********************************************************************************************************/

#if BSP_CFG_USE_BOOTLOADER > 0
    ldr    r4, =__ms_segger_rtt_start__
    ldr    r5, =__ms_segger_rtt_end__
    sub    r4, r5, r4

    add    r1, r1, r4
    add    r2, r2, r4
#endif

/*********************************************************************************************************
  Here are two copies of loop implementations. First one favors performance
  and the second one favors code size. Default uses the second one.
  Define macro "__PERFORMANCE_IMPLEMENTATION" in project to use the first one
*********************************************************************************************************/

#ifdef __PERFORMANCE_IMPLEMENTATION
    subs    r3, r2
    ble    .LC1
.LC0:
    subs    r3, #4
    ldr    r0, [r1, r3]
    str    r0, [r2, r3]
    bgt    .LC0
.LC1:
#else  /* code size implemenation */
.LC0:
    cmp     r2, r3
    ittt    lt
    ldrlt   r0, [r1], #4
    strlt   r0, [r2], #4
    blt    .LC0
#endif

/*********************************************************************************************************
  This part of work usually is done in C library startup code. Otherwise,
  define this macro to enable it in this startup.

  Loop to zero out BSS section, which uses following symbols
  in linker script:
  __bss_start__: start of BSS section. Must align to 4
  __bss_end__: end of BSS section. Must align to 4
*********************************************************************************************************/

    ldr     r1, =__bss_start__
    ldr     r2, =__bss_end__

    movs    r0, 0
.LC5:
    cmp     r1, r2
    itt    lt
    strlt   r0, [r1], #4
    blt    .LC5

/*********************************************************************************************************
  CMSIS system init(Will initial VTOR)
*********************************************************************************************************/

#if BSP_CFG_USE_BOOTLOADER == 0
    ldr     r0, =SystemInit
    blx     r0
#endif

/*********************************************************************************************************
  Call bsp_init
*********************************************************************************************************/

    ldr   r0,=bsp_init
    blx   r0
    .size Reset_Handler, . - Reset_Handler

/*********************************************************************************************************
  Macro to define default handlers.
  Default handler will be weak symbol and just dead loops.
  They can be overwritten by other handlers
*********************************************************************************************************/

    .macro def_irq_handler  handler_name
    .align  1
    .thumb_func
    .weak \handler_name
    .type \handler_name, %function
\handler_name:
    b \handler_name
    .size \handler_name, . - \handler_name
    .endm

    def_irq_handler   WWDG_IRQHandler                   /* Window WatchDog                             */
    def_irq_handler   PVD_IRQHandler                    /* PVD through EXTI Line detection             */
    def_irq_handler   TAMP_STAMP_IRQHandler             /* Tamper and TimeStamps through the EXTI line */
    def_irq_handler   RTC_WKUP_IRQHandler               /* RTC Wakeup through the EXTI line            */
    def_irq_handler   FLASH_IRQHandler                  /* FLASH                                       */
    def_irq_handler   RCC_IRQHandler                    /* RCC                                         */
    def_irq_handler   EXTI0_IRQHandler                  /* EXTI Line0                                  */
    def_irq_handler   EXTI1_IRQHandler                  /* EXTI Line1                                  */
    def_irq_handler   EXTI2_IRQHandler                  /* EXTI Line2                                  */
    def_irq_handler   EXTI3_IRQHandler                  /* EXTI Line3                                  */
    def_irq_handler   EXTI4_IRQHandler                  /* EXTI Line4                                  */
    def_irq_handler   DMA1_Stream0_IRQHandler           /* DMA1 Stream 0                               */
    def_irq_handler   DMA1_Stream1_IRQHandler           /* DMA1 Stream 1                               */
    def_irq_handler   DMA1_Stream2_IRQHandler           /* DMA1 Stream 2                               */
    def_irq_handler   DMA1_Stream3_IRQHandler           /* DMA1 Stream 3                               */
    def_irq_handler   DMA1_Stream4_IRQHandler           /* DMA1 Stream 4                               */
    def_irq_handler   DMA1_Stream5_IRQHandler           /* DMA1 Stream 5                               */
    def_irq_handler   DMA1_Stream6_IRQHandler           /* DMA1 Stream 6                               */
    def_irq_handler   ADC_IRQHandler                    /* ADC1, ADC2 and ADC3s                        */
    def_irq_handler   CAN1_TX_IRQHandler                /* CAN1 TX                                     */
    def_irq_handler   CAN1_RX0_IRQHandler               /* CAN1 RX0                                    */
    def_irq_handler   CAN1_RX1_IRQHandler               /* CAN1 RX1                                    */
    def_irq_handler   CAN1_SCE_IRQHandler               /* CAN1 SCE                                    */
    def_irq_handler   EXTI9_5_IRQHandler                /* External Line[9:5]s                         */
    def_irq_handler   TIM1_BRK_TIM9_IRQHandler          /* TIM1 Break and TIM9                         */
    def_irq_handler   TIM1_UP_TIM10_IRQHandler          /* TIM1 Update and TIM10                       */
    def_irq_handler   TIM1_TRG_COM_TIM11_IRQHandler     /* TIM1 Trigger and Commutation and TIM11      */
    def_irq_handler   TIM1_CC_IRQHandler                /* TIM1 Capture Compare                        */
    def_irq_handler   TIM2_IRQHandler                   /* TIM2                                        */
    def_irq_handler   TIM3_IRQHandler                   /* TIM3                                        */
    def_irq_handler   TIM4_IRQHandler                   /* TIM4                                        */
    def_irq_handler   I2C1_EV_IRQHandler                /* I2C1 Event                                  */
    def_irq_handler   I2C1_ER_IRQHandler                /* I2C1 Error                                  */
    def_irq_handler   I2C2_EV_IRQHandler                /* I2C2 Event                                  */
    def_irq_handler   I2C2_ER_IRQHandler                /* I2C2 Error                                  */
    def_irq_handler   SPI1_IRQHandler                   /* SPI1                                        */
    def_irq_handler   SPI2_IRQHandler                   /* SPI2                                        */
    def_irq_handler   USART1_IRQHandler                 /* USART1                                      */
    def_irq_handler   USART2_IRQHandler                 /* USART2                                      */
    def_irq_handler   USART3_IRQHandler                 /* USART3                                      */
    def_irq_handler   EXTI15_10_IRQHandler              /* External Line[15:10]s                       */
    def_irq_handler   RTC_Alarm_IRQHandler              /* RTC Alarm (A and B) through EXTI Line       */
    def_irq_handler   OTG_FS_WKUP_IRQHandler            /* USB OTG FS Wakeup through EXTI line         */
    def_irq_handler   TIM8_BRK_TIM12_IRQHandler         /* TIM8 Break and TIM12                        */
    def_irq_handler   TIM8_UP_TIM13_IRQHandler          /* TIM8 Update and TIM13                       */
    def_irq_handler   TIM8_TRG_COM_TIM14_IRQHandler     /* TIM8 Trigger and Commutation and TIM14      */
    def_irq_handler   TIM8_CC_IRQHandler                /* TIM8 Capture Compare                        */
    def_irq_handler   DMA1_Stream7_IRQHandler           /* DMA1 Stream7                                */
    def_irq_handler   SDIO_IRQHandler                   /* SDIO                                        */
    def_irq_handler   TIM5_IRQHandler                   /* TIM5                                        */
    def_irq_handler   SPI3_IRQHandler                   /* SPI3                                        */
    def_irq_handler   TIM6_IRQHandler                   /* TIM6                                        */
    def_irq_handler   TIM7_IRQHandler                   /* TIM7                                        */
    def_irq_handler   DMA2_Stream0_IRQHandler           /* DMA2 Stream 0                               */
    def_irq_handler   DMA2_Stream1_IRQHandler           /* DMA2 Stream 1                               */
    def_irq_handler   DMA2_Stream2_IRQHandler           /* DMA2 Stream 2                               */
    def_irq_handler   DMA2_Stream3_IRQHandler           /* DMA2 Stream 3                               */
    def_irq_handler   DMA2_Stream4_IRQHandler           /* DMA2 Stream 4                               */
    def_irq_handler   DFSDM1_FLT0_IRQHandler            /* DFSDM1 Filter0                              */
    def_irq_handler   DFSDM1_FLT1_IRQHandler            /* DFSDM1 Filter1                              */
    def_irq_handler   CAN2_TX_IRQHandler                /* CAN2 TX                                     */
    def_irq_handler   CAN2_RX0_IRQHandler               /* CAN2 RX0                                    */
    def_irq_handler   CAN2_RX1_IRQHandler               /* CAN2 RX1                                    */
    def_irq_handler   CAN2_SCE_IRQHandler               /* CAN2 SCE                                    */
    def_irq_handler   OTG_FS_IRQHandler                 /* USB OTG FS                                  */
    def_irq_handler   DMA2_Stream5_IRQHandler           /* DMA2 Stream 5                               */
    def_irq_handler   DMA2_Stream6_IRQHandler           /* DMA2 Stream 6                               */
    def_irq_handler   DMA2_Stream7_IRQHandler           /* DMA2 Stream 7                               */
    def_irq_handler   USART6_IRQHandler                 /* USART6                                      */
    def_irq_handler   I2C3_EV_IRQHandler                /* I2C3 event                                  */
    def_irq_handler   I2C3_ER_IRQHandler                /* I2C3 error                                  */
    def_irq_handler   RNG_IRQHandler                    /* RNG                                         */
    def_irq_handler   FPU_IRQHandler                    /* FPU                                         */
    def_irq_handler   SPI4_IRQHandler                   /* SPI4                                        */
    def_irq_handler   SPI5_IRQHandler                   /* SPI5                                        */
    def_irq_handler   QUADSPI_IRQHandler                /* QuadSPI                                     */
    def_irq_handler   FMPI2C1_EV_IRQHandler             /* FMPI2C1 Event                               */
    def_irq_handler   FMPI2C1_ER_IRQHandler             /* FMPI2C1 Error                               */

    .end
/*********************************************************************************************************
  END
*********************************************************************************************************/
