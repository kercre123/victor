.version 00.40.00

;=====================================================================
;      _ _____  ______ _____                            _           
;     | |  __ \|  ____/ ____|                          | |          
;     | | |__) | |__ | |  __    ___ _ __   ___ ___   __| | ___ _ __ 
; _   | |  ___/|  __|| | |_ |  / _ \ '_ \ / __/ _ \ / _` |/ _ \ '__|
;| |__| | |    | |___| |__| | |  __/ | | | (_| (_) | (_| |  __/ |   
; \____/|_|    |______\_____|  \___|_| |_|\___\___/ \__,_|\___|_|   
; main JPEG encoder routines
;=====================================================================

.include JpegEncoderDefines.incl

 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ; DCT coefficients (these change as QP changes), but are constant
 ; for the entire frame, so they are not inlined, but inclued.
 ; These tables get precomputed for a given QP.
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 .data .data.DCT_COEFS; 0x1c000040
  TBL_QUANT_Y:
   .incbin "DCT/DCTQuantY.tbl"


; .data DCT_COEFS 0x1c000140
  TBL_QUANT_C:
   .incbin "DCT/DCTQuantCr.tbl"
 
 
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ;"YDC_HT" in C code, but bitstream members are ints)
 ;          and .value member is left aligned at 16bit margin   
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 .data .data.HUFF_TABLES; 0x1c000240
  TBL_HUFF_YDC:
    .int 0x00000002, 0x00000000
    .int 0x00000003, 0x00004000
    .int 0x00000003, 0x00006000
    .int 0x00000003, 0x00008000
    .int 0x00000003, 0x0000a000
    .int 0x00000003, 0x0000c000
    .int 0x00000004, 0x0000e000
    .int 0x00000005, 0x0000f000
    .int 0x00000006, 0x0000f800
    .int 0x00000007, 0x0000fc00
    .int 0x00000008, 0x0000fe00
    .int 0x00000009, 0x0000ff00

 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ;"YAC_HT" in C code, but bitstream members are ints)
 ;          and .value member is left aligned at 16bit margin   
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  TBL_HUFF_YAC:
    .int 0x00000004, 0x0000a000
    .int 0x00000002, 0x00000000
    .int 0x00000002, 0x00004000
    .int 0x00000003, 0x00008000
    .int 0x00000004, 0x0000b000
    .int 0x00000005, 0x0000d000
    .int 0x00000007, 0x0000f000
    .int 0x00000008, 0x0000f800
    .int 0x0000000a, 0x0000fd80
    .int 0x00000010, 0x0000ff82
    .int 0x00000010, 0x0000ff83
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000004, 0x0000c000
    .int 0x00000005, 0x0000d800
    .int 0x00000007, 0x0000f200
    .int 0x00000009, 0x0000fb00
    .int 0x0000000b, 0x0000fec0
    .int 0x00000010, 0x0000ff84
    .int 0x00000010, 0x0000ff85
    .int 0x00000010, 0x0000ff86
    .int 0x00000010, 0x0000ff87
    .int 0x00000010, 0x0000ff88
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000005, 0x0000e000
    .int 0x00000008, 0x0000f900
    .int 0x0000000a, 0x0000fdc0
    .int 0x0000000c, 0x0000ff40
    .int 0x00000010, 0x0000ff89
    .int 0x00000010, 0x0000ff8a
    .int 0x00000010, 0x0000ff8b
    .int 0x00000010, 0x0000ff8c
    .int 0x00000010, 0x0000ff8d
    .int 0x00000010, 0x0000ff8e
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000006, 0x0000e800
    .int 0x00000009, 0x0000fb80
    .int 0x0000000c, 0x0000ff50
    .int 0x00000010, 0x0000ff8f
    .int 0x00000010, 0x0000ff90
    .int 0x00000010, 0x0000ff91
    .int 0x00000010, 0x0000ff92
    .int 0x00000010, 0x0000ff93
    .int 0x00000010, 0x0000ff94
    .int 0x00000010, 0x0000ff95
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000006, 0x0000ec00
    .int 0x0000000a, 0x0000fe00
    .int 0x00000010, 0x0000ff96
    .int 0x00000010, 0x0000ff97
    .int 0x00000010, 0x0000ff98
    .int 0x00000010, 0x0000ff99
    .int 0x00000010, 0x0000ff9a
    .int 0x00000010, 0x0000ff9b
    .int 0x00000010, 0x0000ff9c
    .int 0x00000010, 0x0000ff9d
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000007, 0x0000f400
    .int 0x0000000b, 0x0000fee0
    .int 0x00000010, 0x0000ff9e
    .int 0x00000010, 0x0000ff9f
    .int 0x00000010, 0x0000ffa0
    .int 0x00000010, 0x0000ffa1
    .int 0x00000010, 0x0000ffa2
    .int 0x00000010, 0x0000ffa3
    .int 0x00000010, 0x0000ffa4
    .int 0x00000010, 0x0000ffa5
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000007, 0x0000f600
    .int 0x0000000c, 0x0000ff60
    .int 0x00000010, 0x0000ffa6
    .int 0x00000010, 0x0000ffa7
    .int 0x00000010, 0x0000ffa8
    .int 0x00000010, 0x0000ffa9
    .int 0x00000010, 0x0000ffaa
    .int 0x00000010, 0x0000ffab
    .int 0x00000010, 0x0000ffac
    .int 0x00000010, 0x0000ffad
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000008, 0x0000fa00
    .int 0x0000000c, 0x0000ff70
    .int 0x00000010, 0x0000ffae
    .int 0x00000010, 0x0000ffaf
    .int 0x00000010, 0x0000ffb0
    .int 0x00000010, 0x0000ffb1
    .int 0x00000010, 0x0000ffb2
    .int 0x00000010, 0x0000ffb3
    .int 0x00000010, 0x0000ffb4
    .int 0x00000010, 0x0000ffb5
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000009, 0x0000fc00
    .int 0x0000000f, 0x0000ff80
    .int 0x00000010, 0x0000ffb6
    .int 0x00000010, 0x0000ffb7
    .int 0x00000010, 0x0000ffb8
    .int 0x00000010, 0x0000ffb9
    .int 0x00000010, 0x0000ffba
    .int 0x00000010, 0x0000ffbb
    .int 0x00000010, 0x0000ffbc
    .int 0x00000010, 0x0000ffbd
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000009, 0x0000fc80
    .int 0x00000010, 0x0000ffbe
    .int 0x00000010, 0x0000ffbf
    .int 0x00000010, 0x0000ffc0
    .int 0x00000010, 0x0000ffc1
    .int 0x00000010, 0x0000ffc2
    .int 0x00000010, 0x0000ffc3
    .int 0x00000010, 0x0000ffc4
    .int 0x00000010, 0x0000ffc5
    .int 0x00000010, 0x0000ffc6
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000009, 0x0000fd00
    .int 0x00000010, 0x0000ffc7
    .int 0x00000010, 0x0000ffc8
    .int 0x00000010, 0x0000ffc9
    .int 0x00000010, 0x0000ffca
    .int 0x00000010, 0x0000ffcb
    .int 0x00000010, 0x0000ffcc
    .int 0x00000010, 0x0000ffcd
    .int 0x00000010, 0x0000ffce
    .int 0x00000010, 0x0000ffcf
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x0000000a, 0x0000fe40
    .int 0x00000010, 0x0000ffd0
    .int 0x00000010, 0x0000ffd1
    .int 0x00000010, 0x0000ffd2
    .int 0x00000010, 0x0000ffd3
    .int 0x00000010, 0x0000ffd4
    .int 0x00000010, 0x0000ffd5
    .int 0x00000010, 0x0000ffd6
    .int 0x00000010, 0x0000ffd7
    .int 0x00000010, 0x0000ffd8
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x0000000a, 0x0000fe80
    .int 0x00000010, 0x0000ffd9
    .int 0x00000010, 0x0000ffda
    .int 0x00000010, 0x0000ffdb
    .int 0x00000010, 0x0000ffdc
    .int 0x00000010, 0x0000ffdd
    .int 0x00000010, 0x0000ffde
    .int 0x00000010, 0x0000ffdf
    .int 0x00000010, 0x0000ffe0
    .int 0x00000010, 0x0000ffe1
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x0000000b, 0x0000ff00
    .int 0x00000010, 0x0000ffe2
    .int 0x00000010, 0x0000ffe3
    .int 0x00000010, 0x0000ffe4
    .int 0x00000010, 0x0000ffe5
    .int 0x00000010, 0x0000ffe6
    .int 0x00000010, 0x0000ffe7
    .int 0x00000010, 0x0000ffe8
    .int 0x00000010, 0x0000ffe9
    .int 0x00000010, 0x0000ffea
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000010, 0x0000ffeb
    .int 0x00000010, 0x0000ffec
    .int 0x00000010, 0x0000ffed
    .int 0x00000010, 0x0000ffee
    .int 0x00000010, 0x0000ffef
    .int 0x00000010, 0x0000fff0
    .int 0x00000010, 0x0000fff1
    .int 0x00000010, 0x0000fff2
    .int 0x00000010, 0x0000fff3
    .int 0x00000010, 0x0000fff4
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x0000000b, 0x0000ff20
    .int 0x00000010, 0x0000fff5
    .int 0x00000010, 0x0000fff6
    .int 0x00000010, 0x0000fff7
    .int 0x00000010, 0x0000fff8
    .int 0x00000010, 0x0000fff9
    .int 0x00000010, 0x0000fffa
    .int 0x00000010, 0x0000fffb
    .int 0x00000010, 0x0000fffc
    .int 0x00000010, 0x0000fffd
    .int 0x00000010, 0x0000fffe
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000

 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ;"CbDC_HT" in C code, but bitstream members are ints)
 ;          and .value member is left aligned at 16bit margin   
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  TBL_HUFF_CDC:
    .int 0x00000002, 0x00000000
    .int 0x00000002, 0x00004000
    .int 0x00000002, 0x00008000
    .int 0x00000003, 0x0000c000
    .int 0x00000004, 0x0000e000
    .int 0x00000005, 0x0000f000
    .int 0x00000006, 0x0000f800
    .int 0x00000007, 0x0000fc00
    .int 0x00000008, 0x0000fe00
    .int 0x00000009, 0x0000ff00
    .int 0x0000000a, 0x0000ff80
    .int 0x0000000b, 0x0000ffc0

 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ;"CbAC_HT" in C code, but bitstream members are ints)
 ;          and .value member is left aligned at 16bit margin   
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  TBL_HUFF_CAC: 
    .int 0x00000002, 0x00000000
    .int 0x00000002, 0x00004000
    .int 0x00000003, 0x00008000
    .int 0x00000004, 0x0000a000
    .int 0x00000005, 0x0000c000
    .int 0x00000005, 0x0000c800
    .int 0x00000006, 0x0000e000
    .int 0x00000007, 0x0000f000
    .int 0x00000009, 0x0000fa00
    .int 0x0000000a, 0x0000fd80
    .int 0x0000000c, 0x0000ff40
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000004, 0x0000b000
    .int 0x00000006, 0x0000e400
    .int 0x00000008, 0x0000f600
    .int 0x00000009, 0x0000fa80
    .int 0x0000000b, 0x0000fec0
    .int 0x0000000c, 0x0000ff50
    .int 0x00000010, 0x0000ff88
    .int 0x00000010, 0x0000ff89
    .int 0x00000010, 0x0000ff8a
    .int 0x00000010, 0x0000ff8b
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000005, 0x0000d000
    .int 0x00000008, 0x0000f700
    .int 0x0000000a, 0x0000fdc0
    .int 0x0000000c, 0x0000ff60
    .int 0x0000000f, 0x0000ff84
    .int 0x00000010, 0x0000ff8c
    .int 0x00000010, 0x0000ff8d
    .int 0x00000010, 0x0000ff8e
    .int 0x00000010, 0x0000ff8f
    .int 0x00000010, 0x0000ff90
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000005, 0x0000d800
    .int 0x00000008, 0x0000f800
    .int 0x0000000a, 0x0000fe00
    .int 0x0000000c, 0x0000ff70
    .int 0x00000010, 0x0000ff91
    .int 0x00000010, 0x0000ff92
    .int 0x00000010, 0x0000ff93
    .int 0x00000010, 0x0000ff94
    .int 0x00000010, 0x0000ff95
    .int 0x00000010, 0x0000ff96
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000006, 0x0000e800
    .int 0x00000009, 0x0000fb00
    .int 0x00000010, 0x0000ff97
    .int 0x00000010, 0x0000ff98
    .int 0x00000010, 0x0000ff99
    .int 0x00000010, 0x0000ff9a
    .int 0x00000010, 0x0000ff9b
    .int 0x00000010, 0x0000ff9c
    .int 0x00000010, 0x0000ff9d
    .int 0x00000010, 0x0000ff9e
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000006, 0x0000ec00
    .int 0x0000000a, 0x0000fe40
    .int 0x00000010, 0x0000ff9f
    .int 0x00000010, 0x0000ffa0
    .int 0x00000010, 0x0000ffa1
    .int 0x00000010, 0x0000ffa2
    .int 0x00000010, 0x0000ffa3
    .int 0x00000010, 0x0000ffa4
    .int 0x00000010, 0x0000ffa5
    .int 0x00000010, 0x0000ffa6
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000007, 0x0000f200
    .int 0x0000000b, 0x0000fee0
    .int 0x00000010, 0x0000ffa7
    .int 0x00000010, 0x0000ffa8
    .int 0x00000010, 0x0000ffa9
    .int 0x00000010, 0x0000ffaa
    .int 0x00000010, 0x0000ffab
    .int 0x00000010, 0x0000ffac
    .int 0x00000010, 0x0000ffad
    .int 0x00000010, 0x0000ffae
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000007, 0x0000f400
    .int 0x0000000b, 0x0000ff00
    .int 0x00000010, 0x0000ffaf
    .int 0x00000010, 0x0000ffb0
    .int 0x00000010, 0x0000ffb1
    .int 0x00000010, 0x0000ffb2
    .int 0x00000010, 0x0000ffb3
    .int 0x00000010, 0x0000ffb4
    .int 0x00000010, 0x0000ffb5
    .int 0x00000010, 0x0000ffb6
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000008, 0x0000f900
    .int 0x00000010, 0x0000ffb7
    .int 0x00000010, 0x0000ffb8
    .int 0x00000010, 0x0000ffb9
    .int 0x00000010, 0x0000ffba
    .int 0x00000010, 0x0000ffbb
    .int 0x00000010, 0x0000ffbc
    .int 0x00000010, 0x0000ffbd
    .int 0x00000010, 0x0000ffbe
    .int 0x00000010, 0x0000ffbf
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000009, 0x0000fb80
    .int 0x00000010, 0x0000ffc0
    .int 0x00000010, 0x0000ffc1
    .int 0x00000010, 0x0000ffc2
    .int 0x00000010, 0x0000ffc3
    .int 0x00000010, 0x0000ffc4
    .int 0x00000010, 0x0000ffc5
    .int 0x00000010, 0x0000ffc6
    .int 0x00000010, 0x0000ffc7
    .int 0x00000010, 0x0000ffc8
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000009, 0x0000fc00
    .int 0x00000010, 0x0000ffc9
    .int 0x00000010, 0x0000ffca
    .int 0x00000010, 0x0000ffcb
    .int 0x00000010, 0x0000ffcc
    .int 0x00000010, 0x0000ffcd
    .int 0x00000010, 0x0000ffce
    .int 0x00000010, 0x0000ffcf
    .int 0x00000010, 0x0000ffd0
    .int 0x00000010, 0x0000ffd1
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000009, 0x0000fc80
    .int 0x00000010, 0x0000ffd2
    .int 0x00000010, 0x0000ffd3
    .int 0x00000010, 0x0000ffd4
    .int 0x00000010, 0x0000ffd5
    .int 0x00000010, 0x0000ffd6
    .int 0x00000010, 0x0000ffd7
    .int 0x00000010, 0x0000ffd8
    .int 0x00000010, 0x0000ffd9
    .int 0x00000010, 0x0000ffda
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000009, 0x0000fd00
    .int 0x00000010, 0x0000ffdb
    .int 0x00000010, 0x0000ffdc
    .int 0x00000010, 0x0000ffdd
    .int 0x00000010, 0x0000ffde
    .int 0x00000010, 0x0000ffdf
    .int 0x00000010, 0x0000ffe0
    .int 0x00000010, 0x0000ffe1
    .int 0x00000010, 0x0000ffe2
    .int 0x00000010, 0x0000ffe3
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x0000000b, 0x0000ff20
    .int 0x00000010, 0x0000ffe4
    .int 0x00000010, 0x0000ffe5
    .int 0x00000010, 0x0000ffe6
    .int 0x00000010, 0x0000ffe7
    .int 0x00000010, 0x0000ffe8
    .int 0x00000010, 0x0000ffe9
    .int 0x00000010, 0x0000ffea
    .int 0x00000010, 0x0000ffeb
    .int 0x00000010, 0x0000ffec
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x0000000e, 0x0000ff80
    .int 0x00000010, 0x0000ffed
    .int 0x00000010, 0x0000ffee
    .int 0x00000010, 0x0000ffef
    .int 0x00000010, 0x0000fff0
    .int 0x00000010, 0x0000fff1
    .int 0x00000010, 0x0000fff2
    .int 0x00000010, 0x0000fff3
    .int 0x00000010, 0x0000fff4
    .int 0x00000010, 0x0000fff5
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x0000000a, 0x0000fe80
    .int 0x0000000f, 0x0000ff86
    .int 0x00000010, 0x0000fff6
    .int 0x00000010, 0x0000fff7
    .int 0x00000010, 0x0000fff8
    .int 0x00000010, 0x0000fff9
    .int 0x00000010, 0x0000fffa
    .int 0x00000010, 0x0000fffb
    .int 0x00000010, 0x0000fffc
    .int 0x00000010, 0x0000fffd
    .int 0x00000010, 0x0000fffe
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000
    .int 0x00000000, 0x00000000

    .code .text.L_BLK_enc_start
;~~~~~~~~~~~~~~~~~~~~~~
 L_BLK_enc_start:
;~~~~~~~~~~~~~~~~~~~~~~
 ; bru.jmp i16 ;return from function
 ; nop
 ; nop
 ; nop
 ; nop
 ; nop
;=====================================
;  _____   _____ _______ 
; |  __ \ / ____|__   __|
; | |  | | |       | |   
; | |  | | |       | |   
; | |__| | |____   | |   
; |_____/ \_____|  |_|   
;=====================================
; Inputs:
;  i0 = block addr
;  i2 = Horiz-stride (different for Y/chroma blocks)

;========================================================================================= 
; DCT start : load one block(64 signed bytes) and convert it to 64 FLOATs into V0..V15
;=========================================================================================
 ;load as byte, need to do level-shift on 8bit values
 lsu0.ld64.l v10, i0 || iau.add i0, i0, i2 || lsu1.ldil    i1,  128
 lsu0.ld64.h v10, i0 || iau.add i0, i0, i2 || cmu.cpivr.x8 v31, i1
 lsu0.ld64.l v11, i0 || iau.add i0, i0, i2 
 lsu0.ld64.h v11, i0 || iau.add i0, i0, i2 || cmu.cpis     s8, i31; SAVE i31
 lsu0.ld64.l v12, i0 || iau.add i0, i0, i2
 lsu0.ld64.h v12, i0 || iau.add i0, i0, i2
 lsu0.ld64.l v13, i0 || iau.add i0, i0, i2
 lsu0.ld64.h v13, i0 || iau.add i0, i0, i2
  
 vau.sub.i8 v10, v10, v31
 vau.sub.i8 v11, v11, v31

;convert i8->i16 
 cmu.cpvv.i8.i16 v24, v10
 cmu.cpvv.i8.i16 v25, v10 || lsu1.SWZC4WORD [1032] || vau.sub.i8 v12, v12, v31
 cmu.cpvv.i8.i16 v26, v11
 cmu.cpvv.i8.i16 v27, v11 || lsu1.SWZC4WORD [1032] || vau.sub.i8 v13, v13, v31
 cmu.cpvv.i8.i16 v28, v12
 cmu.cpvv.i8.i16 v29, v12 || lsu1.SWZC4WORD [1032]
 cmu.cpvv.i8.i16 v30, v13
 cmu.cpvv.i8.i16 v31, v13 || lsu1.SWZC4WORD [1032]
 
;convert i16->f32
 cmu.cpvv.i16.f32 v0,  v24
 cmu.cpvv.i16.f32 v4,  v24 || lsu1.SWZC4WORD [1032] ;nice convert (i.e. upper components...)
 cmu.cpvv.i16.f32 v1,  v25
 cmu.cpvv.i16.f32 v5,  v25 || lsu1.SWZC4WORD [1032]
 cmu.cpvv.i16.f32 v2,  v26 
 cmu.cpvv.i16.f32 v6,  v26 || lsu1.SWZC4WORD [1032]
 cmu.cpvv.i16.f32 v3,  v27 
 cmu.cpvv.i16.f32 v7,  v27 || lsu1.SWZC4WORD [1032]
 cmu.cpvv.i16.f32 v8,  v28 
 cmu.cpvv.i16.f32 v12, v28 || lsu1.SWZC4WORD [1032]
 cmu.cpvv.i16.f32 v9,  v29 
 cmu.cpvv.i16.f32 v13, v29 || lsu1.SWZC4WORD [1032]
 cmu.cpvv.i16.f32 v10, v30 
 cmu.cpvv.i16.f32 v14, v30 || lsu1.SWZC4WORD [1032]
 cmu.cpvv.i16.f32 v11, v31 
 cmu.cpvv.i16.f32 v15, v31 || lsu1.SWZC4WORD [1032]
 


;==================================================
;Arrange inputs in vectors for vector processing
;==================================================
 nop ;needed as cpvv.i16.f32 latency = 1 c.c.
 cmu.ROT4L V0,  V0
 cmu.ROT4L V4,  V4
 cmu.ROT4L V8,  V8
 cmu.ROT4L V12, V12

;#################################
;Start processing : ROW processing
;#################################

;~~~~~~~~~~~~~~~~~~~
; C reference "row_L1":
 vau.add.f32 v16, v3,  v4 ;tmpA_0
 vau.add.f32 v17, v2,  v5 ;tmpA_1
 vau.add.f32 v18, v1,  v6 ;tmpA_2
 vau.add.f32 v19, v0,  v7 ;tmpA_3
 vau.sub.f32 v20, v3,  v4 ;tmpA_7
 vau.sub.f32 v21, v2,  v5 ;tmpA_6
 vau.sub.f32 v22, v1,  v6 ;tmpA_5
 vau.sub.f32 v23, v0,  v7 ;tmpA_4
 
 vau.add.f32 v24, v11, v12 ;tmpB_0
 vau.add.f32 v25, v10, v13 ;tmpB_1
 vau.add.f32 v26, v9,  v14 ;tmpB_2
 vau.add.f32 v27, v8,  v15 ;tmpB_3
 vau.sub.f32 v28, v11, v12 ;tmpB_7
 vau.sub.f32 v29, v10, v13 ;tmpB_6
 vau.sub.f32 v30, v9,  v14 ;tmpB_5
 vau.sub.f32 v31, v8,  v15 ;tmpB_4

;~~~~~~~~~~~~~~~~~~~ 
; C reference "row_L2":
 vau.add.f32 V10, V16, V19 || lsu0.ldil i31, DCT_CT1
 vau.sub.f32 V16, V16, V19 || lsu0.ldih i31, DCT_CT1
 vau.add.f32 V19, V17, V18 || cmu.cpivr.x32 v8, i31
 vau.sub.f32 V17, V17, V18
 vau.add.f32 V11, V24, V27
 vau.sub.f32 V24, V24, V27
 vau.add.f32 V18, V16, V17
 vau.add.f32 V27, V25, V26
 vau.sub.f32 V25, V25, V26
 vau.mul.f32 V18, V18, V8
 vau.add.f32 V3,  V10, V19
 vau.add.f32 V26, V24, V25
 vau.sub.f32 V7,  V10, V19
 vau.add.f32 V1,  V16, V18
 vau.mul.f32 V26, V26, V8
 vau.sub.f32 V5,  V16, V18
 vau.add.f32 V11, V11, V27
 vau.sub.f32 V15, V11, V27
 vau.add.f32 V9,  V24, V26
 vau.sub.f32 V13, V24, V26

;~~~~~~~~~~~~~~~~~~~ 
; C reference "row_L3":
 vau.add.f32 V23, V23, V22 || lsu0.ldil i31, DCT_CT1 || lsu1.ldih i31, DCT_CT1
 vau.add.f32 V21, V21, V20 || cmu.cpivr.x32 V0, i31 ; V0 = CT_1[]
 vau.add.f32 V22, V22, V21 || lsu0.ldil i31, DCT_CT2 || lsu1.ldih i31, DCT_CT2
 vau.add.f32 V31, V31, V30 || cmu.cpivr.x32 V2, i31 ; V2 = CT_2[]
 vau.add.f32 V29, V29, V28 || lsu0.ldil i31, DCT_CT3 || lsu1.ldih i31, DCT_CT3
 vau.sub.f32 V16, V23, V21 || cmu.cpivr.x32 V4, i31 ; V4 = CT_3[]
 vau.add.f32 V30, V30, V29 || lsu0.ldil i31, DCT_CT4 || lsu1.ldih i31, DCT_CT4
 vau.sub.f32 V24, V31, V29 || cmu.cpivr.x32 V6, i31 ; V6 = CT_4[]
 
 vau.mul.f32 V16, V16, V2
 vau.mul.f32 V17, V23, V4
 vau.mul.f32 V24, V24, V2
 vau.mul.f32 V12, V21, V6
 vau.add.f32 V17, V17, V16
 vau.mul.f32 V25, V31, V4
 vau.mul.f32 V8,  V29, V6
 vau.add.f32 V16, V16, V12
 vau.add.f32 V25, V25, V24
 vau.mul.f32 V22, V22, V0
 vau.add.f32 V24, V24, V8
 vau.mul.f32 V30, V30, V0
 
 ; ;tmp2, tmp7, dataptr[5,3,1,7]
 vau.add.f32 V18, V20, V22
 vau.sub.f32 V20, V20, V22
 vau.add.f32 V26, V28, V30
 vau.sub.f32 V28, V28, V30
 vau.add.f32 V6,  V20, V17
 vau.sub.f32 V0,  V20, V17
 vau.add.f32 V2,  V18, V16
 vau.sub.f32 V4,  V18, V16
 vau.add.f32 V14, V28, V25
 vau.sub.f32 V8,  V28, V25
 vau.add.f32 V10, V26, V24
 vau.sub.f32 V12, V26, V24
 
;#######################
; COLUMN processing
;#######################
 ;prepare data:
 cmu.ROT4R V0,  V0
 cmu.ROT4R V4,  V4
 cmu.ROT4R V8,  V8
 cmu.ROT4R V12, V12

 ;compute tmp0,1,2,3,.. 7
 vau.add.f32 V16, V3, V8
 vau.add.f32 V17, V2, V9
 vau.add.f32 V18, V1, V10
 vau.add.f32 V19, V0, V11
 vau.sub.f32 V20, V3, V8
 vau.sub.f32 V21, V2, V9
 vau.sub.f32 V22, V1, V10
 vau.sub.f32 V23, V0, V11

 vau.add.f32 V24, V7, V12
 vau.add.f32 V25, V6, V13
 vau.add.f32 V26, V5, V14
 vau.add.f32 V27, V4, V15
 vau.sub.f32 V28, V7, V12
 vau.sub.f32 V29, V6, V13
 vau.sub.f32 V30, V5, V14
 vau.sub.f32 V31, V4, V15

 ;~~~~~~~~~~~~~~~~~~~ 
; C reference "col_L2":
 vau.add.f32 V10, V16, V19 || lsu0.ldil i31, DCT_CT1
 vau.sub.f32 V16, V16, V19 || lsu0.ldih i31, DCT_CT1
 vau.add.f32 V19, V17, V18 || cmu.cpivr.x32 v8, i31
 vau.sub.f32 V17, V17, V18
 vau.add.f32 V11, V24, V27
 vau.sub.f32 V24, V24, V27
 vau.add.f32 V18, V16, V17
 vau.add.f32 V27, V25, V26
 vau.sub.f32 V25, V25, V26
 vau.mul.f32 V18, V18, V8
 vau.add.f32 V3,  V10, V19
 vau.add.f32 V26, V24, V25
 vau.sub.f32 V7,  V10, V19
 vau.add.f32 V1,  V16, V18
 vau.mul.f32 V26, V26, V8
 vau.sub.f32 V5,  V16, V18
 vau.add.f32 V11, V11, V27
 vau.sub.f32 V15, V11, V27
 vau.add.f32 V9,  V24, V26
 vau.sub.f32 V13, V24, V26

;~~~~~~~~~~~~~~~~~~~ 
; C reference "col_L3": & laod Quantization table
 vau.add.f32 V23, V23, V22 || lsu0.ldil i31, DCT_CT1 || lsu1.ldih i31, DCT_CT1
 vau.add.f32 V21, V21, V20 || cmu.cpivr.x32 V0, i31 ; V0 = CT_1[]
 vau.add.f32 V22, V22, V21 || lsu0.ldil i31, DCT_CT2 || lsu1.ldih i31, DCT_CT2
 vau.add.f32 V31, V31, V30 || cmu.cpivr.x32 V2, i31 ; V2 = CT_2[]
 vau.add.f32 V29, V29, V28 || lsu0.ldil i31, DCT_CT3 || lsu1.ldih i31, DCT_CT3
 vau.sub.f32 V16, V23, V21 || cmu.cpivr.x32 V4, i31 ; V4 = CT_3[]
 vau.add.f32 V30, V30, V29 || lsu0.ldil i31, DCT_CT4 || lsu1.ldih i31, DCT_CT4
 vau.sub.f32 V24, V31, V29 || cmu.cpivr.x32 V6, i31 ; V6 = CT_4[]
 
 vau.mul.f32 V16, V16, V2
 vau.mul.f32 V17, V23, V4
 vau.mul.f32 V24, V24, V2  || lsu0.ldi64.l v31,  i23 ;V31 = quant_v0
 vau.mul.f32 V12, V21, V6  || lsu0.ldi64.h v31,  i23
 vau.add.f32 V17, V17, V16 || lsu0.ldi64.l v29,  i23 ;V29 = quant_v1
 vau.mul.f32 V25, V31, V4  || lsu0.ldi64.h v29,  i23
 vau.mul.f32 V8,  V29, V6  || lsu0.ldi64.l v19,  i23 ;V19  =quant_v2
 vau.add.f32 V16, V16, V12 || lsu0.ldi64.h v19,  i23
 vau.add.f32 V25, V25, V24 || lsu0.ldi64.l v21,  i23 ;V21 =quant_v3
 vau.mul.f32 V22, V22, V0  || lsu0.ldi64.h v21,  i23
 vau.add.f32 V24, V24, V8  || lsu0.ldi64.l v22,  i23 ;V22 =quant_v4
 vau.mul.f32 V30, V30, V0  || lsu0.ldi64.h v22,  i23
 
 ; ;tmp2, tmp7, dataptr[5,3,1,7]
 vau.add.f32 V18, V20, V22 || lsu0.ldi64.l v30,  i23 ;V30 =quant_v5
 vau.sub.f32 V20, V20, V22 || lsu0.ldi64.h v30,  i23
 vau.add.f32 V26, V28, V30 || lsu0.ldi64.l v17,  i23 ;V17 =quant_v6
 vau.sub.f32 V28, V28, V30 || lsu0.ldi64.h v17,  i23
 vau.add.f32 V6,  V20, V17 || lsu0.ldi64.l v16,  i23 ;V16 =quant_v7
 vau.sub.f32 V0,  V20, V17 || lsu0.ldi64.h v16,  i23
 vau.add.f32 V2,  V18, V16 || lsu0.ldi64.l v25,  i23 ;V25 =quant_v8
 vau.sub.f32 V4,  V18, V16 || lsu0.ldi64.h v25,  i23
 vau.add.f32 V14, V28, V25 || lsu0.ldi64.l v24,  i23 ;V24 =quant_v9
 vau.sub.f32 V8,  V28, V25 || lsu0.ldi64.h v24,  i23
 vau.add.f32 V10, V26, V24 || lsu0.ldi64.l v26,  i23 ;V26 =quant_v10
 vau.sub.f32 V12, V26, V24 || lsu0.ldi64.h v26,  i23
 

 
 ;========================================================================================= 
 ;========================================================================================= 
 ;  Quantization, fp32-i16 conversion (quantization table was loaded ahead...)
 ;========================================================================================= 
 ;========================================================================================= 
 vau.mul.f32 V3,   V3, V31 || lsu0.ldi64.l v31,  i23; X quant_v0, load quant_v11_low
 vau.mul.f32 V11, V11, V29 || lsu0.ldi64.h v31,  i23; X quant_v1, load quant_v11_high
 vau.mul.f32 V2,   V2, V19 || lsu0.ldi64.l v29,  i23; X quant_v2, load quant_v12_low
 vau.mul.f32 V10, V10, V21 || lsu0.ldi64.h v29,  i23; X quant_v3, load quant_V12_high
 vau.mul.f32 V1,   V1, V22 || lsu0.ldi64.l v19,  i23; X quant_v4, load quant_V13_low
 vau.mul.f32 V9,   V9, V30 || lsu0.ldi64.h v19,  i23; X quant_v5, load quant_V13 high
 vau.mul.f32 V0,   V0, V17 || lsu0.ldi64.l v21,  i23 || cmu.cpvv.f32.i16s V17, V11; X quant_v6, load quant_V14_low
 vau.mul.f32 V8,   V8, V16 || lsu0.ldi64.h v21,  i23 || cmu.cpvv.f32.i16s V16, V3 ; X quant_v7, load quant_V14_high
 
 vau.mul.f32 V7,   V7, V25 || lsu0.ldi64.l v22,  i23 || cmu.cpvv.f32.i16s V18, V2 ; X quant_v8, load quant_V15_low
 vau.mul.f32 V15, V15, V24 || lsu0.ldi64.h v22,  i23 || cmu.cpvv.f32.i16s V20, V1; X quant_v9, load quant_V15_high
 vau.mul.f32 V6,   V6, V26 || cmu.cpvv.f32.i16s V23, V8 ; X quant_V10
 vau.mul.f32 V14, V14, V31 || cmu.cpvv.f32.i16s V24, V7 ; X quant_V11
 vau.mul.f32 V5,   V5, V29 || cmu.cpvv.f32.i16s V25, V15; X quant_V12
 vau.mul.f32 V13, V13, V19 || cmu.cpvv.f32.i16s V19, V10; X quant_V13
 vau.mul.f32 V4,   V4, V21 || cmu.cpvv.f32.i16s V21, V9 ; X quant_V14
 vau.mul.f32 V12, V12, V22 || cmu.cpvv.f32.i16s V22, V0 ; X quant_V15
 
 cmu.cpvv.f32.i16s V26, V6
 cmu.cpvv.f32.i16s V27, V14
 cmu.cpvv.f32.i16s V28, V5
 cmu.cpvv.f32.i16s V29, V13
 cmu.cpvv.f32.i16s V30, V4
 cmu.cpvv.f32.i16s V31, V12

 ;=================================================================== 
 ; _______       ______                     _____  _      ______ 
 ;|___  (_)     |___  /               _    |  __ \| |    |  ____|
 ;   / / _  __ _   / / __ _  __ _   _| |_  | |__) | |    | |__   
 ;  / / | |/ _` | / / / _` |/ _` | |_   _| |  _  /| |    |  __|  
 ; / /__| | (_| |/ /_| (_| | (_| |   |_|   | | \ \| |____| |____ 
 ;/_____|_|\__, /_____\__,_|\__, |         |_|  \_\______|______|
 ;          __/ |            __/ |                               
 ;         |___/            |___/                                
 ;===================================================================
 
 ; Zig-Zag  + Run-Length encoding
 ; 0,  1,   8, 16,  9,  2, 3,  10,
 ; 17, 24, 32, 25, 18, 11, 4,   5,
 ; 12, 19, 26, 33, 40, 48, 41, 34,
 ; 27, 20, 13,  6,  7, 14, 21, 28,
 ; 35, 42, 49, 56, 57, 50, 43, 36,
 ; 29, 22, 15, 23, 30, 37, 44, 51,
 ; 58, 59, 52, 45, 38, 31, 39, 46,
 ; 53, 60, 61, 54, 47, 55, 62, 63
 ;=========================================== 
 ;   Temporary reg usage:
 ;    i0 = {Run[15:0], Coef[15:0 } pair
 ;    i1 = VRF store addr (LUT) (=tot number of pairs at the end)
 ;    i2 = 0x0000_FFFF
 ;    i3 = 0x0001_0000
 ;    i4 = dummy (to get flag results)
 ;=========================================== 
 
 ;assume no zeroes initially (this setup can be moved above, in Quant steps....)
 lsu0.ldil i0, 0x0000 || lsu1.ldil i2,  0xFFFF
 lsu0.ldih i3, 0x0001 || lsu1.ldil i3,  0x0000
 lsu0.ldil i1, 0x0000 || lsu1.ldil i19, 0
 lsu1.ldil i7, 0
 
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ; pos_coefs[0:7] ={ 0,  1,   8, 16,  9,  2, 3,  10}
 ; skip DC.. start with second element:
   
  cmu.cpvi.x16 i0.l, v16.1 ;data[1]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 1
  cmu.cpvi.x16 i0.l, v18.0 ;data[8]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 2
  cmu.cpvi.x16 i0.l, v20.0 ;data[16]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 3
  cmu.cpvi.x16 i0.l, v18.1 ;data[9]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 4
  cmu.cpvi.x16 i0.l, v16.2 ;data[2]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 5
  cmu.cpvi.x16 i0.l, v16.3 ;data[3]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 6
  cmu.cpvi.x16 i0.l, v18.2 ;data[10]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 7
  
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ; pos_coefs[8:15] = {17, 24, 32, 25, 18, 11, 4,   5,}
  cmu.cpvi.x16 i0.l, v20.1 ;data[17]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 8
  cmu.cpvi.x16 i0.l, v22.0 ;data[24]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 9
  cmu.cpvi.x16 i0.l, v24.0 ;data[32]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 10
  cmu.cpvi.x16 i0.l, v22.1 ;data[25]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 11
  cmu.cpvi.x16 i0.l, v20.2 ;data[18]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 12
  cmu.cpvi.x16 i0.l, v18.3 ;data[11]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 13
  cmu.cpvi.x16 i0.l, v17.0 ;data[4]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 14
  cmu.cpvi.x16 i0.l, v17.1 ;data[5]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 15
 
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ; pos_coefs[16:23] = {12, 19, 26, 33, 40, 48, 41, 34}
  cmu.cpvi.x16 i0.l, v19.0 ;data[12]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 16
  cmu.cpvi.x16 i0.l, v20.3 ;data[19]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 17
  cmu.cpvi.x16 i0.l, v22.2 ;data[26]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 18
  cmu.cpvi.x16 i0.l, v24.1 ;data[33]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 19
  cmu.cpvi.x16 i0.l, v26.0 ;data[40]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 20
  cmu.cpvi.x16 i0.l, v28.0 ;data[48]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 21
  cmu.cpvi.x16 i0.l, v26.1 ;data[41]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 22
  cmu.cpvi.x16 i0.l, v24.2 ;data[34]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 23
 
  ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ;pos_coefs[24:31] = {27, 20, 13,  6,  7, 14, 21, 28}
  cmu.cpvi.x16 i0.l, v22.3 ;data[27]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 24
  cmu.cpvi.x16 i0.l, v21.0 ;data[20]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 25
  cmu.cpvi.x16 i0.l, v19.1 ;data[13]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 26
  cmu.cpvi.x16 i0.l, v17.2 ;data[6]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 27
  cmu.cpvi.x16 i0.l, v17.3 ;data[7]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 28
  cmu.cpvi.x16 i0.l, v19.2 ;data[14]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 29
  cmu.cpvi.x16 i0.l, v21.1 ;data[21]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 30
  cmu.cpvi.x16 i0.l, v23.0 ;data[28]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 31
  
  ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ;pos_coefs[32:39] = {35, 42, 49, 56, 57, 50, 43, 36}
  cmu.cpvi.x16 i0.l, v24.3 ;data[35]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 32
  cmu.cpvi.x16 i0.l, v26.2 ;data[42]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 33
  cmu.cpvi.x16 i0.l, v28.1 ;data[49]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 34
  cmu.cpvi.x16 i0.l, v30.0 ;data[56]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 35
  cmu.cpvi.x16 i0.l, v30.1 ;data[57]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 36
  cmu.cpvi.x16 i0.l, v28.2 ;data[50]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 37
  cmu.cpvi.x16 i0.l, v26.3 ;data[43]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 38
  cmu.cpvi.x16 i0.l, v25.0 ;data[36]
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 39
   
 ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ;pos_coefs[40:47] = { 29, 22, 15, 23, 30, 37, 44, 51}
  cmu.cpvi.x16 i0.l, v23.1 ;data 29
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 40
  cmu.cpvi.x16 i0.l, v21.2 ;data 22
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 41
  cmu.cpvi.x16 i0.l, v19.3 ;data 15
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 42
  cmu.cpvi.x16 i0.l, v21.3 ;data 23
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 43
  cmu.cpvi.x16 i0.l, v23.2 ;data 30
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 44
  cmu.cpvi.x16 i0.l, v25.1 ;data 37
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 45 
  cmu.cpvi.x16 i0.l, v27.0 ;data 44
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 46
  cmu.cpvi.x16 i0.l, v28.3 ;data 51
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 47
   
  ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ;pos_coefs[48:55] = {58, 59, 52, 45, 38, 31, 39, 46}
  cmu.cpvi.x16 i0.l, v30.2 ;data 58
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 48
  cmu.cpvi.x16 i0.l, v30.3 ;data 59
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 49
  cmu.cpvi.x16 i0.l, v29.0 ;data 52
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 50
  cmu.cpvi.x16 i0.l, v27.1 ;data 45
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 51
  cmu.cpvi.x16 i0.l, v25.2 ;data 38
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 52
  cmu.cpvi.x16 i0.l, v23.3 ;data 31
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 53
  cmu.cpvi.x16 i0.l, v25.3 ;data 39
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 54
  cmu.cpvi.x16 i0.l, v27.2 ;data 46
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 55

  ;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ;pos_coefs[56:63] = {; 53, 60, 61, 54, 47, 55, 62, 63}
  cmu.cpvi.x16 i0.l, v29.1 ;data 53
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 56
  cmu.cpvi.x16 i0.l, v31.0 ;data 60
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 57
  cmu.cpvi.x16 i0.l, v31.1 ;data 61
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 58
  cmu.cpvi.x16 i0.l, v29.2 ;data 54
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 59
  cmu.cpvi.x16 i0.l, v27.3 ;data 47
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 60
  cmu.cpvi.x16 i0.l, v29.3 ;data 55
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 61
  cmu.cpvi.x16 i0.l, v31.2 ;data 62
   sau.and i4, i0, i2
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 62
  cmu.cpvi.x16 i0.l, v31.3 ;data 63
   sau.and i4, i0, i2 || cmu.cpsi   i31, s8   ;restore i31(img width)
   peu.pc1s EQ  || iau.add  i0, i0, i3
   peu.pc1s NEQ || iau.incs i1, 1 || lsu0.ldil i0,0 || cmu.lutw32 i0, i1, 0 || lsu1.ldil i19, 63
   
 ;After last coef was encoded, write a ZERO{Zrun/Coef} to mark the end of AC-pairs array
   cmu.lutw32 i7,  i1, 0
   
  ;======================================================================================
;  _    _            __    __            _____   _____ 
; | |  | |          / _|  / _|          |  __ \ / ____|
; | |__| |  _   _  | |_  | |_   ______  | |  | | |     
; |  __  | | | | | |  _| |  _| |______| | |  | | |     
; | |  | | | |_| | | |   | |            | |__| | |____ 
; |_|  |_|  \__,_| |_|   |_|            |_____/ \_____|
; Huffman DC encode
;======================================================================================
;  v16.0 = DC value
;  i2  = (TMP) temp (abs Diff; load addr for Huff tbl; 16, 0, Shift amount for amplitude
;  i4  = (TMP) current DC; diff.i16; diff.i32; diff.actualizat dupa testul <0?
;  i5  = (TMP) amplitude CATEGORY
;  i6  = (TMP) Huff-code_len[bits], load addr...
;  i7  = (TMP) Huff-code (left aligned at 16bits boundary)
;  i8  = (TMP) Insert pos; 0x00FF_0000(mask for second byte)
;  i9  = (TMP) LSByte_1
;  i10 = (TMP) LSByte_2
;  i11 = (TMP) 0xFF
;  i12 = (TMP) next addresses starting from i26+1
;  i13 = (TMP) next addresses starting from i26+2
;  i12 = (TMP) 32767 (for a while) 
;  i14 = (TMP) mask-for amplitude
;  i15 = (TMP) jump address


 cmu.cpvi i4, v16.0   || lsu0.ldil i11, 0x00FF || lsu1.ldil i3, 0
 iau.sub  i4, i4, i29 || cmu.cpii i29, i4 ;Compute DIFF, save current diff as "prev DC" for future

;Get amplitude Category:
 cmu.cpii.i16.i32 i4, i4 || lsu0.ldil i13, 0xFFFF
 iau.abs          i2, i4 || lsu0.ldih i13, 0xFFFF || cmu.cpii i0, i4
 iau.bsr          i5, i2 || lsu0.ldil i12, 32767
 iau.incs         i5, 1

;================================================================= 
;Load Huff-pair {code_len, code}
;=================================================================
 iau.shl   i6, i5, 3        || sau.shl.u32 i13, i13, i5
 iau.add   i2, i25, i6 
 lsu0.ld64 i6, i2           || cmu.cmz.i32 i4  || iau.not i14, i13
  peu.pc1c LT               || iau.add i4, i4, i12
  sau.add.i32 i12, i26, 0x1 || iau.and i4, i4, i14
  sau.add.i32 i13, i26, 0x2 || lsu0.ldil i2, 16 
  sau.sub.i32 i8,  i2, i21  || iau.sub   i2, i2, i5
  iau.shl     i4,  i4, i2

  
;=================================================================
;Category-Huffman-code in bitbuff, store, manage stuff bytes (0x00)
;=================================================================
 iau.shl       i7,  i7,  i8 || sau.add.i32 i21, i21, i6 || lsu0.ldil i15, L_huff_DC_done || lsu1.ldil i2, 16
 iau.or        i22, i22, i7 || lsu1.ldih i8, 0x00FF     || lsu0.ldil i8, 0x0000
 iau.shr.u32   i9,  i22, 24 || sau.and i10, i22, i8     || cmu.cmii.u32 i21, i2  || lsu0.ldih i15, L_huff_DC_done
 
 ;if Diff==0, just encode categ-bits, but not amplitude
 sau.or i0, i0, i0  || lsu0.ldil i2, 0;testul trebuie pus pe Diff, nu pe vaoloarea actualizata dupa "bitcode"
 peu.pc1s EQ        || bru.jmp i15
 
  peu.pc1c      GTE      || lsu0.st8 i9,  i26     || iau.xor     i9,  i9,  i11 || cmu.cpii    i26, i12 || sau.shr.u32 i10, i10, 16
  peu.pcc0i.and GTE, EQ  || lsu0.st8 i2,  i26     || cmu.cpii    i26, i13      || sau.add.i32 i13, i13, 1
  peu.pc1c      GTE      || lsu0.st8 i10, i26     || iau.xor     i10, i10, i11 || sau.add.i32 i26, i26, 1
  peu.pcc0i.and GTE, EQ  || lsu0.st8 i2,  i13     || sau.add.i32 i26, i13, 1
  peu.pc1c      GTE      || iau.sub  i21, i21, 16 || sau.shl.u32 i22, i22, 16
  

;=================================================================
;Encode also amplitude (if Category != 0)
;=================================================================


                                 sau.add.i32 i12, i26, 0x1 || lsu0.ldil i2, 16
  iau.sub       i8,  i2,  i21 || sau.add.i32 i13, i26, 0x2
  iau.shl       i4,  i4,  i8  || sau.add.i32 i21, i21, i5
  iau.or        i22, i22, i4  || lsu1.ldih   i8,  0x00FF  || lsu0.ldil i8, 0x0000
  iau.shr.u32   i9,  i22, 24  || sau.and     i10, i22, i8 || cmu.cmii.i32 i21, i2 || lsu0.ldil i2, 0
 
  peu.pc1c      GTE      || lsu0.st8 i9,  i26     || iau.xor     i9,  i9,  i11 || cmu.cpii    i26, i12 || sau.shr.u32 i10, i10, 16
  peu.pcc0i.and GTE, EQ  || lsu0.st8 i2,  i26     || cmu.cpii    i26, i13      || sau.add.i32 i13, i13, 1
  peu.pc1c      GTE      || lsu0.st8 i10, i26     || iau.xor     i10, i10, i11 || sau.add.i32 i26, i26, 1
  peu.pcc0i.and GTE, EQ  || lsu0.st8 i2,  i13     || sau.add.i32 i26, i13, 1
  peu.pc1c      GTE      || iau.sub  i21, i21, 16 || sau.shl.u32 i22, i22, 16
  
;~~~~~~~~~~~~~~~~ 
L_huff_DC_done:
;~~~~~~~~~~~~~~~~

;======================================================================================
;  _    _            __    __                     _____ 
; | |  | |          / _|  / _|              /\   / ____|
; | |__| |  _   _  | |_  | |_   ______     /  \ | |     
; |  __  | | | | | |  _| |  _| |______|   / /\ \| |     
; | |  | | | |_| | | |   | |             / ____ \ |____ 
; |_|  |_|  \__,_| |_|   |_|            /_/    \_\_____|
;======================================================================================
;  i1    = number of AC coefs (zeroRun/Coef pairs)
;  v0... = the {zeroRun/Coef} pairs, each set on 32bit: {16bit, 16bit}
;  i3    = VRF-lut address (set to zero at init) 


;~~~~~~~~~~~~~~~~~~~~
L_huff_AC_next:
;~~~~~~~~~~~~~~~~~~~~
 cmu.lut32.u32 i4, i3,0

;~~~~~~~~~~~~~~~~~~~~
; so that when treating 16Zrun, we don't reload the old Zrun/coef pair
L_huff_AC_next_2: 
;~~~~~~~~~~~~~~~~~~~~

 iau.or i1, i1, i1      || lsu0.ldil i15, L_huff_EOB     || lsu1.ldih i15, L_huff_EOB 
 peu.pc1i EQ            || bru.jmp     i15
 iau.shr.u32 i2, i4, 20 || cmu.cmz.i32 i1         || lsu0.ldil i15, L_huff_AC_coef || lsu1.ldih i15, L_huff_AC_coef
 peu.pcc0i.and NEQ, EQ  || bru.jmp     i15
 
 ;~~~~~~~~~~~~~~~~~~~~
 L_huff_AC_16Zeroes:
 ;~~~~~~~~~~~~~~~~~~~~
  sau.add.i32 i12, i26, 0x1 || cmu.cpsi  i6, s26    || lsu0.ldil    i2, 16
  sau.add.i32 i13, i26, 0x2 || cmu.cpsi  i7, s25    || iau.sub      i8, i2, i21 || lsu0.ldil    i15, L_huff_AC_next_2
  sau.add.i32 i5,  i21, i6  || iau.shl   i7, i7, i8 || lsu0.ldih    i15, L_huff_AC_next_2     || lsu1.ldil i6, 0
  iau.or      i14, i22, i7  || lsu0.ldil i8, 0x0000 || lsu1.ldih    i8, 0x00FF
  iau.shr.u32 i9,  i14, 24  || sau.and i10, i14, i8 || cmu.cmii.i32 i5, i2 || lsu0.ldil i2, 0 || lsu1.ldih i6, 0x0010
  
  peu.pc1c      GTE     || iau.xor i9, i9,  i11     || sau.shr.u32 i10,i10,16 || cmu.cpii i26,i12 || lsu0.st8 i9, i26
  bru.jmp i15           || sau.sub.i32 i4,  i4,  i6 || lsu1.ldil i8, 0x0000
    
  peu.pcc0i.and GTE, EQ || sau.add.i32 i13, i13, 1  || cmu.cpii    i26, i13    || lsu0.st8 i2, i26
  peu.pc1c      GTE     || iau.xor     i10, i10, i11|| sau.add.i32 i26, i26, 1 || lsu0.st8 i10, i26
  peu.pcc0i.and GTE, EQ || sau.add.i32 i26, i13, 1  || lsu0.st8    i2,  i13
  iau.add i21, i5, i8   || cmu.cpii    i22, i14
  peu.pc1c      GTE     || iau.sub     i21, i21, 16 || sau.shl.u32 i22, i22, 16
 
 ;===========================================================================================
 L_huff_AC_coef:
 ;===========================================================================================
 
 ;Get Amplitude-Gategory of current AC coef:
  sau.shr.u32 i8, i4, 16 || cmu.cpii.i16.i32 i5, i4 || lsu0.ldil i13, 0xFFFF
  iau.abs i2, i5         || lsu0.ldih i13, 0xFFFF
  iau.bsr i5, i2         || sau.shl.u32 i8, i8, 7 || lsu0.ldil i12, 32767
  iau.incs i5, 1
 
 ;================================================================
 ; Load Huff-entry from table:
 ;=============================
  iau.shl   i6, i5,  3  || sau.shl.u32 i13,i13,i5
  iau.add   i2, i24, i8 || cmu.cpii.i16.i32 i4, i4
  iau.add   i2, i2,  i6
  iau.not   i14,i13     || cmu.cmz.i32 i4 || lsu0.ld64, i6, i2
   peu.pc1c LT          || iau.add i4, i4, i12
  iau.and   i4, i4, i14 || sau.add.i32 i12, i26, 0x1
  lsu0.ldil i2, 16      || sau.add.i32 i13, i26, 0x2
  iau.sub   i2, i2, i5  || sau.sub.i32 i8, i2, i21
  iau.shl   i4, i4, i2  

 ;================================================================
 ; Encode Categ bits:
 ;========================
  iau.shl     i7,  i7,  i8 || sau.add.i32 i21, i21, i6 || lsu1.ldil i2, 16
  iau.or      i22, i22, i7 || lsu1.ldih i8, 0x00FF     || lsu0.ldil i8, 0x0000
  iau.shr.u32 i9,  i22, 24 || sau.and i10, i22, i8     || cmu.cmii.u32 i21, i2 || lsu0.ldil i2, 0
  
  peu.pc1c      GTE        || iau.xor i9, i9, i11  || sau.shr.u32 i10, i10, 16 || cmu.cpii i26, i12 || lsu0.st8 i9, i26
  peu.pcc0i.and GTE, EQ    ||                         sau.add.i32 i13, i13, 1  || cmu.cpii i26, i13 || lsu0.st8 i2, i26 
  peu.pc1c      GTE        || iau.xor i10,i10,i11  || sau.add.i32 i26, i26, 1                       || lsu0.st8 i10,i26
  peu.pcc0i.and GTE, EQ                            || sau.add.i32 i26, i13, 1                       || lsu0.st8 i2, i13
  peu.pc1c      GTE        || iau.sub i21, i21, 16 || sau.shl.u32 i22, i22, 16 
  
 ;================================================================
 ; Encode Amplitude bits:
 ;========================
  sau.add.i32 i12, i26, 0x1 || lsu0.ldil i2, 16      || iau.incs i3, 1 ;goto next addr in VRF-LUT
  sau.add.i32 i13, i26, 0x2 || iau.sub i8,  i2,  i21 || lsu1.ldil i15, L_huff_AC_next
  sau.add.i32 i21, i21, i5  || iau.shl i4,  i4,  i8  || lsu1.ldih i15, L_huff_AC_next
                               iau.or  i22, i22, i4  || lsu0.ldil i8, 0x0000 || lsu1.ldih i8, 0x00FF
  iau.shr.u32 i9, i22, 24   || sau.and i10, i22, i8  || cmu.cmii.i32 i21, i2 || lsu0.ldil i2, 0
  
  bru.jmp i15 || iau.incs i1, -1 ;for now, don't loop...
  
  peu.pc1c      GTE     || iau.xor i9, i9, i11 || sau.shr.u32 i10, i10, 16 || cmu.cpii i26, i12 || lsu0.st8 i9,  i26
  peu.pcc0i.and GTE, EQ ||                        sau.add.i32 i13, i13, 1  || cmu.cpii i26, i13 || lsu0.st8 i2,  i26
  peu.pc1c      GTE     || iau.xor i10,i10,i11 || sau.add.i32 i26, i26, 1                       || lsu0.st8 i10, i26
  peu.pcc0i.and GTE, EQ ||                        sau.add.i32 i26, i13, 1                       || lsu0.st8 i2,  i13
  peu.pc1c      GTE     || iau.sub i21,i21,16  || sau.shl.u32 i22, i22, 16
   
   
 ;~~~~~~~~~~~~~~~~~~~~
 L_huff_EOB:
 ;~~~~~~~~~~~~~~~~~~~~
  lsu1.ldil i15, L_huff_fin
  iau.sub i19,  i19, 63 || sau.add.i32 i12, i26, 0x1 || cmu.cpsi i6, s28 || lsu0.ldil i2, 16 || lsu1.ldih i15, L_huff_fin
  peu.pc1i EQ           || bru.jmp     i15
   iau.sub     i8,  i2,  i21 || sau.add.i32 i13, i26, 0x2 || cmu.cpsi i7, s27
   iau.shl     i7,  i7,  i8  || sau.add.i32 i5,  i21, i6
   iau.or      i14, i22, i7  || lsu0.ldil i8, 0x0000      || lsu1.ldih i8, 0x00FF
   iau.shr.u32 i9,  i14, 24  || sau.and i10, i14, i8
   cmu.cmii.i32 i5, i2       || lsu0.ldil i2, 0
  
  peu.pc1c      GTE     || iau.xor     i9,  i9,  i11 || sau.shr.u32 i10, i10, 16 || cmu.cpii i26, i12 || lsu0.st8 i9, i26
    bru.jmp i15         || lsu0.ldil   i8,  0x0000
  peu.pcc0i.and GTE, EQ || sau.add.i32 i13, i13, 1   || cmu.cpii i26, i13        || lsu0.st8 i2,  i26
  peu.pc1c      GTE     || sau.add.i32 i26, i26, 1   || iau.xor  i10, i10, i11   || lsu0.st8 i10, i26
  peu.pcc0i.and GTE, EQ || sau.add.i32 i26, i13, 1   || lsu0.st8 i2, i13
    iau.add i21, i5, i8 || cmu.cpii    i22, i14
  peu.pc1c      GTE     || iau.sub     i21, i21, 16  || sau.shl.u32 i22, i22, 16
   
   
 ;~~~~~~~~~~~~~~~~~~~~
 L_huff_fin:
 ;~~~~~~~~~~~~~~~~~~~~
  
 ;temporary end point, will return from call actually....
 
;======================= 
; Finish:
;======================= 
; bru.swih 0x1f
 ; nop
 ; nop
 ; nop
 ; nop
 ; nop
  
 bru.jmp i16 ;return from function
 nop
 nop
 nop
 nop
 nop
 
.end