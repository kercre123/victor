#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    // binaryImg = img < averageImg;
    Result ThresholdScaleImage(const Array<u8> &originalImage, const Array<u32> &scaleImage, Array<u8> &binaryImage)
    {
      for(s32 y=0; y<scaleImage.get_size(0); y++) {
        const u32 * restrict scaleImage_rowPointer = scaleImage.Pointer(y, 0);
        const u8 * restrict originalImage_rowPointer = originalImage.Pointer(y, 0);
        u8 * restrict binaryImage_rowPointer = binaryImage.Pointer(y, 0);

        memset(binaryImage_rowPointer, 0, binaryImage.get_stride());

        for(s32 x=0; x<scaleImage.get_size(1); x++) {
          if(originalImage_rowPointer[x] < scaleImage_rowPointer[x]) {
            binaryImage_rowPointer[x] = 1;
          }
        } // for(s32 x=0; x<scaleImage.get_size(1); x++)
      } // for(s32 y=0; y<scaleImage.get_size(0); y++)

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki