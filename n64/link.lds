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
        *(uninit.*)
    } >ram

    uninit.rdp (NOLOAD) : ALIGN(16) {
        *(uniti.rdp)
    }

    /DISCARD/ : {
        *(*)
    }
}
