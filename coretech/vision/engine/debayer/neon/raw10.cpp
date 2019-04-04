#ifdef __ARM_NEON__

#include "coretech/vision/engine/debayer/neon/raw10.h"

#include <arm_neon.h>
#include <cmath>

namespace Anki {
namespace Vision {
namespace Neon {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

RAW10toRGB24::RAW10toRGB24() : Op()
{
  // Setup a gamma table for 128 (2^7) possible input values
  for (int i = 0; i < _gammaLUT.size(); ++i){
    _gammaLUT[i] = 255 * std::powf((f32)i/127.0f, Debayer::GAMMA);
  }
}

Result RAW10toRGB24::operator()(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  u8 outChannels = 3;
  u8 inChannels = 1;

  u8 sampleRate = 1;
  if (!Debayer::SampleRateFromScale(outArgs.scale, sampleRate))
  {
    return RESULT_FAIL;
  }

  // How many columns bytes to iterate over in the output image
  s32 rowStep = 2;
  s32 colStep = 8;

  u32 inColSkip = 5 * sampleRate;
  u32 outColSkip = colStep * outChannels;
  
  // Skip the rest of the bytes in a row and move down an extra rows;
  u32 inRowSkip = (2*sampleRate-1)*(inChannels * (inArgs.width * 5)/4);
  u32 outRowSkip = outChannels * outArgs.width;

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + inRowSkip;

  u8* outBufferPtr1 = outArgs.data;
  u8* outBufferPtr2 = outBufferPtr1 + outRowSkip;

  std::array<uint8x8x4_t,4> gammaLUT;
  for (int i = 0; i < gammaLUT.size(); ++i){
    for (int j = 0; j < 4; ++j){
      gammaLUT[i].val[j] = vld1_u8(_gammaLUT.data()+8*(i*4+j));
    }
  }
  uint8x8_t value_32 = vdup_n_u8(32);

  auto extract = [&](uint8x8_t aa, uint8x8_t bb) -> uint8x8_t {
    uint8x8_t buffer = vext_u8(vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(aa), 32)), bb, 4);
    buffer = vshr_n_u8(buffer, 1);

    uint8x8_t output = vtbl4_u8(gammaLUT[0], buffer);
    for (int i = 1; i < gammaLUT.size(); ++i)
    {
      buffer = vsub_u8(buffer, value_32);
      output = vadd_u8(output, vtbl4_u8(gammaLUT[i], buffer));
    }
    return output;
  };

  u8 map_low[8] =  { 0, 0, 1, 1, 2, 2, 3, 3};
  u8 map_high[8] = { 4, 4, 5, 5, 6, 6, 7, 7};
  uint8x8_t mapL = vld1_u8(map_low);
  uint8x8_t mapH = vld1_u8(map_high);

  for (u32 row = 0; row < outArgs.height; row += rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += 2*colStep) // we're doing 2 blocks of 8 (16 columns and 2 rows)
    {
      uint8x8_t aa,bb;
      aa = vld1_u8(inBufferPtr1);
      inBufferPtr1 += inColSkip;
      bb = vld1_u8(inBufferPtr1);
      inBufferPtr1 += inColSkip;

      uint8x8_t buffer1 = extract(aa, bb);

      aa = vld1_u8(inBufferPtr1);
      inBufferPtr1 += inColSkip;
      bb = vld1_u8(inBufferPtr1);
      inBufferPtr1 += inColSkip;

      uint8x8_t buffer2 = extract(aa, bb);

      aa = vld1_u8(inBufferPtr2);
      inBufferPtr2 += inColSkip;
      bb = vld1_u8(inBufferPtr2);
      inBufferPtr2 += inColSkip;

      uint8x8_t buffer3 = extract(aa, bb);

      aa = vld1_u8(inBufferPtr2);
      inBufferPtr2 += inColSkip;
      bb = vld1_u8(inBufferPtr2);
      inBufferPtr2 += inColSkip;

      uint8x8_t buffer4 = extract(aa, bb);

      uint8x8x2_t unzipped1 = vuzp_u8(buffer1, buffer2);
      uint8x8x2_t unzipped2 = vuzp_u8(buffer3, buffer4);

      buffer1 = unzipped1.val[0]; // red
      buffer2 = unzipped1.val[1]; // green
      buffer3 = unzipped2.val[0]; // green
      buffer4 = unzipped2.val[1]; // blue

      uint8x8x3_t rgb;
      rgb.val[0] = vtbl1_u8(buffer1, mapL);
      rgb.val[1] = vtbl1_u8(buffer2, mapL);
      rgb.val[2] = vtbl1_u8(buffer4, mapL);

      vst3_u8(outBufferPtr1, rgb);

      rgb.val[1] = vtbl1_u8(buffer3, mapL);

      vst3_u8(outBufferPtr2, rgb);

      outBufferPtr1 += outColSkip;
      outBufferPtr2 += outColSkip;

      rgb.val[0] = vtbl1_u8(buffer1, mapH);
      rgb.val[1] = vtbl1_u8(buffer2, mapH);
      rgb.val[2] = vtbl1_u8(buffer4, mapH);

      vst3_u8(outBufferPtr1, rgb);

      rgb.val[1] = vtbl1_u8(buffer3, mapH);

      vst3_u8(outBufferPtr2, rgb);

      outBufferPtr1 += outColSkip;
      outBufferPtr2 += outColSkip;
    }

    inBufferPtr1 += inRowSkip;
    inBufferPtr2 += inRowSkip;
    outBufferPtr1 += outRowSkip;
    outBufferPtr2 += outRowSkip;
  }
  return RESULT_OK; // SUCCESS
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

RAW10toY8::RAW10toY8() : Op()
{
  // Setup a gamma table for 128 (2^7) possible input values
  for (int i = 0; i < _gammaLUT.size(); ++i){
    _gammaLUT[i] = 255 * std::powf((f32)i/127.0f, Debayer::GAMMA);
  }
}

Result RAW10toY8::operator()(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  u8 outChannels = 1;
  u8 inChannels = 1;

  u8 sampleRate = 1;
  if (!Debayer::SampleRateFromScale(outArgs.scale, sampleRate))
  {
    return RESULT_FAIL;
  }

  // How many columns bytes to iterate over in the output image
  s32 rowStep = 2;
  s32 colStep = 8;

  u32 inColSkip = 5 * sampleRate;
  u32 outColSkip = colStep * outChannels;
  
  // Skip the rest of the bytes in a row and move down an extra rows;
  u32 inRowSkip = (2*sampleRate-1)*(inChannels * (inArgs.width * 5)/4);
  u32 outRowSkip = outChannels * outArgs.width;

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + inRowSkip;

  u8* outBufferPtr1 = outArgs.data;
  u8* outBufferPtr2 = outBufferPtr1 + outRowSkip;

  std::array<uint8x8x4_t,4> gammaLUT;
  for (int i = 0; i < gammaLUT.size(); ++i){
    for (int j = 0; j < 4; ++j){
      gammaLUT[i].val[j] = vld1_u8(_gammaLUT.data()+8*(i*4+j));
    }
  }
  uint8x8_t value_32 = vdup_n_u8(32);

  auto extract = [&](uint8x8_t aa, uint8x8_t bb) -> uint8x8_t {
    uint8x8_t buffer = vext_u8(vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(aa), 32)), bb, 4);
    buffer = vshr_n_u8(buffer, 1);

    uint8x8_t output = vtbl4_u8(gammaLUT[0], buffer);
    for (int i = 1; i < gammaLUT.size(); ++i)
    {
      buffer = vsub_u8(buffer, value_32);
      output = vadd_u8(output, vtbl4_u8(gammaLUT[i], buffer));
    }
    return output;
  };

  u8 map_low[8] =  { 0, 0, 1, 1, 2, 2, 3, 3};
  u8 map_high[8] = { 4, 4, 5, 5, 6, 6, 7, 7};
  uint8x8_t mapL = vld1_u8(map_low);
  uint8x8_t mapH = vld1_u8(map_high);

  for (u32 row = 0; row < outArgs.height; row += rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += 2*colStep) // we're doing 2 blocks of 8 (16 columns and 2 rows)
    {
      uint8x8_t aa,bb;
      aa = vld1_u8(inBufferPtr1);
      inBufferPtr1 += inColSkip;
      bb = vld1_u8(inBufferPtr1);
      inBufferPtr1 += inColSkip;

      uint8x8_t buffer1 = extract(aa, bb);

      aa = vld1_u8(inBufferPtr1);
      inBufferPtr1 += inColSkip;
      bb = vld1_u8(inBufferPtr1);
      inBufferPtr1 += inColSkip;

      uint8x8_t buffer2 = extract(aa, bb);

      aa = vld1_u8(inBufferPtr2);
      inBufferPtr2 += inColSkip;
      bb = vld1_u8(inBufferPtr2);
      inBufferPtr2 += inColSkip;

      uint8x8_t buffer3 = extract(aa, bb);

      aa = vld1_u8(inBufferPtr2);
      inBufferPtr2 += inColSkip;
      bb = vld1_u8(inBufferPtr2);
      inBufferPtr2 += inColSkip;

      uint8x8_t buffer4 = extract(aa, bb);

      uint8x8x2_t unzipped1 = vuzp_u8(buffer1, buffer2);
      uint8x8x2_t unzipped2 = vuzp_u8(buffer3, buffer4);

      buffer1 = unzipped1.val[0]; // red
      buffer2 = unzipped1.val[1]; // green
      buffer3 = unzipped2.val[0]; // green
      buffer4 = unzipped2.val[1]; // blue

      uint8x8_t grey;
      grey = vtbl1_u8(buffer2, mapL);
      vst1_u8(outBufferPtr1, grey);
      outBufferPtr1 += outColSkip;

      grey = vtbl1_u8(buffer2, mapH);
      vst1_u8(outBufferPtr1, grey);
      outBufferPtr1 += outColSkip;

      grey = vtbl1_u8(buffer3, mapL);
      vst1_u8(outBufferPtr2, grey);
      outBufferPtr2 += outColSkip;

      grey = vtbl1_u8(buffer3, mapH);
      vst1_u8(outBufferPtr2, grey);
      outBufferPtr2 += outColSkip;
    }

    inBufferPtr1 += inRowSkip;
    inBufferPtr2 += inRowSkip;
    outBufferPtr1 += outRowSkip;
    outBufferPtr2 += outRowSkip;
  }
  return RESULT_OK; // SUCCESS
}

} /* namespace Neon */
} /* namespace Vision */
} /* namespace Anki */

#endif /* __ARM_NEON__ */