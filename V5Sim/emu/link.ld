/* Fake VEXos lives in low DDR, below the VEXos jump table (0x037FC000)
   and the PROS cold (0x03800000) / heap (0x04A00000) / hot (0x07800000) regions. */
ENTRY(_start)
MEMORY { SHIM : ORIGIN = 0x00100000, LENGTH = 0x00E00000 }
SECTIONS {
  .text : { *(.text.boot) *(.text*) *(.rodata*) } > SHIM
  .ARM.exidx : { *(.ARM.exidx*) } > SHIM
  .data : { *(.data*) } > SHIM
  .bss (NOLOAD) : {
    . = ALIGN(16384);
    __bss_start = .;
    *(.bss.ttb)
    *(.bss*) *(COMMON)
    . = ALIGN(8);
    . += 0x4000; __irq_stack_top = .;
    . += 0x1000; __fiq_stack_top = .;
    . += 0x1000; __abt_stack_top = .;
    . += 0x1000; __und_stack_top = .;
    . += 0x4000; __sys_stack_top = .;
    . += 0x8000; __svc_stack_top = .;
    __bss_end = .;
  } > SHIM
  end = .;
}
