/**
* File: compositeImageBuilder.h
*
* Author: Kevin M. Karol
* Created: 3/29/2018
*
* Description: Build a composite image out of clad chunks
* Provides a clean interface that prevents compositeImages from being used while
* chunks are still pending
*
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Vision_CompositeImageBuilder_H__
#define __Vision_CompositeImageBuilder_H__


#include "coretech/vision/shared/compositeImage/compositeImage.h"
#include "coretech/vision/shared/compositeImage/compositeImageLayer.h"
#include "clad/types/compositeImageTypes.h"


namespace Anki {
namespace Vision {

class CompositeImage;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CompositeImageBuilder {
public:
  // The chunk passed into the constructor will be used to validate all future chunks
  // and determine when all chunks have been received
  CompositeImageBuilder(SpriteCache* cache, SpriteSequenceContainer* seqContainer, const CompositeImageChunk& chunk);
  virtual ~CompositeImageBuilder();

  // Add a new chunk to the internal image representation - returns true if
  // the new chunk matches the ID/properties of the constructor chunk
  bool AddImageChunk(SpriteCache* cache, SpriteSequenceContainer* seqContainer, const CompositeImageChunk& chunk);

  // Returns true if all image chunks have been received
  bool CanBuildImage();

  // Returns true if all image chunks have been received and the image passed in
  // has been set
  bool GetCompositeImage(CompositeImage& outImage);


private:  
  using SpriteBoxCountMap = std::map<Vision::LayerName, uint8_t>;

  uint8_t           _expectedNumLayers;
  SpriteBoxCountMap _expectedSpriteBoxesPerLayer;
  
  uint32_t _width;
  uint32_t _height;
  CompositeImage::LayerMap _layerMap;
};

}; // namespace Vision
}; // namespace Anki

#endif // __Vision_CompositeImageBuilder_H__
