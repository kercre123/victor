#include "anki/vision/robot/miscVisionKernels.h"

namespace Anki
{
  namespace Embedded
  {
    // binaryImg = img < averageImg;
    Result BinarizeScaleImage(const Array<u8> &originalImage, const FixedPointArray<u32> &scaleImage, Array<u8> &binaryImage)
    {
      const s32 scaleImageHeight = scaleImage.get_size(0);
      const s32 scaleImageWidth = scaleImage.get_size(1);

      for(s32 y=0; y<scaleImageHeight; y++) {
        const u32 * restrict pScaleImage = scaleImage.Pointer(y, 0);
        const u8 * restrict pOriginalImage = originalImage.Pointer(y, 0);
        u8 * restrict pBinaryImage = binaryImage.Pointer(y, 0);

        memset(pBinaryImage, 0, binaryImage.get_strideWithoutFillPatterns());

        for(s32 x=0; x<scaleImageWidth; x++) {
          const u8 originalValue = pOriginalImage[x];
          const u8 scaleValue = static_cast<u8>( pScaleImage[x] >> scaleImage.get_numFractionalBits() );
          if(originalValue < scaleValue) {
            pBinaryImage[x] = 1;
          }
        } // for(s32 x=0; x<scaleImageWidth; x++)
      } // for(s32 y=0; y<scaleImageHeight; y++)

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki