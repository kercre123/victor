            .data
            .global ble_send
            .asciz "Current Version"
            .word main
            .word deinit
            .word tick
            .word recv
ble_send:   .word nullcall
            .byte 0x3d, 0x70, 0x7a, 0x86, 0x10, 0x3c, 0xef, 0x4f, 0x83, 0x2f, 0xa8, 0xa6, 0x64, 0x20, 0xba, 0xdf

            .text
		.thumb
nullcall:	bx lr
