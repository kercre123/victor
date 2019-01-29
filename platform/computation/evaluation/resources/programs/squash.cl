__kernel void squash(
  __read_only image2d_t input,
  __write_only image2d_t output
)
{
  const int2 in_pos00 = {get_global_id(0)*5, get_global_id(1)*2};

  const uchar byte00 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00              ).s0), 0.0f, 255.0f);
  const uchar byte01 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00 + (int2)(1,0)).s0), 0.0f, 255.0f);
  const uchar byte02 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00 + (int2)(2,0)).s0), 0.0f, 255.0f);
  const uchar byte03 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00 + (int2)(3,0)).s0), 0.0f, 255.0f);
  const uchar byte04 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00 + (int2)(4,0)).s0), 0.0f, 255.0f);
  const uchar byte10 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00 + (int2)(0,1)).s0), 0.0f, 255.0f);
  const uchar byte11 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00 + (int2)(1,1)).s0), 0.0f, 255.0f);
  const uchar byte12 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00 + (int2)(2,1)).s0), 0.0f, 255.0f);
  const uchar byte13 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00 + (int2)(3,1)).s0), 0.0f, 255.0f);
  const uchar byte14 = clamp(round(255*read_imagef(input, g_sampler_int, in_pos00 + (int2)(4,1)).s0), 0.0f, 255.0f);

  const float b00 = ((byte00 << 2) | ((byte04 & 0xC0) >> 6))/1023.0f;
  const float g01 = ((byte01 << 2) | ((byte04 & 0x30) >> 4))/1023.0f;
  const float b02 = ((byte02 << 2) | ((byte04 & 0x0C) >> 2))/1023.0f;
  const float g03 = ((byte03 << 2) | ((byte04 & 0x03)     ))/1023.0f;
  const float g10 = ((byte10 << 2) | ((byte14 & 0xC0) >> 6))/1023.0f;
  const float r11 = ((byte11 << 2) | ((byte14 & 0x30) >> 4))/1023.0f;
  const float g12 = ((byte12 << 2) | ((byte14 & 0x0C) >> 2))/1023.0f;
  const float r13 = ((byte13 << 2) | ((byte14 & 0x03)     ))/1023.0f;

  const float4 rgb00 = (float4)(b00, 0.0f, 0.0f, 1.0f);
  const float4 rgb01 = (float4)(g01, 0.0f, 0.0f, 1.0f);
  const float4 rgb02 = (float4)(b02, 0.0f, 0.0f, 1.0f);
  const float4 rgb03 = (float4)(g03, 0.0f, 0.0f, 1.0f);
  const float4 rgb10 = (float4)(g10, 0.0f, 0.0f, 1.0f);
  const float4 rgb11 = (float4)(r11, 0.0f, 0.0f, 1.0f);
  const float4 rgb12 = (float4)(g12, 0.0f, 0.0f, 1.0f);
  const float4 rgb13 = (float4)(r13, 0.0f, 0.0f, 1.0f);

  const int2 out_pos00 = {get_global_id(0)*4, get_global_id(1)*2};

  write_imagef(output, out_pos00               , rgb00);
  write_imagef(output, out_pos00 + (int2)(1, 0), rgb01);
  write_imagef(output, out_pos00 + (int2)(2, 0), rgb02);
  write_imagef(output, out_pos00 + (int2)(3, 0), rgb03);
  write_imagef(output, out_pos00 + (int2)(0, 1), rgb10);
  write_imagef(output, out_pos00 + (int2)(1, 1), rgb11);
  write_imagef(output, out_pos00 + (int2)(2, 1), rgb12);
  write_imagef(output, out_pos00 + (int2)(3, 1), rgb13);
}