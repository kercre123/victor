/**
 * File: raw10.cpp
 *
 * Author: Patrick Doran
 * Date: 03/25/2019
 *
 * Description: Debayering for RAW10 on Vector. This contains Debayer::Op instances that run on CPU
 * 
 * Copyright: Anki, Inc. 2019
 */
#include "coretech/vision/engine/debayer/raw10.h"

#include "util/logging/logging.h"

#include <cmath>
#include <exception>
#include <vector>
#include <functional>

namespace Anki {
namespace Vision {

namespace 
{

struct SetupInfo
{
  SetupInfo(const Debayer::InArgs& inArgs, const Debayer::OutArgs& outArgs);

  bool valid;                 //! Whether the setup information is usable. If not, there was an error.
  u32 rowStep;                //! Steps for the for loop
  u32 colStep;                //! Steps for the for loop
  u32 inColSkip;              //! Number of bytes in the input to skip per extraction
  u32 inRowSkip;              //! Number of bytes in the input to skip per iteration of output row
  u32 outColSkip;             //! Number of bytes in the output to skip per storing
  u32 outRowSkip;             //! Number of bytes in the output to skip to next storage row at the same column
  u32 outChannels;            //! Number of channels in the output image (e.g. RGB24 = 3, Y8 = 1)
};


SetupInfo::SetupInfo(const Debayer::InArgs& inArgs, const Debayer::OutArgs& outArgs)
  : valid(false)
{

  this->outChannels = 3;
  switch(outArgs.format)
  {
    case Debayer::OutputFormat::RGB24:
    {
      this->outChannels = 3;
      break;
    }
    case Debayer::OutputFormat::Y8:
    {
      this->outChannels = 1;
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("Debayer.RAW10.SetupInfo","Unknown output format %d",outArgs.format);
      return;
    }
  }

  // Since this a bayer format, we are always reading 2x2 blocks so always create 2xN output pixels where N depends
  // on the scale

  // RAW10 means their are 5 bytes for every 4 pixels of a row. Hence, there are W*5/4 bytes per row.
  u32 inBytesPerRow = (inArgs.width*5)/4;

  switch(outArgs.scale)
  {
  case Debayer::Scale::FULL:
  { 
    this->inColSkip = 5;

    // Skip one row, it's bayer format and we want to get to the next R/G or G/B (use every block)
    this->inRowSkip = inBytesPerRow;

    // We create 2x4 output pixels per iteration
    this->rowStep = 2;
    this->colStep = 4;

    this->outColSkip = this->outChannels * this->colStep;
    this->outRowSkip = this->outChannels * outArgs.width;
    break;
  }
  case Debayer::Scale::HALF:
  { 
    this->inColSkip = 5;

    // Skip one row, it's bayer format and we want to get to the next R/G or G/B (use every block)
    this->inRowSkip = inBytesPerRow;


    // We create 1x2 output pixels per iteration
    this->rowStep = 1;
    this->colStep = 2;

    // We're automatically at the next row during iteration using outColSkip
    this->outColSkip = this->outChannels * this->colStep;
    this->outRowSkip = 0; 
    break;
  }
  case Debayer::Scale::QUARTER:
  {
    this->inColSkip = 5;

    // Skip one row to get to the next block, then skip 2 rows (to move down two blocks total)
    this->inRowSkip = 3 * inBytesPerRow;

    // We create 1x1 output pixels per iteration
    this->rowStep = 1;
    this->colStep = 1;

    // We're automatically at the next row during iteration using outColSkip
    this->outColSkip = this->outChannels * this->colStep;
    this->outRowSkip = 0; 
    break;
  }
  case Debayer::Scale::EIGHTH:
  {
    this->inColSkip = 10;

    // Skip one row to get to the next block, then skip 6 rows (to move down four blocks total)
    this->inRowSkip = 7 * inBytesPerRow;

    // We create 1x1 output pixels per iteration
    this->rowStep = 1;
    this->colStep = 1;

    // We're automatically at the next row during iteration using outColSkip
    this->outColSkip = this->outChannels * this->colStep;
    this->outRowSkip = 0; 
    break;
  }
  default:
  {
      PRINT_NAMED_ERROR("Debayer.RAW10.SetupInfo","Unknown scale %d", outArgs.scale);
      this->valid = false;
      return;
  }
  }
  this->valid = true;
}

} /* anomymous namespace */

/**
 * @brief Convenience macro to shorten the function adding line
 */
#define ADD_FUNC(funcs, scale, format, function) { \
  funcs[MakeKey(Debayer::Scale::scale, Debayer::OutputFormat::format)] = \
    std::bind(&HandleRAW10::function, this, std::placeholders::_1, std::placeholders::_2); \
}

HandleRAW10::HandleRAW10(f32 gamma) : Op()
{
  for (int i = 0; i < _gammaLUT.size(); ++i){
    _gammaLUT[i] = 255 * std::powf((f32)i/1023.0f, gamma);
  }

  ADD_FUNC(_functions,    FULL, RGB24, RAW10_to_RGB24_FULL);
  ADD_FUNC(_functions,    HALF, RGB24, RAW10_to_RGB24_HALF);
  ADD_FUNC(_functions, QUARTER, RGB24, RAW10_to_RGB24_QUARTER_or_EIGHTH);
  ADD_FUNC(_functions,  EIGHTH, RGB24, RAW10_to_RGB24_QUARTER_or_EIGHTH);
  ADD_FUNC(_functions,    FULL,    Y8, RAW10_to_Y8_FULL);
  ADD_FUNC(_functions,    HALF,    Y8, RAW10_to_Y8_HALF);
  ADD_FUNC(_functions, QUARTER,    Y8, RAW10_to_Y8_QUARTER_or_EIGHTH);
  ADD_FUNC(_functions,  EIGHTH,    Y8, RAW10_to_Y8_QUARTER_or_EIGHTH);
}

Result HandleRAW10::operator()(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  auto iter = _functions.find(MakeKey(outArgs.scale, outArgs.format));
  if (iter == _functions.end())
  {
    return RESULT_FAIL;
  }
  else
  {
    return iter->second(inArgs, outArgs);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_RGB24_FULL(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);
  if (!setup.valid)
  {
    return RESULT_FAIL;
  }

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;

  u8* outBufferPtr1 = outArgs.data;
  u8* outBufferPtr2 = outBufferPtr1 + setup.outRowSkip;

  std::array<u8, 8> values;

  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      values[0] = _gammaLUT[(inBufferPtr1[0] << 2) | ((inBufferPtr1[4] & 0x03)     )];
      values[1] = _gammaLUT[(inBufferPtr1[1] << 2) | ((inBufferPtr1[4] & 0x0C) >> 2)];
      values[2] = _gammaLUT[(inBufferPtr1[2] << 2) | ((inBufferPtr1[4] & 0x30) >> 4)];
      values[3] = _gammaLUT[(inBufferPtr1[3] << 2) | ((inBufferPtr1[4] & 0xC0) >> 6)];
      values[4] = _gammaLUT[(inBufferPtr2[0] << 2) | ((inBufferPtr2[4] & 0x03)     )];
      values[5] = _gammaLUT[(inBufferPtr2[1] << 2) | ((inBufferPtr2[4] & 0x0C) >> 2)];
      values[6] = _gammaLUT[(inBufferPtr2[2] << 2) | ((inBufferPtr2[4] & 0x30) >> 4)];
      values[7] = _gammaLUT[(inBufferPtr2[3] << 2) | ((inBufferPtr2[4] & 0xC0) >> 6)];

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

      inBufferPtr1 += setup.inColSkip;
      inBufferPtr2 += setup.inColSkip;
      outBufferPtr1 += setup.outColSkip;
      outBufferPtr2 += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;
    outBufferPtr1 += setup.outRowSkip;
    outBufferPtr2 += setup.outRowSkip;
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_RGB24_HALF(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);
  if (!setup.valid)
  {
    return RESULT_FAIL;
  }

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;

  u8* outBufferPtr = outArgs.data;

  std::array<u8, 8> values;

  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      values[0] = _gammaLUT[(inBufferPtr1[0] << 2) | ((inBufferPtr1[4] & 0x03)     )];
      values[1] = _gammaLUT[(inBufferPtr1[1] << 2) | ((inBufferPtr1[4] & 0x0C) >> 2)];
      values[2] = _gammaLUT[(inBufferPtr1[2] << 2) | ((inBufferPtr1[4] & 0x30) >> 4)];
      values[3] = _gammaLUT[(inBufferPtr1[3] << 2) | ((inBufferPtr1[4] & 0xC0) >> 6)];
      values[4] = _gammaLUT[(inBufferPtr2[0] << 2) | ((inBufferPtr2[4] & 0x03)     )];
      values[5] = _gammaLUT[(inBufferPtr2[1] << 2) | ((inBufferPtr2[4] & 0x0C) >> 2)];
      values[6] = _gammaLUT[(inBufferPtr2[2] << 2) | ((inBufferPtr2[4] & 0x30) >> 4)];
      values[7] = _gammaLUT[(inBufferPtr2[3] << 2) | ((inBufferPtr2[4] & 0xC0) >> 6)];

      outBufferPtr[0] = values[0];
      outBufferPtr[1] = (values[1] + values[4])/2;
      outBufferPtr[2] = values[5]; 

      outBufferPtr[3] = values[2];
      outBufferPtr[4] = (values[3] + values[6])/2;
      outBufferPtr[5] = values[7];

      inBufferPtr1 += setup.inColSkip;
      inBufferPtr2 += setup.inColSkip;
      outBufferPtr += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;

    // NOTE: Don't need to skip any output rows, we're iterating over all of them.
  }
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_RGB24_QUARTER_or_EIGHTH(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;

  u8* outBufferPtr = outArgs.data;

  std::array<u8, 8> values;

  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      values[0] = _gammaLUT[(inBufferPtr1[0] << 2) | ((inBufferPtr1[4] & 0x03)     )];
      values[1] = _gammaLUT[(inBufferPtr1[1] << 2) | ((inBufferPtr1[4] & 0x0C) >> 2)];
      // values[2] = _gammaLUT[(inBufferPtr1[2] << 2) | ((inBufferPtr1[4] & 0x30) >> 4)];
      // values[3] = _gammaLUT[(inBufferPtr1[3] << 2) | ((inBufferPtr1[4] & 0xC0) >> 6)];
      values[4] = _gammaLUT[(inBufferPtr2[0] << 2) | ((inBufferPtr2[4] & 0x03)     )];
      values[5] = _gammaLUT[(inBufferPtr2[1] << 2) | ((inBufferPtr2[4] & 0x0C) >> 2)];
      // values[6] = _gammaLUT[(inBufferPtr2[2] << 2) | ((inBufferPtr2[4] & 0x30) >> 4)];
      // values[7] = _gammaLUT[(inBufferPtr2[3] << 2) | ((inBufferPtr2[4] & 0xC0) >> 6)];

      outBufferPtr[0] = values[0];
      outBufferPtr[1] = (values[1] + values[4])/2;
      outBufferPtr[2] = values[5]; 

      inBufferPtr1 += setup.inColSkip;
      inBufferPtr2 += setup.inColSkip;
      outBufferPtr += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;

    // NOTE: Don't need to skip any output rows, we're iterating over all of them.
  }
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_Y8_FULL(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);
  if (!setup.valid)
  {
    return RESULT_FAIL;
  }

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;

  u8* outBufferPtr1 = outArgs.data;
  u8* outBufferPtr2 = outBufferPtr1 + setup.outRowSkip;

  std::array<u8, 8> values;

  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      // values[0] = _gammaLUT[(inBufferPtr1[0] << 2) | ((inBufferPtr1[4] & 0x03)     )];
      values[1] = _gammaLUT[(inBufferPtr1[1] << 2) | ((inBufferPtr1[4] & 0x0C) >> 2)];
      // values[2] = _gammaLUT[(inBufferPtr1[2] << 2) | ((inBufferPtr1[4] & 0x30) >> 4)];
      values[3] = _gammaLUT[(inBufferPtr1[3] << 2) | ((inBufferPtr1[4] & 0xC0) >> 6)];
      values[4] = _gammaLUT[(inBufferPtr2[0] << 2) | ((inBufferPtr2[4] & 0x03)     )];
      // values[5] = _gammaLUT[(inBufferPtr2[1] << 2) | ((inBufferPtr2[4] & 0x0C) >> 2)];
      values[6] = _gammaLUT[(inBufferPtr2[2] << 2) | ((inBufferPtr2[4] & 0x30) >> 4)];
      // values[7] = _gammaLUT[(inBufferPtr2[3] << 2) | ((inBufferPtr2[4] & 0xC0) >> 6)];

      outBufferPtr1[0] = values[1];
      outBufferPtr1[1] = values[1]; 
      outBufferPtr1[2] = values[3];
      outBufferPtr1[3] = values[3];

      outBufferPtr2[0] = values[4];
      outBufferPtr2[1] = values[4];
      outBufferPtr2[2] = values[6];
      outBufferPtr2[3] = values[6];

      inBufferPtr1 += setup.inColSkip;
      inBufferPtr2 += setup.inColSkip;
      outBufferPtr1 += setup.outColSkip;
      outBufferPtr2 += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;
    outBufferPtr1 += setup.outRowSkip;
    outBufferPtr2 += setup.outRowSkip;
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_Y8_HALF(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);
  if (!setup.valid)
  {
    return RESULT_FAIL;
  }

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;

  u8* outBufferPtr = outArgs.data;

  std::array<u8, 8> values;

  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      // values[0] = _gammaLUT[(inBufferPtr1[0] << 2) | ((inBufferPtr1[4] & 0x03)     )];
      values[1] = _gammaLUT[(inBufferPtr1[1] << 2) | ((inBufferPtr1[4] & 0x0C) >> 2)];
      // values[2] = _gammaLUT[(inBufferPtr1[2] << 2) | ((inBufferPtr1[4] & 0x30) >> 4)];
      values[3] = _gammaLUT[(inBufferPtr1[3] << 2) | ((inBufferPtr1[4] & 0xC0) >> 6)];
      values[4] = _gammaLUT[(inBufferPtr2[0] << 2) | ((inBufferPtr2[4] & 0x03)     )];
      // values[5] = _gammaLUT[(inBufferPtr2[1] << 2) | ((inBufferPtr2[4] & 0x0C) >> 2)];
      values[6] = _gammaLUT[(inBufferPtr2[2] << 2) | ((inBufferPtr2[4] & 0x30) >> 4)];
      // values[7] = _gammaLUT[(inBufferPtr2[3] << 2) | ((inBufferPtr2[4] & 0xC0) >> 6)];

      outBufferPtr[0] = (values[1] + values[4])/2;

      outBufferPtr[1] = (values[3] + values[6])/2;

      inBufferPtr1 += setup.inColSkip;
      inBufferPtr2 += setup.inColSkip;
      outBufferPtr += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;

    // NOTE: Don't need to skip any output rows, we're iterating over all of them.
  }
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_Y8_QUARTER_or_EIGHTH(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);
  if (!setup.valid)
  {
    return RESULT_FAIL;
  }

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;

  u8* outBufferPtr = outArgs.data;

  std::array<u8, 8> values;

  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      // values[0] = _gammaLUT[(inBufferPtr1[0] << 2) | ((inBufferPtr1[4] & 0x03)     )];
      values[1] = _gammaLUT[(inBufferPtr1[1] << 2) | ((inBufferPtr1[4] & 0x0C) >> 2)];
      // values[2] = _gammaLUT[(inBufferPtr1[2] << 2) | ((inBufferPtr1[4] & 0x30) >> 4)];
      // values[3] = _gammaLUT[(inBufferPtr1[3] << 2) | ((inBufferPtr1[4] & 0xC0) >> 6)];
      values[4] = _gammaLUT[(inBufferPtr2[0] << 2) | ((inBufferPtr2[4] & 0x03)     )];
      // values[5] = _gammaLUT[(inBufferPtr2[1] << 2) | ((inBufferPtr2[4] & 0x0C) >> 2)];
      // values[6] = _gammaLUT[(inBufferPtr2[2] << 2) | ((inBufferPtr2[4] & 0x30) >> 4)];
      // values[7] = _gammaLUT[(inBufferPtr2[3] << 2) | ((inBufferPtr2[4] & 0xC0) >> 6)];

      outBufferPtr[0] = (values[1] + values[4])/2;

      inBufferPtr1 += setup.inColSkip;
      inBufferPtr2 += setup.inColSkip;
      outBufferPtr += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;

    // NOTE: Don't need to skip any output rows, we're iterating over all of them.
  }
  return RESULT_OK;
}

} /* namespace Vision */
} /* namespace Anki */