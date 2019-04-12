#include "raw10.h"

#include <cmath>

namespace {
/**
 * @brief Convert a block of RGGB8 values into RGB24
 */
inline void RGGBtoRGB(const std::array<u8,8>& values, u8* outBufferPtr1, u8* outBufferPtr2)
{
  // RGGB8 -> RGB format
  outBufferPtr1[0] = values[0];
  outBufferPtr1[1] = values[1];
  outBufferPtr1[2] = values[5];

  outBufferPtr1[3] = values[0];
  outBufferPtr1[4] = values[1];
  outBufferPtr1[5] = values[5]; 

  outBufferPtr1[6] = values[2];
  outBufferPtr1[7] = values[3];
  outBufferPtr1[8] = values[7];

  outBufferPtr1[9] = values[2];
  outBufferPtr1[10] = values[3];
  outBufferPtr1[11] = values[7];

  outBufferPtr2[0] = values[0];
  outBufferPtr2[1] = values[4];
  outBufferPtr2[2] = values[5];

  outBufferPtr2[3] = values[0];
  outBufferPtr2[4] = values[4];
  outBufferPtr2[5] = values[5]; 

  outBufferPtr2[6] = values[2];
  outBufferPtr2[7] = values[6];
  outBufferPtr2[8] = values[7];

  outBufferPtr2[9] = values[2];
  outBufferPtr2[10] = values[6];
  outBufferPtr2[11] = values[7];
}

/**
 * @brief Convert a block of RGGB8 values to Y8
 */
inline void RGGBtoY(const std::array<u8,8>& values, u8* outBufferPtr1, u8* outBufferPtr2)
{
  // RGGB8 -> Y8 format
  outBufferPtr1[0] = values[1];
  outBufferPtr1[1] = values[1]; 
  outBufferPtr1[2] = values[3];
  outBufferPtr1[3] = values[3];

  outBufferPtr2[0] = values[4];
  outBufferPtr2[1] = values[4];
  outBufferPtr2[2] = values[6];
  outBufferPtr2[3] = values[6];
}

} /* anonymous namespace */

namespace Anki {
namespace Vision {

RAW10toRGB24::RAW10toRGB24() : Op()
{
  // Setup a gamma table for 1024 (2^10) possible input values
  for (int ii = 0; ii < _gammaLUT.size(); ++ii)
  {
    _gammaLUT[ii] = 255 * std::powf(ii/1023.0f, Debayer::GAMMA);
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
  s32 colStep = 4;

  u32 inColSkip = 5 * sampleRate;
  u32 outColSkip = colStep * outChannels;

  // Skip the rest of the bytes in a row and move down an extra rows;
  u32 inRowSkip = (2*sampleRate-1)*(inChannels * (inArgs.width * 5)/4);
  u32 outRowSkip = outChannels * outArgs.width;

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + inRowSkip;

  u8* outBufferPtr1 = outArgs.data;
  u8* outBufferPtr2 = outBufferPtr1 + outRowSkip;

  std::array<u8, 8> values;

  for (u32 row = 0; row < outArgs.height; row += rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += colStep)
    {
      // Extract RAW10 RGGB10 rows and convert to Gamma Corrected eight RGGB8 values
      values[0] = _gammaLUT[(inBufferPtr1[0] << 2) | ((inBufferPtr1[4] & 0x03)     )];
      values[1] = _gammaLUT[(inBufferPtr1[1] << 2) | ((inBufferPtr1[4] & 0x0C) >> 2)];
      values[2] = _gammaLUT[(inBufferPtr1[2] << 2) | ((inBufferPtr1[4] & 0x30) >> 4)];
      values[3] = _gammaLUT[(inBufferPtr1[3] << 2) | ((inBufferPtr1[4] & 0xC0) >> 6)];
      values[4] = _gammaLUT[(inBufferPtr2[0] << 2) | ((inBufferPtr2[4] & 0x03)     )];
      values[5] = _gammaLUT[(inBufferPtr2[1] << 2) | ((inBufferPtr2[4] & 0x0C) >> 2)];
      values[6] = _gammaLUT[(inBufferPtr2[2] << 2) | ((inBufferPtr2[4] & 0x30) >> 4)];
      values[7] = _gammaLUT[(inBufferPtr2[3] << 2) | ((inBufferPtr2[4] & 0xC0) >> 6)];

      RGGBtoRGB(values, outBufferPtr1, outBufferPtr2);

      // Move the pointer forward
      inBufferPtr1 += inColSkip;
      inBufferPtr2 += inColSkip;
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

RAW10toY8::RAW10toY8() : Op()
{
  // Setup a gamma table for 1024 (2^10) possible input values
  for (int ii = 0; ii < _gammaLUT.size(); ++ii)
  {
    _gammaLUT[ii] = 255 * std::powf(ii/1023.0f, Debayer::GAMMA);
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
  s32 colStep = 4;

  u32 inColSkip = 5 * sampleRate;
  u32 outColSkip = colStep * outChannels;

  // Skip the rest of the bytes in a row and move down an extra rows;
  u32 inRowSkip = (2*sampleRate-1)*(inChannels * (inArgs.width * 5)/4);
  u32 outRowSkip = outChannels * outArgs.width;

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + inRowSkip;

  u8* outBufferPtr1 = outArgs.data;
  u8* outBufferPtr2 = outBufferPtr1 + outRowSkip;

  std::array<u8, 8> values;

  for (u32 row = 0; row < outArgs.height; row += rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += colStep)
    {
      // Extract RAW10 RGGB10 rows and convert to Gamma Corrected eight RGGB8 values
      values[0] = _gammaLUT[(inBufferPtr1[0] << 2) | ((inBufferPtr1[4] & 0x03)     )];
      values[1] = _gammaLUT[(inBufferPtr1[1] << 2) | ((inBufferPtr1[4] & 0x0C) >> 2)];
      values[2] = _gammaLUT[(inBufferPtr1[2] << 2) | ((inBufferPtr1[4] & 0x30) >> 4)];
      values[3] = _gammaLUT[(inBufferPtr1[3] << 2) | ((inBufferPtr1[4] & 0xC0) >> 6)];
      values[4] = _gammaLUT[(inBufferPtr2[0] << 2) | ((inBufferPtr2[4] & 0x03)     )];
      values[5] = _gammaLUT[(inBufferPtr2[1] << 2) | ((inBufferPtr2[4] & 0x0C) >> 2)];
      values[6] = _gammaLUT[(inBufferPtr2[2] << 2) | ((inBufferPtr2[4] & 0x30) >> 4)];
      values[7] = _gammaLUT[(inBufferPtr2[3] << 2) | ((inBufferPtr2[4] & 0xC0) >> 6)];

      // Convert the squashed values to Y8
      RGGBtoY(values, outBufferPtr1, outBufferPtr2);

      // Move the pointer forward
      inBufferPtr1 += inColSkip;
      inBufferPtr2 += inColSkip;
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

} /* namespace Vision */
} /* namespace Anki */