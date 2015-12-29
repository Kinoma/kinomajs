SECTIONS
{
  . = 0 + SIZEOF_HEADERS;
  .text           : { *(.text .text.*) }
  .rodata         : { *(.rodata .rodata.*) }
  .data           :
  {
    *(.data .data.*)
  }
  .bss            :
  {
   *(.bss .bss.*)
   . = ALIGN(. != 0 ? 32 / 8 : 1);
  }
  .plt            : { *(.plt) }
  .got            : { *(.got.plt) *(.got) }
  .rel.dyn        : { *(.rel.dyn) }
  .rel.plt        : { *(.rel.plt) }

  .hash           : { *(.hash) }
  .gnu.hash       : { *(.gnu.hash) }
  .dynsym         : { *(.dynsym) }
  .dynstr         : { *(.dynstr) }
  .version        : { *(.version) }
  .version_d      : { *(.version_d) }
  .version_r      : { *(.version_r) }
  .init           : { KEEP (*(.init)) }
  .fini           : { KEEP (*(.fini)) }

  .ARM.extab   : { *(.ARM.extab*) }
   __exidx_start = .;
  .ARM.exidx   : { *(.ARM.exidx*) }
   __exidx_end = .;
  .interp : { *(.interp) }

  .tdata	  : { *(.tdata .tdata.*) }
  .tbss		  : { *(.tbss .tbss.*) }
  .preinit_array     :
  {
     PROVIDE_HIDDEN (__preinit_array_start = .); 
     KEEP (*(.preinit_array))
     PROVIDE_HIDDEN (__preinit_array_end = .); 
  }
  .init_array     :
  {
     PROVIDE_HIDDEN (__init_array_start = .); 
     KEEP (*(SORT(.init_array.*)))
     KEEP (*(.init_array))
     PROVIDE_HIDDEN (__init_array_end = .); 
  }
  .fini_array     :
  {
     PROVIDE_HIDDEN (__fini_array_start = .); 
     KEEP (*(.fini_array))
     KEEP (*(SORT(.fini_array.*)))
     PROVIDE_HIDDEN (__fini_array_end = .); 
  }
  .dynamic        : { *(.dynamic) }
}
