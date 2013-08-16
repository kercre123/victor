#include "anki/vision.h"

namespace Anki
{
  Result DownsampleByFactor(const Matrix<u8> &img, u32 downsampleFactor, Matrix<u8> &imgDownsampled)
  {
    DASConditionalErrorAndReturnValue(IsPowerOfTwo(downsampleFactor),
      RESULT_FAIL, "DownsampleByFactor", "downsampleFactor must be a power of 2");

    //imgDownsampled = zeros(floor(size(img)/downsampleFactor), class(img));
    DASConditionalErrorAndReturnValue(imgDownsampled.get_size(0) == img.get_size(0) >> downsampleFactor && imgDownsampled.get_size(1) == img.get_size(1) >> downsampleFactor,
      RESULT_FAIL, "DownsampleByFactor", "size(imgDownsampled) is not equal to size(img) >> downsampleFactor");

    //maxY = downsampleFactor * size(imgDownsampled,1);
    //maxX = downsampleFactor * size(imgDownsampled,2);
    //
    //img = uint16(img);
    //
    //smallY = 1;
    //for y = 1:downsampleFactor:maxY
    //
    //    smallX = 1;
    //    for x = 1:downsampleFactor:maxX
    //%         imgDownsampled(smallY, smallX) = uint8((img(y,x) + img(y+1,x) + img(y,x+1) + img(y+1,x+1)) / 4);
    //        imgDownsampled(smallY, smallX) = uint8( sum(sum(img(y:(y+downsampleFactor-1),x:(x+downsampleFactor-1)))) / (downsampleFactor^2) );
    //
    //        smallX = smallX + 1;
    //    end
    //
    //    smallY = smallY + 1;
    //end

    return RESULT_OK;
  }
} // namespace Anki