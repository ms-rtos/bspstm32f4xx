/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       MS-RTOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: MS-RTOS-SDRAM.ld
**
** ��   ��   ��: Jiao.jinxing
**
** �ļ���������: 2019 �� 12 �� 14 ��
**
** ��        ��: MS-RTOS SDRAM �������ӽű��ļ�
*********************************************************************************************************/
 
/* Entry Point */
ENTRY(Reset_Handler)

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector : {
    . = ALIGN(4);
    PROVIDE(__vector_start__ = .);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text : {
    . = ALIGN(4);
    PROVIDE (__ms_kern_text_start__ = .);
    *(.ms_encrypt);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    PROVIDE (__ms_kern_text_end__ = .);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    
    . = ALIGN(8);
    PROVIDE (__ms_shell_cmd_start__ = .);
    KEEP(*(.ms_shell_cmd*))
    PROVIDE (__ms_shell_cmd_end__ = .);
    
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */

    KEEP (*(.init))
    KEEP (*(.fini))
    
    PROVIDE(__ctors_start__ = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
    PROVIDE(__ctors_end__ = .);

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM : {
    __exidx_start = .;
      *(.ARM.exidx*)
      __exidx_end = .;
  } >FLASH

  .ARM.attributes : { *(.ARM.attributes) } > FLASH

  /* used by the startup to initialize data */
  . = ALIGN(8);
  _sidata = .;

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : AT ( _sidata ) {
    . = ALIGN(8);
    _sdata = .;        /* create a global symbol at data start */

    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    
    PROVIDE(__dtors_start__ = .);
    KEEP(*(SORT(.dtors.*)))
    KEEP(*(.dtors))
    PROVIDE(__dtors_end__ = .);
    
    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM

  /* Uninitialized data section */
  .bss : {
    . = ALIGN(8);
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  PROVIDE ( end = _ebss );
  PROVIDE ( _end = _ebss );

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack : {
    . = ALIGN(8);
    PROVIDE ( __ms_kern_heap_start__ = . );
    . = . + KERN_HEAP_SIZE;
    
    . = ALIGN(0x80);
    PROVIDE(__ms_boot_stack_end__ = .);
    . = . + BOOT_STACK_SIZE;
    . = ALIGN(8);
    PROVIDE(__ms_boot_stack_start__ = .);
  } >RAM
  
  ._ms_segger_rtt (NOLOAD) : {
    PROVIDE(__ms_segger_rtt_start__ = .);
    *(.ms_segger_rtt)  /* .ms_segger_rtt sections */
    PROVIDE(__ms_segger_rtt_end__ = .);
  } >MS_SEGGER_RTT
  
  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
}

/*********************************************************************************************************
  END
*********************************************************************************************************/