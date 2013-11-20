#include "anki/common/robot/interpolate.h"

namespace Anki
{
  namespace Embedded
  {
    Result Interp2(const Array<u8> &reference, const Array<f32> &xCoordinates, const Array<f32> &yCoordinates, Array<u8> &out, const InterpolationType interpolationType, const u8 invalidValue)
    {
      AnkiConditionalErrorAndReturnValue(interpolationType == INTERPOLATE_BILINEAR,
        RESULT_FAIL, "Interp2", "Only INTERPOLATE_BILINEAR is supported");

      AnkiConditionalErrorAndReturnValue(reference.IsValid(),
        RESULT_FAIL, "Interp2", "reference is not valid");

      AnkiConditionalErrorAndReturnValue(xCoordinates.IsValid(),
        RESULT_FAIL, "Interp2", "xCoordinates is not valid");

      AnkiConditionalErrorAndReturnValue(yCoordinates.IsValid(),
        RESULT_FAIL, "Interp2", "yCoordinates is not valid");

      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL, "Interp2", "out is not valid");

      AnkiConditionalErrorAndReturnValue(xCoordinates.get_size(1) == yCoordinates.get_size(1) &&
        xCoordinates.get_size(1) == out.get_size(1) &&
        xCoordinates.get_size(0) == 1 &&
        yCoordinates.get_size(0) == 1 &&
        out.get_size(0) == 1,
        RESULT_FAIL, "Interp2", "xCoordinates, yCoordinates, and out must be 1xN");

      AnkiConditionalErrorAndReturnValue(xCoordinates.get_rawDataPointer() != out.get_rawDataPointer() &&
        yCoordinates.get_rawDataPointer() != out.get_rawDataPointer() &&
        reference.get_rawDataPointer() != out.get_rawDataPointer(),
        RESULT_FAIL, "Interp2", "xCoordinates, yCoordinates, and reference cannot be the same as out");

      const s32 referenceHeight = reference.get_size(0);
      const s32 referenceWidth = reference.get_size(1);

      const f32 xyMin = 0.0f;
      const f32 xMax = static_cast<f32>(referenceWidth) - 1.0f;
      const f32 yMax = static_cast<f32>(referenceHeight) - 1.0f;

      const s32 numValues = xCoordinates.get_size(1);

      const f32 * restrict pXCoordinates = xCoordinates.Pointer(0,0);
      const f32 * restrict pYCoordinates = yCoordinates.Pointer(0,0);
      const u8 * restrict pReference = reference.Pointer(0,0);

      u8 * restrict pOut = out.Pointer(0,0);

      for(s32 i=0; i<numValues; i++) {
        const f32 curX = pXCoordinates[i];
        const f32 curY = pYCoordinates[i];

        const f32 x0 = floorf(curX);
        const f32 x1 = x0 + 1.0f; // ceilf(curX);

        const f32 y0 = floorf(curY);
        const f32 y1 = y0 + 1.0f; // ceilf(curY);

        // If out of bounds, set as invalid and continue
        if(x0 < xyMin || x1 > xMax || y0 < xyMin || y1 > yMax) {
          pOut[i] = invalidValue;
          continue;
        }

        const f32 alphaX = curX - x0;
        const f32 alphaXinverse = 1 - alphaX;

        const f32 alphaY = curY - y0;
        const f32 alphaYinverse = 1.0f - alphaY;

        const s32 y0S32 = static_cast<s32>(Roundf(y0));
        const s32 y1S32 = static_cast<s32>(Roundf(y1));
        const s32 x0S32 = static_cast<s32>(Roundf(x0));

        const u8 * restrict pReference_y0 = reference.Pointer(y0S32, x0S32);
        const u8 * restrict pReference_y1 = reference.Pointer(y1S32, x0S32);

        const f32 pixelTL = *pReference_y0;
        const f32 pixelTR = *(pReference_y0+1);
        const f32 pixelBL = *pReference_y1;
        const f32 pixelBR = *(pReference_y1+1);

        const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);
        const u8 interpolatedPixel = static_cast<u8>(Roundf(interpolatedPixelF32));

        pOut[i] = interpolatedPixel;
      }

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki