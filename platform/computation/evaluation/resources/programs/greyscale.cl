__kernel void greyscale(
  __read_only image2d_t input,
  __write_only image2d_t output
)
{
  int2 pos = {get_global_id(0), get_global_id(1)};
  float4 rgb = read_imagef(input, g_sampler_int, pos);
  float4 pixel = dot(g_grayscale, rgb);
  write_imagef(output, pos, pixel);
}