OUTPUT_ARCH( "riscv" )


_HEAP_SIZE  = 0x400;

MEMORY {

        npu_384k_sram : ORIGIN = 0x1e800000, LENGTH = 0x00060000
        npu_16k_sram : ORIGIN = 0x1e900000, LENGTH = 0x00004000
}



ENTRY( _start )


SECTIONS
{
  

  .init           :
  {
    KEEP (*(SORT_NONE(.init)))
  } >npu_384k_sram

  .text           :
  {
    *(.text.unlikely .text.unlikely.*)
    *(.text.startup .text.startup.*)
    *(.text .text.*)
    *(.gnu.linkonce.t.*)
  } >npu_384k_sram

  .fini           :
  {
    KEEP (*(SORT_NONE(.fini)))
  } >npu_384k_sram

  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);

  .rodata         :
  {
    *(.rdata)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)
  } >npu_384k_sram

  . = ALIGN(4);

  .preinit_array  :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >npu_384k_sram

  .init_array     :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >npu_384k_sram

  .fini_array     :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >npu_384k_sram

  .ctors          :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    /* We don't want to include the .ctor section from
       the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  }  >npu_384k_sram

  .dtors          :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  }  >npu_384k_sram

  .lalign         :
  {
    . = ALIGN(4);
    PROVIDE( _data_lma = . );
  } >npu_384k_sram

  .dalign         :
  {
    . = ALIGN(4);
    PROVIDE( _data = . );
  } >npu_16k_sram

  .data          :
  {
    *(.data .data.*)
    *(.gnu.linkonce.d.*)
    . = ALIGN(8);
    PROVIDE( __global_pointer$ = . + 0x800 );
    *(.sdata .sdata.*)
    *(.gnu.linkonce.s.*)
    . = ALIGN(8);
    *(.srodata.cst16)
    *(.srodata.cst8)
    *(.srodata.cst4)
    *(.srodata.cst2)
    *(.srodata .srodata.*)
  } >npu_16k_sram

  . = ALIGN(4);
  PROVIDE( _edata = . );
  PROVIDE( edata = . );

    .bss : {
       . = ALIGN(4);
       __bss_start = .;
       *(.bss)
       *(.bss.*)
       *(.gnu.linkonce.b.*)
       *(COMMON)
       *(.sbss)
       *(.scommon)
       . = ALIGN(4);
       __bss_end = .;
  } >npu_16k_sram

    .heap : {

       . = ALIGN(8);

       _heap = .;
       _heap_start = .;
       __HEAP_START = .;
       . += _HEAP_SIZE;
       _heap_end = .;
       __HEAP_MAX = .;
  } >npu_16k_sram

  _end = .;
}
