__kernel void pipeline(
  __read_only image2d_t bayer,
  __write_only image2d_t full_rgb,
  __write_only image2d_t full_grey,
  __write_only image2d_t half_rgb,
  __write_only image2d_t half_grey)
{
  const int2 in_pos00 = {get_global_id(0)*10, get_global_id(1)*4};
  const int2 in_pos01 = in_pos00 + (int2)(5,0);
  const int2 in_pos10 = in_pos00 + (int2)(0,2);
  const int2 in_pos11 = in_pos00 + (int2)(5,2);

  const float8 values00 = read_imagef_bayer_raw10(bayer, in_pos00);
  const float8 values01 = read_imagef_bayer_raw10(bayer, in_pos01);
  const float8 values10 = read_imagef_bayer_raw10(bayer, in_pos10);
  const float8 values11 = read_imagef_bayer_raw10(bayer, in_pos11);

  const int2 full_pos00 = {get_global_id(0)*8, get_global_id(1)*4};
  const int2 full_pos01 = full_pos00 + (int2)(4,0);
  const int2 full_pos10 = full_pos00 + (int2)(0,2);
  const int2 full_pos11 = full_pos00 + (int2)(4,2);

  const int2 half_pos00 = {get_global_id(0)*4, get_global_id(1)*2};

  write_imagef_bayer_rgb(full_rgb, full_pos00, values00);
  write_imagef_bayer_rgb(full_rgb, full_pos01, values01);
  write_imagef_bayer_rgb(full_rgb, full_pos10, values10);
  write_imagef_bayer_rgb(full_rgb, full_pos11, values11);

  write_imagef_bayer_rgb(half_rgb, half_pos00, values00);

  write_imagef_bayer_grey(full_grey, full_pos00, values00);
  write_imagef_bayer_grey(full_grey, full_pos01, values01);
  write_imagef_bayer_grey(full_grey, full_pos10, values10);
  write_imagef_bayer_grey(full_grey, full_pos11, values11);

  write_imagef_bayer_grey(half_grey, half_pos00, values00);
}