#include "anki/vision/robot/miscVisionKernels.h"

namespace Anki
{
  namespace Embedded
  {
    // binaryImg = img < averageImg;
    IN_DDR Result BinarizeScaleImage(const Array<u8> &originalImage, const FixedPointArray<u32> &scaleImage, Array<u8> &binaryImage)
    {
      for(s32 y=0; y<scaleImage.get_size(0); y++) {
        const u32 * restrict scaleImage_rowPointer = scaleImage.Pointer(y, 0);
        const u8 * restrict originalImage_rowPointer = originalImage.Pointer(y, 0);
        u8 * restrict binaryImage_rowPointer = binaryImage.Pointer(y, 0);

        memset(binaryImage_rowPointer, 0, binaryImage.get_strideWithoutFillPatterns());

        for(s32 x=0; x<scaleImage.get_size(1); x++) {
          const u8 originalValue = originalImage_rowPointer[x];
          const u8 scaleValue = static_cast<u8>( scaleImage_rowPointer[x] >> scaleImage.get_numFractionalBits() );
          if(originalValue < scaleValue) {
            binaryImage_rowPointer[x] = 1;
          }
        } // for(s32 x=0; x<scaleImage.get_size(1); x++)
      } // for(s32 y=0; y<scaleImage.get_size(0); y++)

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki