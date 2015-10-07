/**
 * File: ImageDeChunker.h
 *
 * Author: Andrew Stein
 * Date:   11/20/2014
 *
 * Description: Defines a dechunker for Cozmo image data
 *
 * Copyright: Anki, Inc. 2015
 **/

#include <opencv2/core/core.hpp>
#include "anki/common/types.h"
#include "util/logging/logging.h"
#include "anki/vision/CameraSettings.h"
#include "clad/types/imageTypes.h"

namespace Anki {
namespace Cozmo {

class ImageDeChunker
{
public:
  
  ImageDeChunker();
  
  // Return true if image just completed and is available, false otherwise.
  bool AppendChunk(u32 imageId, u32 frameTimeStamp, u16 nrows, u16 ncols,
                   ImageEncoding encoding, u8 totalChunkCount,
                   u8 chunkId, const std::array<u8, (size_t)ImageConstants::IMAGE_CHUNK_SIZE>& data, u32 chunkSize);
  
  bool AppendChunk(u32 imageId, u32 frameTimeStamp, u16 nrows, u16 ncols,
                   ImageEncoding encoding, u8 totalChunkCount,
                   u8 chunkId, const u8* data, u32 chunkSize);
  
  cv::Mat GetImage() { return _img; }
  const std::vector<u8>& GetRawData() const { return _imgData; }
  
  //void SetImageCompleteCallback(std::function<void(const Vision::Image&)>callback) const;
  
private:

  u32              _imgID;
  std::vector<u8>  _imgData;
  u32              _imgBytes;
  u32              _imgWidth, _imgHeight;
  u8               _expectedChunkId;
  bool             _isImgValid;
  
  cv::Mat          _img;
  
}; // class ImageDeChunker

} // namespace Cozmo
} // namespace Anki
