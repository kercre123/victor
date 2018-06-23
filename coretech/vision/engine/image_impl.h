// This file exists simply to avoid confusion with including _impl files:
// Image, which doesn't really need an _impl file, inherits from Array2d, which
// does use an _impl file (since it's templated).  But it's weird for people
// who want to use Image to need to know that they need to include array2d_impl.

#ifndef __Anki_Vision_Image_Impl_H__
#define __Anki_Vision_Image_Impl_H__

#include "coretech/vision/engine/image.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/math/rect_impl.h"

namespace Anki {
namespace Vision {

template<typename T>
template<typename DerivedType>
void ImageBase<T>::DrawSubImage(const DerivedType& subImage, const Point2f& topLeftCorner, 
                                T blankPixelValue, bool drawBlankPixels)
{
  s32 subImageColOffset = 0;
  s32 subImageRowOffset = 0;

  // If the subimage wants to be placed with a negative position 
  // copy it starting wherever it would become visible in the image
  // rect will be clamped to 0,0 on intersect below
  if(topLeftCorner.x() < 0){
    subImageColOffset = -topLeftCorner.x();
  }
  if(topLeftCorner.y() < 0){
    subImageRowOffset = -topLeftCorner.y();
  }

  // Calculate where the subimage wants to be copied to and then clamp that area
  // to the space that actually exists within the image
  Rectangle<s32> rect(topLeftCorner.x(), topLeftCorner.y(), 
                      subImage.GetNumCols(), subImage.GetNumRows());    
  rect = GetBoundingRect().Intersect(rect);

  // named variables for readability
  const s32 numColsToCopy = rect.GetWidth();
  const s32 numRowsToCopy = rect.GetHeight();
  const s32 destColOffset = rect.GetX();
  const s32 destRowOffset = rect.GetY();

  if(drawBlankPixels){
    for(s32 relRowIdx = 0; relRowIdx < numRowsToCopy; ++relRowIdx){
      const T* source_row = subImage.GetRow(relRowIdx + subImageRowOffset) + subImageColOffset;
      T* dest_row = Array2d<T>::GetRow(relRowIdx + destRowOffset) + destColOffset;
      std::memcpy(dest_row, source_row, sizeof(T) * numColsToCopy);
    }
  }else{
    for(s32 relRowIdx = 0; relRowIdx < numRowsToCopy; ++relRowIdx){
      const T* source_row = subImage.GetRow(relRowIdx + subImageRowOffset) + subImageColOffset;
      T* dest_row = Array2d<T>::GetRow(relRowIdx + destRowOffset) + destColOffset;
      for(s32 relColIdx = 0; relColIdx < numColsToCopy; ++relColIdx){
        if(source_row[relColIdx] != blankPixelValue){
          dest_row[relColIdx] = source_row[relColIdx];
        } 
      }
    }
  }
}

} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_Image_Impl_H__
