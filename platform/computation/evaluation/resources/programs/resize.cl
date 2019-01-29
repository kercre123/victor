__kernel void resize(
  __read_only image2d_t input,
  __write_only image2d_t output
)
{
  int2 out_dim = get_image_dim(output);
  int2 out_pos = {get_global_id(0), get_global_id(1)};
  float2 in_pos = convert_float2(out_pos) /convert_float2(out_dim);
  float4 pixel = read_imagef(input, g_sampler_flt, in_pos);
  write_imagef(output, out_pos, pixel);
}