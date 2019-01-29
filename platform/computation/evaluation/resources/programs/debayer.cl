/**
 * BGGR10 RAW10 -> RGB888
 * RAW10 Byte Layout:
 *     Byte 0:  P0[9]  P0[8]  P0[7]  P0[6]  P0[5]  P0[4]  P0[3]  P0[2]
 *     Byte 1:  P1[9]  P1[8]  P1[7]  P1[6]  P1[5]  P1[4]  P1[3]  P1[2]
 *     Byte 2:  P2[9]  P2[8]  P2[7]  P2[6]  P2[5]  P2[4]  P2[3]  P2[2]
 *     Byte 3:  P3[9]  P3[8]  P3[7]  P3[6]  P3[5]  P3[4]  P3[3]  P3[2]
 *     Byte 4:  P3[1]  P3[0]  P2[1]  P2[0]  P1[1]  P1[0]  P0[1]  P0[0]
 *
 * Input[row,col]
 *     byte00(row  ,col)  byte01(row  ,col+1)  byte02(row  ,col+2)  byte03(row  ,col+3)  byte04(row  ,col+3)
 *     byte10(row+1,col)  byte11(row+1,col+1)  byte12(row+1,col+2)  byte13(row+1,col+3)  byte14(row+1,col+3)
 *
 *     b00  g01  b02  g03  byte04
 *     g10  r11  g12  r13  byte14
 * 
 * Output[row,col]
 *     rgb00(row  ,col)  rgb01(row  ,col+1)  rgb02(row  ,col+2)  rgb03(row  ,col+3)
 *     rgb10(row+1,col)  rgb11(row+1,col+1)  rgb12(row+1,col+2)  rgb13(row+1,col+3)
 *
 * Iterate 5 columns by 2 rows on the output image
 */

__kernel void debayer(
__global const uchar* input,
__global uchar* output,
  const int in_w,
  const int out_w)
{
  // =====================
  // Load the RAW10 bytes

  const int2 in_pos00 = {get_global_id(0)*5, get_global_id(1)*2};
  const int2 in_pos01 = in_pos00 + (int2)(1, 0);
  const int2 in_pos02 = in_pos00 + (int2)(2, 0);
  const int2 in_pos03 = in_pos00 + (int2)(3, 0);
  const int2 in_pos04 = in_pos00 + (int2)(4, 0);
  const int2 in_pos10 = in_pos00 + (int2)(0, 1);
  const int2 in_pos11 = in_pos00 + (int2)(1, 1);
  const int2 in_pos12 = in_pos00 + (int2)(2, 1);
  const int2 in_pos13 = in_pos00 + (int2)(3, 1);
  const int2 in_pos14 = in_pos00 + (int2)(4, 1);

  const int index00 = in_pos00.y * in_w + in_pos00.x;
  const int index01 = in_pos01.y * in_w + in_pos01.x;
  const int index02 = in_pos02.y * in_w + in_pos02.x;
  const int index03 = in_pos03.y * in_w + in_pos03.x;
  const int index04 = in_pos04.y * in_w + in_pos04.x;
  const int index10 = in_pos10.y * in_w + in_pos10.x;
  const int index11 = in_pos11.y * in_w + in_pos11.x;
  const int index12 = in_pos12.y * in_w + in_pos12.x;
  const int index13 = in_pos13.y * in_w + in_pos13.x;
  const int index14 = in_pos14.y * in_w + in_pos14.x;

  const int byte00 = input[index00];
  const int byte01 = input[index01];
  const int byte02 = input[index02];
  const int byte03 = input[index03];
  const int byte04 = input[index04];
  const int byte10 = input[index10];
  const int byte11 = input[index11];
  const int byte12 = input[index12];
  const int byte13 = input[index13];
  const int byte14 = input[index14];

  // ===============================================
  // Extract the BGGR10 values from the RAW10 bytes

  // TODO: Which one of these is right? According to RAW10 it should be the first one.

  // int b00 = (byte00 << 2) | ((byte04 & 0x03)     );
  // int g01 = (byte01 << 2) | ((byte04 & 0x0C) >> 2);
  // int b02 = (byte02 << 2) | ((byte04 & 0x30) >> 4);
  // int g03 = (byte03 << 2) | ((byte04 & 0xC0) >> 6);
  // int g10 = (byte10 << 2) | ((byte14 & 0x03)     );
  // int r11 = (byte11 << 2) | ((byte14 & 0x0C) >> 2);
  // int g12 = (byte12 << 2) | ((byte14 & 0x30) >> 4);
  // int r13 = (byte13 << 2) | ((byte14 & 0xC0) >> 6);

  int b00 = (byte00 << 2) | ((byte04 & 0xC0) >> 6);
  int g01 = (byte01 << 2) | ((byte04 & 0x30) >> 4);
  int b02 = (byte02 << 2) | ((byte04 & 0x0C) >> 2);
  int g03 = (byte03 << 2) | ((byte04 & 0x03)     );
  int g10 = (byte10 << 2) | ((byte14 & 0xC0) >> 6);
  int r11 = (byte11 << 2) | ((byte14 & 0x30) >> 4);
  int g12 = (byte12 << 2) | ((byte14 & 0x0C) >> 2);
  int r13 = (byte13 << 2) | ((byte14 & 0x03)     );

  // =============================================
  // Convert the BGGR10 values into RGB888 values

  // Convert from 10-bit to 8-bit by scaling based on max values: v*(255/1023)
  b00 = round(255.0f * (b00 / 1023.0f));
  g01 = round(255.0f * (g01 / 1023.0f));
  b02 = round(255.0f * (b02 / 1023.0f));
  g03 = round(255.0f * (g03 / 1023.0f));
  g10 = round(255.0f * (g10 / 1023.0f));
  r11 = round(255.0f * (r11 / 1023.0f));
  g12 = round(255.0f * (g12 / 1023.0f));
  r13 = round(255.0f * (r13 / 1023.0f));

  const uint4 rgb00 = (uint4)(r11, g01, b00, 255);
  const uint4 rgb01 = (uint4)(r11, g01, b00, 255);
  const uint4 rgb02 = (uint4)(r13, g03, b02, 255);
  const uint4 rgb03 = (uint4)(r13, g03, b02, 255);
  const uint4 rgb10 = (uint4)(r11, g10, b00, 255);
  const uint4 rgb11 = (uint4)(r11, g10, b00, 255);
  const uint4 rgb12 = (uint4)(r13, g12, b02, 255);
  const uint4 rgb13 = (uint4)(r13, g12, b02, 255);

  // =============================================
  // Save the RGB values to the output array

  const int2 out_pos00 = {get_global_id(0)*4, get_global_id(1)*2};
  const int2 out_pos01 = out_pos00 + (int2)(1, 0);
  const int2 out_pos02 = out_pos00 + (int2)(2, 0);
  const int2 out_pos03 = out_pos00 + (int2)(3, 0);
  const int2 out_pos10 = out_pos00 + (int2)(0, 1);
  const int2 out_pos11 = out_pos00 + (int2)(1, 1);
  const int2 out_pos12 = out_pos00 + (int2)(2, 1);
  const int2 out_pos13 = out_pos00 + (int2)(3, 1);

  const int outdex00 = 3*(out_pos00.y * out_w + out_pos00.x);
  const int outdex01 = 3*(out_pos01.y * out_w + out_pos01.x);
  const int outdex02 = 3*(out_pos02.y * out_w + out_pos02.x);
  const int outdex03 = 3*(out_pos03.y * out_w + out_pos03.x);
  const int outdex10 = 3*(out_pos10.y * out_w + out_pos10.x);
  const int outdex11 = 3*(out_pos11.y * out_w + out_pos11.x);
  const int outdex12 = 3*(out_pos12.y * out_w + out_pos12.x);
  const int outdex13 = 3*(out_pos13.y * out_w + out_pos13.x);

  output[outdex00]   = rgb00.s0;
  output[outdex00+1] = rgb00.s1;
  output[outdex00+2] = rgb00.s2;

  output[outdex01]   = rgb01.s0;
  output[outdex01+1] = rgb01.s1;
  output[outdex01+2] = rgb01.s2;

  output[outdex02]   = rgb02.s0;
  output[outdex02+1] = rgb02.s1;
  output[outdex02+2] = rgb02.s2;

  output[outdex03]   = rgb03.s0;
  output[outdex03+1] = rgb03.s1;
  output[outdex03+2] = rgb03.s2;

  output[outdex10]   = rgb10.s0;
  output[outdex10+1] = rgb10.s1;
  output[outdex10+2] = rgb10.s2;

  output[outdex11]   = rgb11.s0;
  output[outdex11+1] = rgb11.s1;
  output[outdex11+2] = rgb11.s2;

  output[outdex12]   = rgb12.s0;
  output[outdex12+1] = rgb12.s1;
  output[outdex12+2] = rgb12.s2;

  output[outdex13]   = rgb13.s0;
  output[outdex13+1] = rgb13.s1;
  output[outdex13+2] = rgb13.s2;

}

//======================================================================================================================

__kernel void debayer_half(
__global const uchar* input,
__global uchar* output,
  const int in_w,
  const int out_w)
{
  // =====================
  // Load the RAW10 bytes

  const int2 in_pos00 = {get_global_id(0)*10, get_global_id(1)*4};
  const int2 in_pos01 = in_pos00 + (int2)(1, 0);
  const int2 in_pos02 = in_pos00 + (int2)(2, 0);
  const int2 in_pos03 = in_pos00 + (int2)(3, 0);
  const int2 in_pos04 = in_pos00 + (int2)(4, 0);
  const int2 in_pos10 = in_pos00 + (int2)(0, 1);
  const int2 in_pos11 = in_pos00 + (int2)(1, 1);
  const int2 in_pos12 = in_pos00 + (int2)(2, 1);
  const int2 in_pos13 = in_pos00 + (int2)(3, 1);
  const int2 in_pos14 = in_pos00 + (int2)(4, 1);

  const int index00 = in_pos00.y * in_w + in_pos00.x;
  const int index01 = in_pos01.y * in_w + in_pos01.x;
  const int index02 = in_pos02.y * in_w + in_pos02.x;
  const int index03 = in_pos03.y * in_w + in_pos03.x;
  const int index04 = in_pos04.y * in_w + in_pos04.x;
  const int index10 = in_pos10.y * in_w + in_pos10.x;
  const int index11 = in_pos11.y * in_w + in_pos11.x;
  const int index12 = in_pos12.y * in_w + in_pos12.x;
  const int index13 = in_pos13.y * in_w + in_pos13.x;
  const int index14 = in_pos14.y * in_w + in_pos14.x;

  const int byte00 = input[index00];
  const int byte01 = input[index01];
  const int byte02 = input[index02];
  const int byte03 = input[index03];
  const int byte04 = input[index04];
  const int byte10 = input[index10];
  const int byte11 = input[index11];
  const int byte12 = input[index12];
  const int byte13 = input[index13];
  const int byte14 = input[index14];

  // ===============================================
  // Extract the BGGR10 values from the RAW10 bytes

  // TODO: Which one of these is right? According to RAW10 it should be the first one.

  int b00 = (byte00 << 2) |  (byte04 & 0x03);
  int g01 = (byte01 << 2) | ((byte04 & 0x0C) >> 2);
  int b02 = (byte02 << 2) | ((byte04 & 0x30) >> 4);
  int g03 = (byte03 << 2) | ((byte04 & 0xC0) >> 6);
  int g10 = (byte10 << 2) |  (byte14 & 0x03);
  int r11 = (byte11 << 2) | ((byte14 & 0x0C) >> 2);
  int g12 = (byte12 << 2) | ((byte14 & 0x30) >> 4);
  int r13 = (byte13 << 2) | ((byte14 & 0xC0) >> 6);

  // =============================================
  // Convert the BGGR10 values into RGB888 values

  // Convert from 10-bit to 8-bit by scaling based on max values: v*(255/1023)
  b00 = round(255.0f * (b00 / 1023.0f));
  g01 = round(255.0f * (g01 / 1023.0f));
  b02 = round(255.0f * (b02 / 1023.0f));
  g03 = round(255.0f * (g03 / 1023.0f));
  g10 = round(255.0f * (g10 / 1023.0f));
  r11 = round(255.0f * (r11 / 1023.0f));
  g12 = round(255.0f * (g12 / 1023.0f));
  r13 = round(255.0f * (r13 / 1023.0f));

  const uint4 rgb00 = (uint4)(r11, g01, b00, 255);
  const uint4 rgb01 = (uint4)(r11, g01, b00, 255);
  const uint4 rgb02 = (uint4)(r13, g03, b02, 255);
  const uint4 rgb03 = (uint4)(r13, g03, b02, 255);
  const uint4 rgb10 = (uint4)(r11, g10, b00, 255);
  const uint4 rgb11 = (uint4)(r11, g10, b00, 255);
  const uint4 rgb12 = (uint4)(r13, g12, b02, 255);
  const uint4 rgb13 = (uint4)(r13, g12, b02, 255);

  // =============================================
  // Save the RGB values to the output array

  const int2 out_pos00 = {get_global_id(0)*4, get_global_id(1)*2};
  const int2 out_pos01 = out_pos00 + (int2)(1, 0);
  const int2 out_pos02 = out_pos00 + (int2)(2, 0);
  const int2 out_pos03 = out_pos00 + (int2)(3, 0);
  const int2 out_pos10 = out_pos00 + (int2)(0, 1);
  const int2 out_pos11 = out_pos00 + (int2)(1, 1);
  const int2 out_pos12 = out_pos00 + (int2)(2, 1);
  const int2 out_pos13 = out_pos00 + (int2)(3, 1);

  const int outdex00 = 3*(out_pos00.y * out_w + out_pos00.x);
  const int outdex01 = 3*(out_pos01.y * out_w + out_pos01.x);
  const int outdex02 = 3*(out_pos02.y * out_w + out_pos02.x);
  const int outdex03 = 3*(out_pos03.y * out_w + out_pos03.x);
  const int outdex10 = 3*(out_pos10.y * out_w + out_pos10.x);
  const int outdex11 = 3*(out_pos11.y * out_w + out_pos11.x);
  const int outdex12 = 3*(out_pos12.y * out_w + out_pos12.x);
  const int outdex13 = 3*(out_pos13.y * out_w + out_pos13.x);

  output[outdex00]   = rgb00.s0;
  output[outdex00+1] = rgb00.s1;
  output[outdex00+2] = rgb00.s2;

  output[outdex01]   = rgb01.s0;
  output[outdex01+1] = rgb01.s1;
  output[outdex01+2] = rgb01.s2;

  output[outdex02]   = rgb02.s0;
  output[outdex02+1] = rgb02.s1;
  output[outdex02+2] = rgb02.s2;

  output[outdex03]   = rgb03.s0;
  output[outdex03+1] = rgb03.s1;
  output[outdex03+2] = rgb03.s2;

  output[outdex10]   = rgb10.s0;
  output[outdex10+1] = rgb10.s1;
  output[outdex10+2] = rgb10.s2;

  output[outdex11]   = rgb11.s0;
  output[outdex11+1] = rgb11.s1;
  output[outdex11+2] = rgb11.s2;

  output[outdex12]   = rgb12.s0;
  output[outdex12+1] = rgb12.s1;
  output[outdex12+2] = rgb12.s2;

  output[outdex13]   = rgb13.s0;
  output[outdex13+1] = rgb13.s1;
  output[outdex13+2] = rgb13.s2;

}

//======================================================================================================================

__kernel void debayer_img(
  __read_only image2d_t input,
  __write_only image2d_t output
)
{
  const int2 in_pos = {get_global_id(0)*5, get_global_id(1)*2};
  const int2 out_pos = {get_global_id(0)*4, get_global_id(1)*2};

  const float8 values = read_imagef_bayer_raw10(input, in_pos);
  write_imagef_bayer_rgb(output, out_pos, values);
}

//======================================================================================================================

__kernel void debayer_img_half(
  __read_only image2d_t input,
  __write_only image2d_t output
)
{
  const int2 in_pos = {get_global_id(0)*10, get_global_id(1)*4};
  const int2 out_pos = {get_global_id(0)*4, get_global_id(1)*2};

  const float8 values = read_imagef_bayer_raw10(input, in_pos);
  write_imagef_bayer_rgb(output, out_pos, values);
}