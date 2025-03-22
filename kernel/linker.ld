ENTRY(_kernel_boot)
SECTIONS
{
    . = 0x80000;
    PROVIDE(_kernel_start = .);
    .text.boot : {
        *(.text.boot)
        KEEP(*(.text.boot))
    }
    .text : {
        PROVIDE(_text_section_start = .);
        *(.text .text.* .gnu.linkonce.t*)
        PROVIDE(_text_section_end = .);
    }
    .rodata : { 
        PROVIDE(_rodata_section_start = .);
        *(.rodata .rodata.* .gnu.linkonce.r*)
        PROVIDE(_rodata_section_end = .); 
    }
    .data : {
        PROVIDE(_data_section_start = .);
        *(.data .data.* .gnu.linkonce.d*)
        PROVIDE(_data_section_end = .);
    }
    .bss : {
        PROVIDE(_bss_section_start = .);
        *(.bss .bss.*)
        *(COMMON)
        PROVIDE(_bss_section_end = .);
    }
    .heap (NOLOAD) : {
        PROVIDE(_heap_start = .);
        . = . + 0x6000000;  /* 分配 1MB 給 heap */
        PROVIDE(_heap_end = .);
    }
    _bss_size = (_bss_section_end - _bss_section_start);
    PROVIDE(_kernel_end = .);
}
