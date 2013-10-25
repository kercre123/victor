#include "anki/vision/robot/miscVisionKernels.h"

namespace Anki
{
  namespace Embedded
  {
    IN_DDR Result DownsampleByFactor(const Array<u8> &image, const s32 downsampleFactor, Array<u8> &imageDownsampled)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL, "DownsampleByFactor", "image is not valid");

      AnkiConditionalErrorAndReturnValue(imageDownsampled.IsValid(),
        RESULT_FAIL, "DownsampleByFactor", "imageDownsampled is not valid");

      AnkiConditionalErrorAndReturnValue(downsampleFactor == 2,
        RESULT_FAIL, "DownsampleByFactor", "Currently, only downsampleFactor==2 is supported");

      AnkiConditionalErrorAndReturnValue(IsPowerOfTwo(downsampleFactor),
        RESULT_FAIL, "DownsampleByFactor", "downsampleFactor must be a power of 2");

      AnkiConditionalErrorAndReturnValue(imageDownsampled.get_size(0) == (imageHeight / downsampleFactor) && imageDownsampled.get_size(1) == (imageWidth / downsampleFactor),
        RESULT_FAIL, "DownsampleByFactor", "size(imageDownsampled) is not equal to size(image) >> downsampleFactor");

      const s32 maxY = downsampleFactor * imageDownsampled.get_size(0);
      const s32 maxX = downsampleFactor * imageDownsampled.get_size(1);

      for(s32 y=0, ySmall=0; y<maxY; y+=2, ySmall++) {
        const u8 * restrict image_rowPointerY0 = image.Pointer(y, 0);
        const u8 * restrict image_rowPointerY1 = image.Pointer(y+1, 0);

        u8 * restrict imageDownsampled_rowPointer = imageDownsampled.Pointer(ySmall, 0);

        for(s32 x=0, xSmall=0; x<maxX; x+=2, xSmall++) {
          const u32 imageSum = static_cast<u32>(image_rowPointerY0[x]) + static_cast<u32>(image_rowPointerY0[x+1]) + static_cast<u32>(image_rowPointerY1[x]) + static_cast<u32>(image_rowPointerY1[x+1]);
          imageDownsampled_rowPointer[xSmall] = static_cast<u8>(imageSum >> 2);
        }
      }

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki