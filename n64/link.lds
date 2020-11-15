STACK_SIZE = 8K;

ENTRY(_start)

MEMORY {
    rom (R) : ORIGIN = 0, LENGTH = 64M
    ram (RWX) : ORIGIN = 0x80000400, LENGTH = 4M - 0x400
}

SECTIONS {
    .header : {
        LONG(0x80371240)
        LONG(0x0000000f)
        LONG(0x80000400)
        LONG(0x0000144c)
        . = 0x1000;
    } >rom

    .text : {
        _text_start = .;
        *(.text.entry)
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.data .data.*)
        _text_end = .;
    } >ram AT>rom

    pakdata : ALIGN(16) {
        . = .;
    } >rom
    _pakdata_offset = LOADADDR(pakdata);

    .bss (NOLOAD) : ALIGN(16) {
        _bss_start = .;
        *(.bss .bss.*)
        /* COMMON and .scommon are used by libgultra.a */
        *(COMMON)
        *(.scommon .scommon.*)
        _bss_end = .;
    } >ram

    uninit (NOLOAD) : ALIGN(16) {
        *(uninit uninit.stack)
    } >ram

    _heap1_start = ALIGN(16);

    cfb 0x80200000 (NOLOAD) : {
        _heap1_end = .;
        *(uninit.cfb)
        . = ALIGN(16);
        _heap2_start = .;
    }

    zb 0x80300000 (NOLOAD) : {
        _heap2_end = .;
        *(uninit.zb)
        . = ALIGN(16);
        _heap3_start = .;
    }

    /DISCARD/ : {
        *(*)
    }
}
