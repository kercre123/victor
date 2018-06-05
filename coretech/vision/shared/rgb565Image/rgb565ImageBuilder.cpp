/**
 * File: rgb565ImageBuilder.h
 *
 * Author: Nicolas Kent
 * Date:   5/25/2018
 *
 * Description: Assembles a proper rgb565 image from chunks sent to engine
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/vision/shared/rgb565Image/rgb565ImageBuilder.h"

namespace Anki {
namespace Vision {

RGB565ImageBuilder::RGB565ImageBuilder()
: _chunkMask(0)
, _data(PIXEL_COUNT)
{

}

RGB565ImageBuilder::~RGB565ImageBuilder()
{

}

void RGB565ImageBuilder::AddDataChunk(const ChunkDataContainer& chunkData, uint16_t chunkIndex, uint16_t numPixels)
{
  _chunkMask |= (1L << chunkIndex);

  const uint32_t offset = chunkIndex * PIXEL_COUNT_PER_CHUNK;
  std::copy(std::begin(chunkData), std::begin(chunkData) + numPixels, std::begin(_data) + offset);
}

void RGB565ImageBuilder::Clear()
{
  _chunkMask = 0;
}

} // namespace Vision
} // namespace Anki
