#include "anki/vision.h"

namespace Anki
{
  Result DownsampleByFactor(const Matrix<u8> &img, u32 downsampleFactor, Matrix<u8> &imgDownsampled)
  {
    DASConditionalErrorAndReturnValue(downsampleFactor == 2,
      RESULT_FAIL, "DownsampleByFactor", "Currently, only downsampleFactor==2 is supported");

    DASConditionalErrorAndReturnValue(IsPowerOfTwo(downsampleFactor),
      RESULT_FAIL, "DownsampleByFactor", "downsampleFactor must be a power of 2");

    DASConditionalErrorAndReturnValue(imgDownsampled.get_size(0) == (img.get_size(0) / downsampleFactor) && imgDownsampled.get_size(1) == (img.get_size(1) / downsampleFactor),
      RESULT_FAIL, "DownsampleByFactor", "size(imgDownsampled) is not equal to size(img) >> downsampleFactor");

    const u32 maxY = downsampleFactor * imgDownsampled.get_size(0);
    const u32 maxX = downsampleFactor * imgDownsampled.get_size(1);

    for(u32 y=0, ySmall=0; y<maxY; y+=2, ySmall++) {
      const u8 * restrict img_rowPointerY0 = img.Pointer(y, 0);
      const u8 * restrict img_rowPointerY1 = img.Pointer(y+1, 0);

      u8 * restrict imgDownsampled_rowPointer = imgDownsampled.Pointer(ySmall, 0);

      for(u32 x=0, xSmall=0; x<maxX; x+=2, xSmall++) {
        const u32 imgSum = static_cast<u32>(img_rowPointerY0[x]) + static_cast<u32>(img_rowPointerY0[x+1]) + static_cast<u32>(img_rowPointerY1[x]) + static_cast<u32>(img_rowPointerY1[x+1]);
        imgDownsampled_rowPointer[xSmall] = static_cast<u8>(imgSum >> 2);
      }
    }

    return RESULT_OK;
  }
} // namespace Anki