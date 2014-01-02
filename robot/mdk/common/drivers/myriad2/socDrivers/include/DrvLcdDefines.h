#ifndef _BRINGUP_SABRE_LCD_DEF_H_
#define _BRINGUP_SABRE_LCD_DEF_H_

/* ******************************************************************************************
                                   LCD controller control register defines
******************************************************************************************  */
// --- bit 0
#define D_LCD_CTRL_PROGRESSIVE (0x00)         // default
#define D_LCD_CTRL_INTERLACED  (0x01)
// --- bit 1
#define D_LCD_CTRL_ENABLE      (0x02)         // enable conrtoller
// --- bits 2,3,4,5
#define D_LCD_CTRL_VL1_ENABLE  (0x04)         // enable video layer 1
#define D_LCD_CTRL_VL2_ENABLE  (0x08)         // enable  video layer 2
#define D_LCD_CTRL_GL1_ENABLE  (0x10)         // enable  graphics layer 1
#define D_LCD_CTRL_GL2_ENABLE  (0x20)         // enable  graphics layer 2
// --- bits 6:7
#define D_LCD_CTRL_ALPHA_BLEND_VL1 (0x00)     // video layer 1 - default
#define D_LCD_CTRL_ALPHA_BLEND_VL2 (0x40)     // video layer 2
#define D_LCD_CTRL_ALPHA_BLEND_GL1 (0x80)     // graphics layer 1
#define D_LCD_CTRL_ALPHA_BLEND_GL2 (0xC0)     // graphics layer 2
// --- bits 8:9
#define D_LCD_CTRL_ALPHA_TOP_VL1 (0x000)      // video layer 1 - default
#define D_LCD_CTRL_ALPHA_TOP_VL2 (0x100)      // video layer 2
#define D_LCD_CTRL_ALPHA_TOP_GL1 (0x200)      // graphics layer 1
#define D_LCD_CTRL_ALPHA_TOP_GL2 (0x300)      // graphics layer 2
// --- bits 10:11
#define D_LCD_CTRL_ALPHA_MIDDLE_VL1 (0x000)   // video layer 1 - default
#define D_LCD_CTRL_ALPHA_MIDDLE_VL2 (0x400)   // video layer 2
#define D_LCD_CTRL_ALPHA_MIDDLE_GL1 (0x800)   // graphics layer 1
#define D_LCD_CTRL_ALPHA_MIDDLE_GL2 (0xC00)   // graphics layer 2
// --- bits 12:13
#define D_LCD_CTRL_ALPHA_BOTTOM_VL1 (0x0000)  // video layer 1 - default
#define D_LCD_CTRL_ALPHA_BOTTOM_VL2 (0x1000)  // video layer 2
#define D_LCD_CTRL_ALPHA_BOTTOM_GL1 (0x2000)  // graphics layer 1
#define D_LCD_CTRL_ALPHA_BOTTOM_GL2 (0x3000)  // graphics layer 2
// --- bit 14
#define D_LCD_CTRL_TIM_GEN_ENABLE   (0x4000)  // enable timing generator
// --- bit 15
#define D_LCD_CTRL_DISPLAY_MODE_ONE_SHOT (0x8000) // enable one shot mode, default is continous
// --- bits 16, 17, 18
#define D_LCD_CTRL_PWM0_EN (0x10000)          // enable PWM 0
#define D_LCD_CTRL_PWM1_EN (0x20000)          // enable PWM 1
#define D_LCD_CTRL_PWM2_EN (0x40000)          // enable PWM 2
// --- bits 19:20
#define D_LCD_CTRL_OUTPUT_DISABLED     (0x000000)  // output disabled
#define D_LCD_CTRL_OUTPUT_RGB          (0x080000)  // output using RGB interface
#define D_LCD_CTRL_OUTPUT_INTEL8080    (0x100000)  // output using Intel 8080 interface
#define D_LCD_CTRL_OUTPUT_MOTOROLA6800 (0x180000)  //output using Motorola 68000 interface
// --- bit 21
#define D_LCD_CTRL_SHARP_TFT           (0x200000)



/* ******************************************************************************************
                                   LCD controller output format register defines
******************************************************************************************  */
// --- bits 0:4
#define D_LCD_OUTF_FORMAT_RGB121212             (0x00 << 0 ) // 5'b00_000,
#define D_LCD_OUTF_FORMAT_RGB101010             (0x01 << 0 ) // 5'b00_001,
#define D_LCD_OUTF_FORMAT_RGB888                (0x02 << 0 ) // 5'b00_010,  // was 5'd0  in mp_sabre
#define D_LCD_OUTF_FORMAT_RGB666                (0x03 << 0 ) // 5'b00_011,  // was 5'd1  in mp_sabre
#define D_LCD_OUTF_FORMAT_RGB565                (0x04 << 0 ) // 5'b00_100,  // was 5'd2  in mp_sabre
#define D_LCD_OUTF_FORMAT_RGB444                (0x05 << 0 ) // 5'b00_101,  // was 5'd3  in mp_sabre
#define D_LCD_OUTF_FORMAT_MRGB121212            (0x10 << 0 ) // 5'b10_000,
#define D_LCD_OUTF_FORMAT_MRGB101010            (0x11 << 0 ) // 5'b10_001,
#define D_LCD_OUTF_FORMAT_MRGB888               (0x12 << 0 ) // 5'b10_010,  // was 5'd8  in mp_sabre
#define D_LCD_OUTF_FORMAT_MRGB666               (0x13 << 0 ) // 5'b10_011,  // was 5'd9  in mp_sabre
#define D_LCD_OUTF_FORMAT_MRGB565               (0x14 << 0 ) // 5'b10_100,  // was 5'd10 in mp_sabre
#define D_LCD_OUTF_FORMAT_YCBCR420_8B_LEGACY    (0x08 << 0 ) // 5'b01_000,
#define D_LCD_OUTF_FORMAT_YCBCR420_8B_DCI       (0x09 << 0 ) // 5'b01_001,  // same as OUT_YCBCR420_8B_LEGACY
#define D_LCD_OUTF_FORMAT_YCBCR420_8B           (0x0A << 0 ) // 5'b01_010,
#define D_LCD_OUTF_FORMAT_YCBCR420_10B          (0x0B << 0 ) // 5'b01_011,
#define D_LCD_OUTF_FORMAT_YCBCR420_12B          (0x0C << 0 ) // 5'b01_100,
#define D_LCD_OUTF_FORMAT_YCBCR422_8B           (0x0D << 0 ) // 5'b01_101,  // was 5'd5  in mp_sabre
#define D_LCD_OUTF_FORMAT_YCBCR422_10B          (0x0E << 0 ) // 5'b01_110,
#define D_LCD_OUTF_FORMAT_YCBCR444              (0x0F << 0 ) // 5'b01_111,  // was 5'd4  in mp_sabre
#define D_LCD_OUTF_FORMAT_MYCBCR420_8B_LEGACY   (0x18 << 0 ) // 5'b11_000,
#define D_LCD_OUTF_FORMAT_MYCBCR420_8B_DCI      (0x19 << 0 ) // 5'b11_001,  // same as OUT_MYCBCR420_8B_LEGACY
#define D_LCD_OUTF_FORMAT_MYCBCR420_8B          (0x1A << 0 ) // 5'b11_010,
#define D_LCD_OUTF_FORMAT_MYCBCR420_10B         (0x1B << 0 ) // 5'b11_011,
#define D_LCD_OUTF_FORMAT_MYCBCR420_12B         (0x1C << 0 ) // 5'b11_100,
#define D_LCD_OUTF_FORMAT_MYCBCR422_8B          (0x1D << 0 ) // 5'b11_101,  // was 5'd12 in mp_sabre
#define D_LCD_OUTF_FORMAT_MYCBCR422_10B         (0x1E << 0 ) // 5'b11_110,
#define D_LCD_OUTF_FORMAT_MYCBCR444             (0x1F << 0 ) // 5'b11_111;  // was 5'd11 in mp_sabre
// --- bit 5
#define D_LCD_OUTF_BGR_ORDER    (1 << 5)   // default is 0, RGB order
// --- bit 6
#define D_LCD_OUTF_Y_ORDER      (1 << 6)   // Y after CB/Cr, default is Y before CB/CR
// --- bit 7
#define D_LCD_OUTF_CRCB_ORDER   (1 << 7)   // Cr before  Cb, default is Cb before Cr
// --- bit  8 - reserved used to be #define D_LCD_OUTF_PCLK_EXT     (1<<8) // use external clock, input from LCD, default is clock from CPR
// --- bit  8:10 -reserved used to be :
//#define D_LCD_OUTF_PCLK_DELAY_05 (0x100)  // delay pixel clk with 0.5ns, default is 0.0 ns
//#define D_LCD_OUTF_PCLK_DELAY_10 (0x200)  // delay pixel clk with 1.0ns
//#define D_LCD_OUTF_PCLK_DELAY_15 (0x300)  // delay pixel clk with 1.5ns
//#define D_LCD_OUTF_PCLK_DELAY_20 (0x400)  // delay pixel clk with 2.0ns
//#define D_LCD_OUTF_PCLK_DELAY_25 (0x500)  // delay pixel clk with 2.5ns
//#define D_LCD_OUTF_PCLK_DELAY_30 (0x600)  // delay pixel clk with 3.0ns
//#define D_LCD_OUTF_PCLK_DELAY_35 (0x700)  // delay pixel clk with 3.5ns
// --- bit 11
#define D_LCD_OUTF_EMBED_SYNC_MODE (1 << 11)   // ITU-R BT.656 embeded sincronization, default is with VSINC, HSINC signals
// ---bit 12
#define D_LCD_OUTF_ENABLE_CLIPPING (1 << 12)   // for YCrCb clip Y between 16 and 235 , Cr/Cb between 16 and 240, for RGB between 16 and 235, default is no clipping
// ---bit 13:14
#define D_LCD_OUTF_RGB_CONV_TRUNC             (1 << 13)  // default mode for RGB conversion is truncation
#define D_LCD_OUTF_RGB_CONV_EQUATIONS         (2 << 13)  // RGB conversion equations
#define D_LCD_OUTF_RGB_CONV_LSB_DIETHER_ERR   (3 << 13)  // LSB ditherring, accumulated errors
#define D_LCD_OUTF_RGB_CONV_LSB_DIETHER_RAN   (4 << 13)  // LSB ditherring, random number
// --- bit 15 - reserved  #define D_LCD_OUTF_FIELD_SETTINGS    (1<<15)  // use field settings, default is odd settins
// --- bit 16 
#define D_LCD_OUTF_CLIP_MODE_VALUE      (0 << 16)   // clip between 16 and 235
#define D_LCD_OUTF_CLIP_MODE_BITS       (1 << 16)   // clip number of bits "I think"


/* ******************************************************************************************
                                   LCD controller Layer config register
******************************************************************************************  */
// ---bit 1:2
#define D_LCD_LAYER_SCALE_H    (0x0002)    // enable horizontal scaling, default is no scaling
#define D_LCD_LAYER_SCALE_V    (0x0004)    // enable vertical scaling
#define D_LCD_LAYER_SCALE_H_V  (0x0006)    // enable vertical and horizontal scaling
// --- bit 3
#define D_LCD_LAYER_CSC_EN     (0x0008)    // enable CSC, default is bypassed
// --- bit 4:5
#define D_LCD_LAYER_ALPHA_STATIC (0x10)    // use static alpha value for layer, default is disabled
#define D_LCD_LAYER_ALPHA_EMBED  (0x20)    // use embeded value for alpha blending
#define D_LCD_LAYER_ALPHA_COMBI  (0x30)    // use static alpha and embedded value, by multiplication
// --- bit 6
#define D_LCD_LAYER_ALPHA_PREMULT  (0x40)    // indicates that the RGB values have been multiplied with alpha
// --- bit 7
#define D_LCD_LAYER_INVERT_COL     (0x80)    // enable color inversion, default is not inverted
// --- bit 8
#define D_LCD_LAYER_TRANSPARENT_EN (0x100)   // enable transparency
// --- bit 9:13
#define D_LCD_LAYER_FORMAT_YCBCR444PLAN (0x0000)   // default Layer config
#define D_LCD_LAYER_FORMAT_YCBCR422PLAN (0x0200)
#define D_LCD_LAYER_FORMAT_YCBCR420PLAN (0x0400)
#define D_LCD_LAYER_FORMAT_RGB888PLAN   (0x0600)
#define D_LCD_LAYER_FORMAT_YCBCR444LIN  (0x0800)
#define D_LCD_LAYER_FORMAT_YCBCR422LIN  (0x0A00)
#define D_LCD_LAYER_FORMAT_RGB888       (0x0C00)
#define D_LCD_LAYER_FORMAT_RGBA8888     (0x0E00)
#define D_LCD_LAYER_FORMAT_RGBX8888     (0x1000)
#define D_LCD_LAYER_FORMAT_RGB565       (0x1200)
#define D_LCD_LAYER_FORMAT_RGBA1555     (0x1400)
#define D_LCD_LAYER_FORMAT_XRGB1555     (0x1600)
#define D_LCD_LAYER_FORMAT_RGB444       (0x1800)
#define D_LCD_LAYER_FORMAT_RGBA4444     (0x1A00)
#define D_LCD_LAYER_FORMAT_RGBX4444     (0x1C00)
#define D_LCD_LAYER_FORMAT_RGB332       (0x1E00)
#define D_LCD_LAYER_FORMAT_RGBA3328     (0x2000)
#define D_LCD_LAYER_FORMAT_RGBX3328     (0x2200)
#define D_LCD_LAYER_FORMAT_CLUT         (0x2400)
// --- bit 14
#define D_LCD_LAYER_PLANAR_STORAGE      (0x4000)   // planar storege format
// --- bit 15:16
#define D_LCD_LAYER_8BPP                 (0x00000)
#define D_LCD_LAYER_16BPP                (0x08000)
#define D_LCD_LAYER_24BPP                (0x10000)
#define D_LCD_LAYER_32BPP                (0x18000)
// --- bit 17
#define D_LCD_LAYER_Y_ORDER              (0x020000) // Y after CRCb, default is Y before crcb
// --- bit 18
#define D_LCD_LAYER_CRCB_ORDER           (0x040000) // CR before Cb, default is CB before Cr
//--- but 19
#define D_LCD_LAYER_BGR_ORDER            (0x080000) // BGR order, default is RGB
// ---bit 20:21
#define D_LCD_LAYER_LUT_2ENT             (0x000000) // 2 entry clut, 1bpp
#define D_LCD_LAYER_LUT_4ENT             (0x100000) // 4 entry clut, 2bpp
#define D_LCD_LAYER_LUT_16ENT            (0x200000) // 18 entry clut, 4bpp
//--- bit 22:24
#define D_LCD_LAYER_NO_FLIP              (0x000000) // no flip or rotaton
#define D_LCD_LAYER_FLIP_V               (0x400000) // flip vertical
#define D_LCD_LAYER_FLIP_H               (0x800000) // flip horizontal
#define D_LCD_LAYER_ROT_R90              (0xC00000) // rotate right 90
#define D_LCD_LAYER_ROT_L90              (0x1000000) // rotate left 90
#define D_LCD_LAYER_ROT_180              (0x1400000) // rotate 180 (flip H & V )
// --- bit 25:26
#define D_LCD_LAYER_FIFO_00              (0x0000000) // fifo empty
#define D_LCD_LAYER_FIFO_25              (0x2000000) // fifo 25%
#define D_LCD_LAYER_FIFO_50              (0x4000000) // fifo 50%
#define D_LCD_LAYER_FIFO_100             (0x6000000) // fifo 100% , full

// --- bit 27:29
#define D_LCD_LAYER_INTERLEAVE_DIS         (0x00000000)
#define D_LCD_LAYER_INTERLEAVE_V           (0x08000000)
#define D_LCD_LAYER_INTERLEAVE_H           (0x10000000)
#define D_LCD_LAYER_INTERLEAVE_CH          (0x18000000)
#define D_LCD_LAYER_INTERLEAVE_V_SUB       (0x20000000)
#define D_LCD_LAYER_INTERLEAVE_H_SUB       (0x28000000)
#define D_LCD_LAYER_INTERLEAVE_CH_SUB      (0x30000000)
//bit 30
#define D_LCD_LAYER_INTER_POS_EVEN    (0x00000000)
#define D_LCD_LAYER_INTER_POS_ODD     (0x40000000)

/* ******************************************************************************************
                                   LCD controller Layer DMA config register
******************************************************************************************  */
// bit 0
#define D_LCD_DMA_LAYER_ENABLE        (0x001)   //default is disabled
// bit 1
#define D_LCD_DMA_LAYER_STATUS        (0x002)   // this should be used only as a mask when reading the status from the DMA CFG register
// bit 2
#define D_LCD_DMA_LAYER_AUTO_UPDATE   (0x004)
// bit 3
#define D_LCD_DMA_LAYER_CONT_UPDATE   (0x008)
// bit 2 + bit 3
#define D_LCD_DMA_LAYER_CONT_PING_PONG_UPDATE   (0x00C)
// bit 4
#define D_LCD_DMA_LAYER_FIFO_ADR_MODE (0x010)  // set FIFO addressing mode, default is increment after each burst
// bit 5:9
#define D_LCD_DMA_LAYER_AXI_BURST_1     (0x020)  // default axi burst is 1
#define D_LCD_DMA_LAYER_AXI_BURST_2     (0x040)
#define D_LCD_DMA_LAYER_AXI_BURST_3     (0x060)
#define D_LCD_DMA_LAYER_AXI_BURST_4     (0x080)
#define D_LCD_DMA_LAYER_AXI_BURST_5     (0x0A0)
#define D_LCD_DMA_LAYER_AXI_BURST_6     (0x0C0)
#define D_LCD_DMA_LAYER_AXI_BURST_7     (0x0E0)
#define D_LCD_DMA_LAYER_AXI_BURST_8     (0x100)
#define D_LCD_DMA_LAYER_AXI_BURST_9     (0x120)
#define D_LCD_DMA_LAYER_AXI_BURST_10    (0x140)
#define D_LCD_DMA_LAYER_AXI_BURST_11    (0x160)
#define D_LCD_DMA_LAYER_AXI_BURST_12    (0x180)
#define D_LCD_DMA_LAYER_AXI_BURST_13    (0x1A0)
#define D_LCD_DMA_LAYER_AXI_BURST_14    (0x1C0)
#define D_LCD_DMA_LAYER_AXI_BURST_15    (0x1E0)
#define D_LCD_DMA_LAYER_AXI_BURST_16    (0x200)
// bit 10
#define D_LCD_DMA_LAYER_V_STRIDE_EN     (0x400)

/* ******************************************************************************************
                                   LCD controller PWM ctrl
******************************************************************************************  */
//bit0:2
#define D_LCD_PWM_TRIG_1H       (0x0000)   // trigger on the rising edge of the Hsinc
#define D_LCD_PWM_TRIG_0H       (0x0001)   // trigger on the falling edge of the Hsinc
#define D_LCD_PWM_TRIG_1V       (0x0002)   // trigger on the rising edge of the Vsinc
#define D_LCD_PWM_TRIG_0V       (0x0003)   // trigger on the falling edge of the Vsinc
#define D_LCD_PWM_TRIG_TIM_GEN  (0x0004)   // trigger on timing generator
// bit 3
#define D_LCD_PWM_REP_EN   (0x0008)   // repeat enable
// bit 4
#define D_LCD_PWM_INVERT   (0x0010)   // PWM out invert ctrl

/* ******************************************************************************************
                                  Interrupts
******************************************************************************************  */
// bit 0
#define D_LCD_INT_EOF              (0x0001)
#define D_LCD_INT_LINE_CMP         (0x0002)
#define D_LCD_INT_V_CMP            (0x0004)
#define D_LCD_INT_L0_DMA_DONE      (0x0008)
// bit 4
#define D_LCD_INT_L0_DMA_DONE_NA   (0x0010)
#define D_LCD_INT_L0_FIFO_OVER     (0x0020)
#define D_LCD_INT_L0_FIFO_UNDER    (0x0040)
#define D_LCD_INT_L0_FIFO_CB_OVER  (0x0080)
//bit 8
#define D_LCD_INT_L0_FIFO_CB_UNDER (0x0100)
#define D_LCD_INT_L0_FIFO_CR_OVER  (0x0200)
#define D_LCD_INT_L0_FIFO_CR_UNDER (0x0400)
#define D_LCD_INT_L1_DMA_DONE      (0x0800)
// bit 12
#define D_LCD_INT_L1_DMA_DONE_NA   (0x1000)
#define D_LCD_INT_L1_FIFO_OVER     (0x2000)
#define D_LCD_INT_L1_FIFO_UNDER    (0x4000)
#define D_LCD_INT_L1_FIFO_CB_OVER  (0x8000)
//bit 16
#define D_LCD_INT_L1_FIFO_CB_UNDER (0x00010000)
#define D_LCD_INT_L1_FIFO_CR_OVER  (0x00020000)
#define D_LCD_INT_L1_FIFO_CR_UNDER (0x00040000)
#define D_LCD_INT_L2_DMA_DONE      (0x00080000)
//bit 20
#define D_LCD_INT_L2_DMA_DONE_NA   (0x00100000)
#define D_LCD_INT_L2_FIFO_OVER     (0x00200000)
#define D_LCD_INT_L2_FIFO_UNDER    (0x00400000)
#define D_LCD_INT_L3_DMA_DONE      (0x00800000)
//bit 24
#define D_LCD_INT_L3_DMA_DONE_NA   (0x01000000)
#define D_LCD_INT_L3_FIFO_OVER     (0x02000000)
#define D_LCD_INT_L3_FIFO_UNDER    (0x04000000)
#define D_LCD_INT_I80_CMD_DONE     (0x08000000)
//bit 28
#define D_LCD_INT_I80_RGB_DONE     (0x10000000)



/* ******************************************************************************************
// LCD VSync status
******************************************************************************************  */
//bits 0 .. 11
#define D_LCD_FRAME_COUNT (0xFFF)
//bit 12 -> field type
#define D_LCD_CRT_FIELD (0x1000)
//bits 13:14 Active region-10, VSYNC-00, backportch-01 or frontportch-11
#define D_LCD_VERTICAL_STATUS (0x6000)

#endif


