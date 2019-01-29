/**
 * Global settings
 */

// Use g_sampler_int when doing read_imagef look-ups with integer position in range [ [0,0], [width,height] ]
__constant sampler_t g_sampler_int = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

// Use g_sampler_flt when doing read_imagef look-ups with floating point position in range [ [0,0], [1,1] ]
__constant sampler_t g_sampler_flt = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

// Vector transformation for converting rgb to greyscale
__constant float4 g_grayscale = { 0.2989f, 0.5870f, 0.1140f, 0 };



float8 read_imagef_bayer_raw10(__read_only image2d_t image, const int2 pos)
{
  // Get the bytes:
  //  1) Read the float4 values from the image (r,0,0,1)
  //  2) Scale the value from [0,1] to [0,255]
  //  3) Convert to uchar rounding to nearest value and clamp values outside the uchar range
  uchar16 bytes;
  bytes.s0 = clamp(round(255*read_imagef(image, g_sampler_int, pos              ).s0), 0.0f, 255.0f);
  bytes.s1 = clamp(round(255*read_imagef(image, g_sampler_int, pos + (int2)(1,0)).s0), 0.0f, 255.0f);
  bytes.s2 = clamp(round(255*read_imagef(image, g_sampler_int, pos + (int2)(2,0)).s0), 0.0f, 255.0f);
  bytes.s3 = clamp(round(255*read_imagef(image, g_sampler_int, pos + (int2)(3,0)).s0), 0.0f, 255.0f);
  bytes.s4 = clamp(round(255*read_imagef(image, g_sampler_int, pos + (int2)(4,0)).s0), 0.0f, 255.0f);
  bytes.s5 = clamp(round(255*read_imagef(image, g_sampler_int, pos + (int2)(0,1)).s0), 0.0f, 255.0f);
  bytes.s6 = clamp(round(255*read_imagef(image, g_sampler_int, pos + (int2)(1,1)).s0), 0.0f, 255.0f);
  bytes.s7 = clamp(round(255*read_imagef(image, g_sampler_int, pos + (int2)(2,1)).s0), 0.0f, 255.0f);
  bytes.s8 = clamp(round(255*read_imagef(image, g_sampler_int, pos + (int2)(3,1)).s0), 0.0f, 255.0f);
  bytes.s9 = clamp(round(255*read_imagef(image, g_sampler_int, pos + (int2)(4,1)).s0), 0.0f, 255.0f);

  // Turn the bytes into their full 10bit BGGR values
  float8 values;
  values.s0 = ((bytes.s0 << 2) | ((bytes.s4 & 0xC0) >> 6))/1023.0f;
  values.s1 = ((bytes.s1 << 2) | ((bytes.s4 & 0x30) >> 4))/1023.0f;
  values.s2 = ((bytes.s2 << 2) | ((bytes.s4 & 0x0C) >> 2))/1023.0f;
  values.s3 = ((bytes.s3 << 2) | ((bytes.s4 & 0x03)     ))/1023.0f;
  values.s4 = ((bytes.s5 << 2) | ((bytes.s9 & 0xC0) >> 6))/1023.0f;
  values.s5 = ((bytes.s6 << 2) | ((bytes.s9 & 0x30) >> 4))/1023.0f;
  values.s6 = ((bytes.s7 << 2) | ((bytes.s9 & 0x0C) >> 2))/1023.0f;
  values.s7 = ((bytes.s8 << 2) | ((bytes.s9 & 0x03)     ))/1023.0f;
  return values;
}

void write_imagef_bayer_rgb(__write_only image2d_t image, const int2 pos, const float8 values)
{
  // Create the RGB pixels from the BGGR values
  const float b00 = values.s0;
  const float g01 = values.s1;
  const float b02 = values.s2;
  const float g03 = values.s3;
  const float g10 = values.s4;
  const float r11 = values.s5;
  const float g12 = values.s6;
  const float r13 = values.s7;

  const float4 rgb00 = (float4)(r11, g01, b00, 1.0f);
  const float4 rgb01 = (float4)(r11, g01, b00, 1.0f);
  const float4 rgb02 = (float4)(r13, g03, b02, 1.0f);
  const float4 rgb03 = (float4)(r13, g03, b02, 1.0f);
  const float4 rgb10 = (float4)(r11, g10, b00, 1.0f);
  const float4 rgb11 = (float4)(r11, g10, b00, 1.0f);
  const float4 rgb12 = (float4)(r13, g12, b02, 1.0f);
  const float4 rgb13 = (float4)(r13, g12, b02, 1.0f);

  write_imagef(image, pos               , rgb00);
  write_imagef(image, pos + (int2)(1, 0), rgb01);
  write_imagef(image, pos + (int2)(2, 0), rgb02);
  write_imagef(image, pos + (int2)(3, 0), rgb03);
  write_imagef(image, pos + (int2)(0, 1), rgb10);
  write_imagef(image, pos + (int2)(1, 1), rgb11);
  write_imagef(image, pos + (int2)(2, 1), rgb12);
  write_imagef(image, pos + (int2)(3, 1), rgb13);
}

void write_imagef_bayer_grey(__write_only image2d_t image, const int2 pos, const float8 values)
{
  const float b00 = values.s0;
  const float g01 = values.s1;
  const float b02 = values.s2;
  const float g03 = values.s3;
  const float g10 = values.s4;
  const float r11 = values.s5;
  const float g12 = values.s6;
  const float r13 = values.s7;

  const float4 rgb00 = (float4)(g01, 0.0f, 0.0f, 1.0f);
  const float4 rgb01 = (float4)(g01, 0.0f, 0.0f, 1.0f);
  const float4 rgb02 = (float4)(g03, 0.0f, 0.0f, 1.0f);
  const float4 rgb03 = (float4)(g03, 0.0f, 0.0f, 1.0f);
  const float4 rgb10 = (float4)(g10, 0.0f, 0.0f, 1.0f);
  const float4 rgb11 = (float4)(g10, 0.0f, 0.0f, 1.0f);
  const float4 rgb12 = (float4)(g12, 0.0f, 0.0f, 1.0f);
  const float4 rgb13 = (float4)(g12, 0.0f, 0.0f, 1.0f);

  write_imagef(image, pos               , rgb00);
  write_imagef(image, pos + (int2)(1, 0), rgb01);
  write_imagef(image, pos + (int2)(2, 0), rgb02);
  write_imagef(image, pos + (int2)(3, 0), rgb03);
  write_imagef(image, pos + (int2)(0, 1), rgb10);
  write_imagef(image, pos + (int2)(1, 1), rgb11);
  write_imagef(image, pos + (int2)(2, 1), rgb12);
  write_imagef(image, pos + (int2)(3, 1), rgb13);
}