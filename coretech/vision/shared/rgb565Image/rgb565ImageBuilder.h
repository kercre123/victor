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

#ifndef __RGB565_Image_Builder_H__
#define __RGB565_Image_Builder_H__

#include "clad/types/spriteNames.h"
#include "util/cladHelpers/cladEnumToStringMap.h"

namespace Anki {
namespace Vision{

class RGB565ImageBuilder {
public:
  static const unsigned int PIXEL_COUNT = 17664;
  static const unsigned int PIXEL_COUNT_PER_CHUNK = 600;
  using ChunkDataContainer = std::array<uint16_t, PIXEL_COUNT_PER_CHUNK>;
  using FullDataContainer = std::vector<uint16_t>;

  RGB565ImageBuilder();
  virtual ~RGB565ImageBuilder();

  // Add a new chunk to the internal image representation
  void AddDataChunk(const ChunkDataContainer& chunkData, uint16_t chunkIndex, uint16_t numPixels);

  // Returns a bitmask of the chunkIndexes we have recieved
  inline uint32_t GetRecievedChunkMask() const { return _chunkMask; }

  // Returns a reference to the internally stored data container
  inline const FullDataContainer& GetAllData() const { return _data; }

  // flushes all construction data
  void Clear();

private:
  uint32_t _chunkMask;
  FullDataContainer _data;
};

};
};

#endif // __RGB565_Image_Builder_H__
