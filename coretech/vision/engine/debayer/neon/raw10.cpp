/**
 * File: neon/raw10.cpp
 *
 * Author: Patrick Doran
 * Date: 03/25/2019
 *
 * Description: Debayering for RAW10 on Vector. This contains Debayer::Op instances that run on CPU using Neon
 *              acceleration.
 *
 * Copyright: Anki, Inc. 2019
 */

#ifdef __ARM_NEON__

#include "coretech/vision/engine/debayer/neon/raw10.h"

#include "util/logging/logging.h"

#include <arm_neon.h>
#include <cmath>
#include <exception>
#include <vector>
#include <functional>

/**
 * @brief Set to 1 to turn on green channel averaging, which only applies to downscaled images. Set to 0 otherwise.
 * @details Setting this to 1 turns on green channel averaging which only applies to downscaled images. Full size
 * images use all input values. Averaging green adds a not insignificant cost to debayering (2-4ms for half size, 
 * 640x360). It's faster to just the use one of the green channels directly and ignore the other channel. This will
 * reduce the quality of the output.
 */
#define DO_GREEN_AVG 0

namespace Anki {
namespace Vision {
namespace Neon {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Anonymous namespace for helper functions and classes
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

namespace {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * @brief Information necessary for storing the images
 */
struct StoreInfo
{
  std::array<uint8x8x4_t,4> gammaLUT;
  uint8x8_t value_32;
  uint8x8_t mapL;
  uint8x8_t mapH;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct SetupInfo
{
  SetupInfo(const Debayer::InArgs& inArgs, const Debayer::OutArgs& outArgs);

  bool valid;                 //! Whether the setup information is usable. If not, there was an error.
  std::array<u8,8> indexes;   //! Index table for extracting inBufferPtr data 
  std::array<u8,4> offsets;   //! Offsets from the inBufferPtr to load bytes into the table. Unused values are UINT8_MAX
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
      PRINT_NAMED_ERROR("Debayer.Neon.RAW10.SetupInfo","Unknown output format: %d", outArgs.format);
      return;
  }

  // Since this a bayer format, we are always reading 2x2 blocks so always create 2xN output pixels where N depends
  // on the scale

  // Scale Comment Legend:
  // Data - The input bytes from the source image. The values in the comment are the offset from the current input 
  //      image pointer. The values in the vector are the byte values of the source image.
  // Table - The input bytes will be loaded into a uint8x8xN_t. The values in the comment are the Data index values and 
  //      where they will be in the table. Anything that is "xx" is a value we don't actually care about. The values in 
  //      those locations will be the sequential bytes following the ones that are not "xx". The values in the vector
  //      are the byte values of the source image.
  // Mapping - The indexes into the table from which to create a new vector. Look up VTBL and vtblN_u8 instructions
  //      for a complete understanding of how to use this. The values in the comment and the vector are the same; they
  //      are indexes into the table.
  // Extract - The resulting uint8x8_t vector from doing the table lookup. The values in the comment are the indexes
  //      from source image. The values in the vector will be the byte values of the source image.

  // RAW10 means their are 5 bytes for every 4 pixels of a row. Hence, there are W*5/4 bytes per row.
  u32 inBytesPerRow = (inArgs.width*5)/4;
  
  switch(outArgs.scale)
  {
  case Debayer::Scale::FULL:
  {
    // Data:          [ 00 01 02 03 04 05 06 07 08 09 10 11 12 ]
    // Table.val[0]:  [ 00 01 02 03 xx xx xx xx ]
    // Table.val[1]:  [ 05 06 07 08 xx xx xx xx ]
    // Mapping:       [  0  1  2  3  8  9 10 11 ] 
    // Extract:       [ 00 01 02 03 05 06 07 08 ]
    this->indexes[0] = 0;
    this->indexes[1] = 1;
    this->indexes[2] = 2;
    this->indexes[3] = 3;
    this->indexes[4] = 8;
    this->indexes[5] = 9;
    this->indexes[6] = 10;
    this->indexes[7] = 11;

    this->offsets[0] = 0;
    this->offsets[1] = 5;
    this->offsets[2] = std::numeric_limits<u8>::max(); // Invalid
    this->offsets[3] = std::numeric_limits<u8>::max(); // Invalid
    
    this->inColSkip = 10;

    // Skip one row, it's bayer format and we want to get to the next R/G or G/B (use every block)
    this->inRowSkip = inBytesPerRow;

    // We create 2x16 output pixels per iteration
    this->rowStep = 2;
    this->colStep = 16;

    this->outColSkip = this->outChannels * this->colStep;
    this->outRowSkip = this->outChannels * outArgs.width;
    break;
  }
  case Debayer::Scale::HALF:
  {
    // Data:          [ 00 01 02 03 04 05 06 07 08 09 10 11 12 ]
    // Table.val[0]:  [ 00 01 02 03 xx xx xx xx ]
    // Table.val[1]:  [ 05 06 07 08 xx xx xx xx ]
    // Mapping:       [  0  1  2  3  8  9 10 11 ] 
    // Extract:       [ 00 01 02 03 05 06 07 08 ]
    this->indexes[0] = 0;
    this->indexes[1] = 1;
    this->indexes[2] = 2;
    this->indexes[3] = 3;
    this->indexes[4] = 8;
    this->indexes[5] = 9;
    this->indexes[6] = 10;
    this->indexes[7] = 11;

    this->offsets[0] = 0;
    this->offsets[1] = 5;
    this->offsets[2] = std::numeric_limits<u8>::max(); // Invalid
    this->offsets[3] = std::numeric_limits<u8>::max(); // Invalid
    
    this->inColSkip = 10;

    // Skip one row, it's bayer format and we want to get to the next R/G or G/B (use every block)
    this->inRowSkip = inBytesPerRow;

    // We create 1x8 output pixels per iteration
    this->rowStep = 1;
    this->colStep = 8;

    // We're automatically at the next row during iteration using outColSkip
    this->outColSkip = this->outChannels * this->colStep;
    this->outRowSkip = 0; 
    break;
  }
  case Debayer::Scale::QUARTER:
  {
    // Data:          [ 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 ]
    // Table.val[0]:  [ 00 01 xx xx xx 05 06 xx ]
    // Table.val[1]:  [ 10 11 xx xx xx 15 16 xx ]
    // Mapping:       [  0  1  5  6  8  9 13 14 ] 
    // Extract:       [ 00 01 05 06 10 11 15 16 ]
    this->indexes[0] = 0;
    this->indexes[1] = 1;
    this->indexes[2] = 5;
    this->indexes[3] = 6;
    this->indexes[4] = 8;
    this->indexes[5] = 9;
    this->indexes[6] = 13;
    this->indexes[7] = 14;

    this->offsets[0] = 0;
    this->offsets[1] = 10;
    this->offsets[2] = std::numeric_limits<u8>::max(); // Invalid
    this->offsets[3] = std::numeric_limits<u8>::max(); // Invalid

    this->inColSkip = 20;

    // Skip one row to get to the next block, then skip 2 rows (to move down two blocks total)
    this->inRowSkip = 3 * inBytesPerRow;

    // We create 1x8 output pixels per iteration
    this->rowStep = 1;
    this->colStep = 8;

    // We're automatically at the next row during iteration using outColSkip
    this->outColSkip = this->outChannels * this->colStep;
    this->outRowSkip = 0; 
    break;
  }
  case Debayer::Scale::EIGHTH:
  {
    // Data:          [ 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 
    //                  16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 ]
    // Table.val[0]:  [ 00 01 xx xx xx xx xx xx ]
    // Table.val[1]:  [ 10 11 xx xx xx xx xx xx ]
    // Table.val[2]:  [ 20 21 xx xx xx xx xx xx ]
    // Table.val[3]:  [ 30 31 xx xx xx xx xx xx ]
    // Mapping:       [  0  1  8  9 16 17 24 25 ] 
    // Extract:       [ 00 01 10 11 20 21 30 31 ]
    this->indexes[0] = 0;
    this->indexes[1] = 1;
    this->indexes[2] = 8;
    this->indexes[3] = 9;
    this->indexes[4] = 16;
    this->indexes[5] = 17;
    this->indexes[6] = 24;
    this->indexes[7] = 25;

    this->offsets[0] = 0;
    this->offsets[1] = 10;
    this->offsets[2] = 20;
    this->offsets[3] = 30;

    this->inColSkip = 40;

    // Skip one row to get to the next block, then skip 6 rows (to move down four blocks total)
    this->inRowSkip = 7 * inBytesPerRow;

    // We create 1x8 output pixels per iteration
    this->rowStep = 1;
    this->colStep = 8;

    // We're automatically at the next row during iteration using outColSkip
    this->outColSkip = this->outChannels * this->colStep;
    this->outRowSkip = 0; 
    break;
  }
  default:
  {
      PRINT_NAMED_ERROR("Debayer.Neon.RAW10.SetupInfo","Unknown scale %d",outArgs.scale);
      return;
  }
  }
  this->valid = true;
}

inline void GammaCorrect(const std::array<uint8x8x4_t,4>& gammaLUT, const uint8x8_t& value_32, uint8x8_t& data)
{
  uint8x8_t buffer = vshr_n_u8(data,1);
  uint8x8_t output = vtbl4_u8(gammaLUT[0], buffer);
  for (int i = 1; i < gammaLUT.size(); ++i)
  {
    buffer = vsub_u8(buffer, value_32);
    output = vtbx4_u8(output, gammaLUT[i], buffer);
  }
  data = output;
}

} /* anonymous namespace */

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// End anonymous namespace
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * @brief Convenience macro to shorten the function adding line
 */
#define ADD_FUNC(funcs, scale, format, function) { \
  funcs[MakeKey(Debayer::Scale::scale, Debayer::OutputFormat::format)] = \
    std::bind(&HandleRAW10::function, this, std::placeholders::_1, std::placeholders::_2); \
}

HandleRAW10::HandleRAW10(f32 gamma) : Op()
{
  // Setup a gamma table for 128 (2^7) possible input values
  for (int i = 0; i < _gammaLUT.size(); ++i){
    _gammaLUT[i] = 255 * std::powf((f32)i/127.0f, gamma);
  }

  ADD_FUNC(_functions,    FULL, RGB24, RAW10_to_RGB24_FULL);
  ADD_FUNC(_functions,    HALF, RGB24, RAW10_to_RGB24_HALF_or_QUARTER);
  ADD_FUNC(_functions, QUARTER, RGB24, RAW10_to_RGB24_HALF_or_QUARTER);
  ADD_FUNC(_functions,  EIGHTH, RGB24, RAW10_to_RGB24_EIGHTH);
  ADD_FUNC(_functions,    FULL,    Y8, RAW10_to_Y8_FULL);
  ADD_FUNC(_functions,    HALF,    Y8, RAW10_to_Y8_HALF_or_QUARTER);
  ADD_FUNC(_functions, QUARTER,    Y8, RAW10_to_Y8_HALF_or_QUARTER);
  ADD_FUNC(_functions,  EIGHTH,    Y8, RAW10_to_Y8_EIGHTH);
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

  // The index to pull values from the table of loaded bytes
  const uint8x8_t index = vld1_u8(setup.indexes.data());

  StoreInfo store;
  for (int i = 0; i < store.gammaLUT.size(); ++i){
    for (int j = 0; j < 4; ++j){
      store.gammaLUT[i].val[j] = vld1_u8(_gammaLUT.data()+8*(i*4+j));
    }
  }
  store.value_32 = vdup_n_u8(32);

  // Map for creating full size images
  const u8 map_low[8] =  { 0, 0, 1, 1, 2, 2, 3, 3};
  const u8 map_high[8] = { 4, 4, 5, 5, 6, 6, 7, 7};
  
  store.mapL = vld1_u8(map_low);
  store.mapH = vld1_u8(map_high);

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;
  u8* outBufferPtr = outArgs.data;

  // Inner Loop variables; filled out during each iteration.
  uint8x8x2_t table;
  uint8x8x4_t block;
  uint8x8x2_t unzipped1;
  uint8x8x2_t unzipped2;
  uint8x8x3_t rgb;

  // Explanation of the iteration:
  //  - Neon allows us to look at and fill out 8 bytes of data
  //  - To get 8 bytes of Red, Blue, and Green, we need to look at 4 blocks of bayer data
  //  - We are either computing 2 rows of output data (FULL) or 1 row (HALF, QUARTER, EIGHTH)
  //  - We are either computing 16 cols of output data (FULL) or 8 cols (HALF, QUARTER, EIGHTH)
  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      // Load and Extract the bytes
      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      block.val[0] = vtbl2_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      block.val[1] = vtbl2_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      block.val[2] = vtbl2_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      block.val[3] = vtbl2_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      // Unzip the bytes so that each part of the block is eight consecutive R, G, G, B
      unzipped1 = vuzp_u8(block.val[0], block.val[1]);
      unzipped2 = vuzp_u8(block.val[2], block.val[3]);

      block.val[0] = unzipped1.val[0];
      block.val[1] = unzipped1.val[1];
      block.val[2] = unzipped2.val[0];
      block.val[3] = unzipped2.val[1];

      // Gamma Correct is the most expensive step timewise. Doing it as a helper function seems to have no effect on
      // the time to complete this step.
      GammaCorrect(store.gammaLUT, store.value_32, block.val[0]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[2]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[3]);
      
      // Store RGB24
      rgb.val[0] = vtbl1_u8(block.val[0], store.mapL);
      rgb.val[1] = vtbl1_u8(block.val[1], store.mapL);
      rgb.val[2] = vtbl1_u8(block.val[3], store.mapL);

      vst3_u8(outBufferPtr, rgb);

      rgb.val[1] = vtbl1_u8(block.val[2], store.mapL);

      vst3_u8(outBufferPtr + setup.outRowSkip, rgb);

      rgb.val[0] = vtbl1_u8(block.val[0], store.mapH);
      rgb.val[1] = vtbl1_u8(block.val[1], store.mapH);
      rgb.val[2] = vtbl1_u8(block.val[3], store.mapH);

      // Offset the pointer forward 24 bytes because we're storing the two sets of 8 pixels
      vst3_u8((outBufferPtr + 24), rgb);

      rgb.val[1] = vtbl1_u8(block.val[2], store.mapH);

      // Offset the pointer forward 24 bytes because we're storing the two sets of 8 pixels
      vst3_u8((outBufferPtr + 24 + setup.outRowSkip), rgb);

      outBufferPtr += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;
    outBufferPtr += setup.outRowSkip;
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_RGB24_HALF_or_QUARTER(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);
  if (!setup.valid)
  {
    return RESULT_FAIL;
  }

  // The index to pull values from the table of loaded bytes
  const uint8x8_t index = vld1_u8(setup.indexes.data());

  StoreInfo store;
  for (int i = 0; i < store.gammaLUT.size(); ++i){
    for (int j = 0; j < 4; ++j){
      store.gammaLUT[i].val[j] = vld1_u8(_gammaLUT.data()+8*(i*4+j));
    }
  }
  store.value_32 = vdup_n_u8(32);

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;
  u8* outBufferPtr = outArgs.data;

  // Inner Loop variables; filled out during each iteration.
  uint8x8x2_t table;
  uint8x8x4_t block;
  uint8x8x2_t unzipped1;
  uint8x8x2_t unzipped2;
  uint8x8x3_t rgb;

  // Explanation of the iteration:
  //  - Neon allows us to look at and fill out 8 bytes of data
  //  - To get 8 bytes of Red, Blue, and Green, we need to look at 4 blocks of bayer data
  //  - We are either computing 2 rows of output data (FULL) or 1 row (HALF, QUARTER, EIGHTH)
  //  - We are either computing 16 cols of output data (FULL) or 8 cols (HALF, QUARTER, EIGHTH)
  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      // Load and Extract the bytes
      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      block.val[0] = vtbl2_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      block.val[1] = vtbl2_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      block.val[2] = vtbl2_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      block.val[3] = vtbl2_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      // Unzip the bytes so that each part of the block is eight consecutive R, G, G, B
      unzipped1 = vuzp_u8(block.val[0], block.val[1]);
      unzipped2 = vuzp_u8(block.val[2], block.val[3]);

      block.val[0] = unzipped1.val[0];
      block.val[1] = unzipped1.val[1];
      block.val[2] = unzipped2.val[0];
      block.val[3] = unzipped2.val[1];

      // Gamma Correct is the most expensive step timewise. Doing it as a helper function seems to have no effect on
      // the time to complete this step.
      
#if DO_GREEN_AVG
      GammaCorrect(store.gammaLUT, store.value_32, block.val[0]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[2]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[3]);
      rgb.val[0] = block.val[0];
      rgb.val[1] = vhadd_u8(block.val[1], block.val[2]);
      rgb.val[2] = block.val[3];
#else
      GammaCorrect(store.gammaLUT, store.value_32, block.val[0]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[3]);
      rgb.val[0] = block.val[0];
      rgb.val[1] = block.val[1];
      rgb.val[2] = block.val[3];
#endif

      vst3_u8(outBufferPtr, rgb);

      outBufferPtr += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;
    
    // NOTE: Don't need to skip any output rows, we're iterating over all of them.
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_RGB24_EIGHTH(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);
  if (!setup.valid)
  {
    return RESULT_FAIL;
  }

  // The index to pull values from the table of loaded bytes
  const uint8x8_t index = vld1_u8(setup.indexes.data());

  StoreInfo store;
  for (int i = 0; i < store.gammaLUT.size(); ++i){
    for (int j = 0; j < 4; ++j){
      store.gammaLUT[i].val[j] = vld1_u8(_gammaLUT.data()+8*(i*4+j));
    }
  }
  store.value_32 = vdup_n_u8(32);

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;
  u8* outBufferPtr = outArgs.data;

  // Inner Loop variables; filled out during each iteration.
  uint8x8x4_t table;
  uint8x8x4_t block;
  uint8x8x2_t unzipped1;
  uint8x8x2_t unzipped2;
  uint8x8x3_t rgb;

  // Explanation of the iteration:
  //  - Neon allows us to look at and fill out 8 bytes of data
  //  - To get 8 bytes of Red, Blue, and Green, we need to look at 4 blocks of bayer data
  //  - We are either computing 2 rows of output data (FULL) or 1 row (HALF, QUARTER, EIGHTH)
  //  - We are either computing 16 cols of output data (FULL) or 8 cols (HALF, QUARTER, EIGHTH)
  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      // Load and Extract the bytes
      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      table.val[2] = vld1_u8(inBufferPtr1+setup.offsets[2]);
      table.val[3] = vld1_u8(inBufferPtr1+setup.offsets[3]);
      block.val[0] = vtbl4_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      table.val[2] = vld1_u8(inBufferPtr1+setup.offsets[2]);
      table.val[3] = vld1_u8(inBufferPtr1+setup.offsets[3]);
      block.val[1] = vtbl4_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      table.val[2] = vld1_u8(inBufferPtr2+setup.offsets[2]);
      table.val[3] = vld1_u8(inBufferPtr2+setup.offsets[3]);
      block.val[2] = vtbl4_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      table.val[2] = vld1_u8(inBufferPtr2+setup.offsets[2]);
      table.val[3] = vld1_u8(inBufferPtr2+setup.offsets[3]);
      block.val[3] = vtbl4_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      // Unzip the bytes so that each part of the block is eight consecutive R, G, G, B
      unzipped1 = vuzp_u8(block.val[0], block.val[1]);
      unzipped2 = vuzp_u8(block.val[2], block.val[3]);

      block.val[0] = unzipped1.val[0];
      block.val[1] = unzipped1.val[1];
      block.val[2] = unzipped2.val[0];
      block.val[3] = unzipped2.val[1];

      // Gamma Correct is the most expensive step timewise. Doing it as a helper function seems to have no effect on
      // the time to complete this step.
      
#if DO_GREEN_AVG
      GammaCorrect(store.gammaLUT, store.value_32, block.val[0]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[2]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[3]);
      rgb.val[0] = block.val[0];
      rgb.val[1] = vhadd_u8(block.val[1], block.val[2]);
      rgb.val[2] = block.val[3];
#else
      GammaCorrect(store.gammaLUT, store.value_32, block.val[0]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[3]);
      rgb.val[0] = block.val[0];
      rgb.val[1] = block.val[1];
      rgb.val[2] = block.val[3];
#endif

      vst3_u8(outBufferPtr, rgb);

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

  // The index to pull values from the table of loaded bytes
  const uint8x8_t index = vld1_u8(setup.indexes.data());

  StoreInfo store;
  for (int i = 0; i < store.gammaLUT.size(); ++i){
    for (int j = 0; j < 4; ++j){
      store.gammaLUT[i].val[j] = vld1_u8(_gammaLUT.data()+8*(i*4+j));
    }
  }
  store.value_32 = vdup_n_u8(32);

  // Map for creating full size images
  const u8 map_low[8] =  { 0, 0, 1, 1, 2, 2, 3, 3};
  const u8 map_high[8] = { 4, 4, 5, 5, 6, 6, 7, 7};
  
  store.mapL = vld1_u8(map_low);
  store.mapH = vld1_u8(map_high);

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;
  u8* outBufferPtr = outArgs.data;

  // Inner Loop variables; filled out during each iteration.
  uint8x8x2_t table;
  uint8x8x4_t block;
  uint8x8x2_t unzipped1;
  uint8x8x2_t unzipped2;

  // Explanation of the iteration:
  //  - Neon allows us to look at and fill out 8 bytes of data
  //  - To get 8 bytes of Red, Blue, and Green, we need to look at 4 blocks of bayer data
  //  - We are either computing 2 rows of output data (FULL) or 1 row (HALF, QUARTER, EIGHTH)
  //  - We are either computing 16 cols of output data (FULL) or 8 cols (HALF, QUARTER, EIGHTH)
  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      // Load and Extract the bytes
      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      block.val[0] = vtbl2_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      block.val[1] = vtbl2_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      block.val[2] = vtbl2_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      block.val[3] = vtbl2_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      // Unzip the bytes so that each part of the block is eight consecutive R, G, G, B
      unzipped1 = vuzp_u8(block.val[0], block.val[1]);
      unzipped2 = vuzp_u8(block.val[2], block.val[3]);

      block.val[0] = unzipped1.val[0];
      block.val[1] = unzipped1.val[1];
      block.val[2] = unzipped2.val[0];
      block.val[3] = unzipped2.val[1];

      // Gamma Correct is the most expensive step timewise. Doing it as a helper function seems to have no effect on
      // the time to complete this step.
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[2]);
      
      // Store Y8
      vst1_u8(outBufferPtr,                         vtbl1_u8(block.val[1], store.mapL));
      vst1_u8(outBufferPtr + 8,                     vtbl1_u8(block.val[1], store.mapH));
      vst1_u8(outBufferPtr + setup.outRowSkip,      vtbl1_u8(block.val[2], store.mapL));
      vst1_u8(outBufferPtr + 8 + setup.outRowSkip,  vtbl1_u8(block.val[2], store.mapH));

      outBufferPtr += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;
    outBufferPtr += setup.outRowSkip;
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_Y8_HALF_or_QUARTER(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);
  if (!setup.valid)
  {
    return RESULT_FAIL;
  }

  // The index to pull values from the table of loaded bytes
  const uint8x8_t index = vld1_u8(setup.indexes.data());

  StoreInfo store;
  for (int i = 0; i < store.gammaLUT.size(); ++i){
    for (int j = 0; j < 4; ++j){
      store.gammaLUT[i].val[j] = vld1_u8(_gammaLUT.data()+8*(i*4+j));
    }
  }
  store.value_32 = vdup_n_u8(32);

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;
  u8* outBufferPtr = outArgs.data;

  // Inner Loop variables; filled out during each iteration.
  uint8x8x2_t table;
  uint8x8x4_t block;
  uint8x8x2_t unzipped1;
  uint8x8x2_t unzipped2;

  // Explanation of the iteration:
  //  - Neon allows us to look at and fill out 8 bytes of data
  //  - To get 8 bytes of Red, Blue, and Green, we need to look at 4 blocks of bayer data
  //  - We are either computing 2 rows of output data (FULL) or 1 row (HALF, QUARTER, EIGHTH)
  //  - We are either computing 16 cols of output data (FULL) or 8 cols (HALF, QUARTER, EIGHTH)
  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      // Load and Extract the bytes
      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      block.val[0] = vtbl2_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      block.val[1] = vtbl2_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      block.val[2] = vtbl2_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      block.val[3] = vtbl2_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      // Unzip the bytes so that each part of the block is eight consecutive R, G, G, B
      unzipped1 = vuzp_u8(block.val[0], block.val[1]);
      unzipped2 = vuzp_u8(block.val[2], block.val[3]);

      block.val[0] = unzipped1.val[0];
      block.val[1] = unzipped1.val[1];
      block.val[2] = unzipped2.val[0];
      block.val[3] = unzipped2.val[1];

      // Store Y8
#if DO_GREEN_AVG
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[2]);
      vst1_u8(outBufferPtr, vhadd_u8(block.val[1], block.val[2]));
#else
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      vst1_u8(outBufferPtr, block.val[1]);
#endif

      outBufferPtr += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;
    
    // NOTE: Don't need to skip any output rows, we're iterating over all of them.
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Result HandleRAW10::RAW10_to_Y8_EIGHTH(const Debayer::InArgs& inArgs, Debayer::OutArgs& outArgs) const
{
  SetupInfo setup(inArgs, outArgs);
  if (!setup.valid)
  {
    return RESULT_FAIL;
  }

  // The index to pull values from the table of loaded bytes
  const uint8x8_t index = vld1_u8(setup.indexes.data());

  StoreInfo store;
  for (int i = 0; i < store.gammaLUT.size(); ++i){
    for (int j = 0; j < 4; ++j){
      store.gammaLUT[i].val[j] = vld1_u8(_gammaLUT.data()+8*(i*4+j));
    }
  }
  store.value_32 = vdup_n_u8(32);

  u8* inBufferPtr1 = inArgs.data;
  u8* inBufferPtr2 = inBufferPtr1 + setup.inRowSkip;
  u8* outBufferPtr = outArgs.data;

  // Inner Loop variables; filled out during each iteration.
  uint8x8x4_t table;
  uint8x8x4_t block;
  uint8x8x2_t unzipped1;
  uint8x8x2_t unzipped2;

  // Explanation of the iteration:
  //  - Neon allows us to look at and fill out 8 bytes of data
  //  - To get 8 bytes of Red, Blue, and Green, we need to look at 4 blocks of bayer data
  //  - We are either computing 2 rows of output data (FULL) or 1 row (HALF, QUARTER, EIGHTH)
  //  - We are either computing 16 cols of output data (FULL) or 8 cols (HALF, QUARTER, EIGHTH)
  for (u32 row = 0; row < outArgs.height; row += setup.rowStep)
  {
    for (u32 col = 0; col < outArgs.width; col += setup.colStep)
    {
      // Load and Extract the bytes
      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      table.val[2] = vld1_u8(inBufferPtr1+setup.offsets[2]);
      table.val[3] = vld1_u8(inBufferPtr1+setup.offsets[3]);
      block.val[0] = vtbl4_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr1);
      table.val[1] = vld1_u8(inBufferPtr1+setup.offsets[1]);
      table.val[2] = vld1_u8(inBufferPtr1+setup.offsets[2]);
      table.val[3] = vld1_u8(inBufferPtr1+setup.offsets[3]);
      block.val[1] = vtbl4_u8(table, index);
      inBufferPtr1 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      table.val[2] = vld1_u8(inBufferPtr2+setup.offsets[2]);
      table.val[3] = vld1_u8(inBufferPtr2+setup.offsets[3]);
      block.val[2] = vtbl4_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      table.val[0] = vld1_u8(inBufferPtr2);
      table.val[1] = vld1_u8(inBufferPtr2+setup.offsets[1]);
      table.val[2] = vld1_u8(inBufferPtr2+setup.offsets[2]);
      table.val[3] = vld1_u8(inBufferPtr2+setup.offsets[3]);
      block.val[3] = vtbl4_u8(table, index);
      inBufferPtr2 += setup.inColSkip;

      // Unzip the bytes so that each part of the block is eight consecutive R, G, G, B
      unzipped1 = vuzp_u8(block.val[0], block.val[1]);
      unzipped2 = vuzp_u8(block.val[2], block.val[3]);

      block.val[0] = unzipped1.val[0];
      block.val[1] = unzipped1.val[1];
      block.val[2] = unzipped2.val[0];
      block.val[3] = unzipped2.val[1];

#if DO_GREEN_AVG
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      GammaCorrect(store.gammaLUT, store.value_32, block.val[2]);
      vst1_u8(outBufferPtr, vhadd_u8(block.val[1], block.val[2]));
#else
      GammaCorrect(store.gammaLUT, store.value_32, block.val[1]);
      vst1_u8(outBufferPtr, block.val[1]);
#endif

      outBufferPtr += setup.outColSkip;
    }
    inBufferPtr1 += setup.inRowSkip;
    inBufferPtr2 += setup.inRowSkip;
    
    // NOTE: Don't need to skip any output rows, we're iterating over all of them.
  }

  return RESULT_OK;
}

} /* namespace Neon */
} /* namespace Vision */
} /* namespace Anki */

#endif /* __ARM_NEON__ */