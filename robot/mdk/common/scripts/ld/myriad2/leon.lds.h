/********************************************************************************************************************
   ------------------------------------------------------------------------------------------------------------------
    Sections included from leon.lds.h
    - This block defines the placement of the code/data and stack associated with LeonOS and LeonRT
   ------------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/

ENTRY( start );

SECTIONS {
  . = MV_LEONOS_BEGIN;
  .sys.traps ALIGN(0x1000) : {
    *(.sys.text.traps)
  }
  .sys.text ALIGN(4) : { 
    *(.sys.text.start)
  }
  .text ALIGN(4) : { *(.text*) }
  .sys.rodata ALIGN(4) : { *(.sys.rodata*) }
  .rodata ALIGN(4) : { *(.rodata*) }
  .data ALIGN(4) : { *(.data*) }

  .bss  (NOLOAD): { 
    . = ALIGN(8);
    __bss_start = . ;
    *(.bss*) *(COMMON)
    . = ALIGN(8);
    __bss_end = . ;
  }
  __EMPTY_RAM = ALIGN(4);
  .init_text ALIGN(4) : {
    *(.init_text*)
    __init_text_end = ALIGN(4);
  }
  
  . = MV_LEONOS_END - 0x100;
  _LEON_STACK_POINTER = .;
  .sys.bss ALIGN(4) (NOLOAD) : { 
    . = ALIGN(8);
    __sys_bss_start = . ;
    *(.sys.bss*) 
    . = ALIGN(8);
    __sys_bss_end = . ;
  }

  . = MV_LEONOS_END;
}

SECTIONS {

  . = MV_LEONRT_BEGIN;

  .lrt.sys.traps ALIGN(0x1000) : {
    *(.lrt.sys.text.traps)
  }
  /* LeonRT Entry point has a requirment that the bottom 12 bits
     are 0. Thus 4KB alignment is required */
  .lrt.sys.text ALIGN(0x1000): { 
    *(.lrt.sys.text.start)
  }                  

  .lrt.text : {KEEP( *(.lrt.text*) )}
  .lrt.sys.rodata : { *(.lrt.sys.rodata*) }
  .lrt.rodata : { *(.lrt.rodata*) }
  .lrt.data : { *(.lrt.data*) }

  .lrt.bss (NOLOAD) : { 
    . = ALIGN(8);
    lrt___bss_start = . ;
    *(.lrt.bss*) *(.lrt.COMMON)
    . = ALIGN(8);
    lrt___bss_end = . ;
  }
  __EMPTY_RAM = ALIGN(4);
  .init_text : {
    *(.init_text*)
    __init_text_end = ALIGN(4);
  }

  . = MV_LEONRT_END - 0x100;
  lrt__LEON_STACK_POINTER = .;

  .lrt.sys.bss ALIGN(4) (NOLOAD) : { 
    . = ALIGN(8);
    lrt___sys_bss_start = . ;
    *(.lrt.sys.bss*) 
    . = ALIGN(8);
    lrt___sys_bss_end = . ;
  }
  . = MV_LEONRT_END;
  
  __MIN_STACK_SIZE = DEFINED( __MIN_STACK_SIZE ) ? __MIN_STACK_SIZE : 3584;
  __MIN_INIT_STACK_SIZE = DEFINED( __MIN_INIT_STACK_SIZE ) ? __MIN_INIT_STACK_SIZE : 1024;
  
  __STACK_SIZE = ABSOLUTE(__sys_bss_start - __bss_end);
  ASSERT( __STACK_SIZE > __MIN_STACK_SIZE , "Leon Code + data too big. Stack space left is smaller than the required minimum (3.5 KB)!")
  
  __INIT_STACK_SIZE = ABSOLUTE(__sys_bss_start - __init_text_end);
  ASSERT( __INIT_STACK_SIZE > __MIN_INIT_STACK_SIZE , "Stack available to the init code is smaller than the specified minimum (which by default is 1 KB)!")
  
  /DISCARD/ : {
    *(.stab .stabstr .comment .gnu.attributes)
  }
}
