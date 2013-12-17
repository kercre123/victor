#include "anki/common/robot/interpolate.h"

namespace Anki
{
  namespace Embedded
  {
    template<> Result Interp2(const Array<u8> &reference, const Array<f32> &xCoordinates, const Array<f32> &yCoordinates, Array<u8> &out, const InterpolationType interpolationType, const u8 invalidValue)
    {
      AnkiConditionalErrorAndReturnValue(interpolationType == INTERPOLATE_LINEAR,
        RESULT_FAIL_INVALID_PARAMETERS, "Interp2", "Only INTERPOLATE_LINEAR is supported");

      AnkiConditionalErrorAndReturnValue(reference.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "reference is not valid");

      AnkiConditionalErrorAndReturnValue(xCoordinates.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "xCoordinates is not valid");

      AnkiConditionalErrorAndReturnValue(yCoordinates.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "yCoordinates is not valid");

      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "out is not valid");

      const s32 referenceHeight = reference.get_size(0);
      const s32 referenceWidth = reference.get_size(1);

      const s32 outHeight = out.get_size(0);
      const s32 outWidth = out.get_size(1);

      const s32 numOutputElements = outHeight * outWidth;

      const bool isOutputOneDimensional = (out.get_size(0) == 1);

      if(isOutputOneDimensional) {
        AnkiConditionalErrorAndReturnValue(
          out.get_size(1) == numOutputElements && xCoordinates.get_size(1) == numOutputElements && yCoordinates.get_size(1) == numOutputElements &&
          xCoordinates.get_size(0) == 1 && yCoordinates.get_size(0) == 1,
          RESULT_FAIL_INVALID_SIZE, "Interp2", "If out is a row vector, then out, xCoordinates, and yCoordinates must all be 1xN");
      } else {
        AnkiConditionalErrorAndReturnValue(
          xCoordinates.get_size(0) == outHeight && xCoordinates.get_size(1) == outWidth &&
          yCoordinates.get_size(0) == outHeight && yCoordinates.get_size(1) == outWidth,
          RESULT_FAIL_INVALID_SIZE, "Interp2", "If the out is not 1 dimensional, then xCoordinates, yCoordinates, and out must all be the same sizes");
      }

      AnkiConditionalErrorAndReturnValue(xCoordinates.get_rawDataPointer() != out.get_rawDataPointer() &&
        yCoordinates.get_rawDataPointer() != out.get_rawDataPointer() &&
        reference.get_rawDataPointer() != out.get_rawDataPointer(),
        RESULT_FAIL_INVALID_SIZE, "Interp2", "xCoordinates, yCoordinates, and reference cannot be the same as out");

      const f32 xyReferenceMin = 0.0f;
      const f32 xReferenceMax = static_cast<f32>(referenceWidth) - 1.0f;
      const f32 yReferenceMax = static_cast<f32>(referenceHeight) - 1.0f;

      //const s32 numValues = xCoordinates.get_size(1);

      const s32 yIterationMax = isOutputOneDimensional ? 1                    : outHeight;
      const s32 xIterationMax = isOutputOneDimensional ? (outHeight*outWidth) : outWidth;

      for(s32 y=0; y<yIterationMax; y++) {
        const f32 * restrict pXCoordinates = xCoordinates.Pointer(y,0);
        const f32 * restrict pYCoordinates = yCoordinates.Pointer(y,0);

        u8 * restrict pOut = out.Pointer(y,0);

        for(s32 x=0; x<xIterationMax; x++) {
          const f32 curX = pXCoordinates[x];
          const f32 curY = pYCoordinates[x];

          const f32 x0 = FLT_FLOOR(curX);
          const f32 x1 = ceilf(curX); // x0 + 1.0f;

          const f32 y0 = FLT_FLOOR(curY);
          const f32 y1 = ceilf(curY); // y0 + 1.0f;

          // If out of bounds, set as invalid and continue
          if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
            pOut[x] = invalidValue;
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

          pOut[x] = interpolatedPixel;
        } // for(s32 x=0; x<xIterationMax; x++)
      } // for(s32 y=0; y<yIterationMax; y++)

      return RESULT_OK;
    }

    template<> Result Interp2_Affine(const Array<u8> &reference, const Array<f32> &homography, Array<u8> &out, const InterpolationType interpolationType, const u8 invalidValue)
    {
      AnkiConditionalErrorAndReturnValue(interpolationType == INTERPOLATE_LINEAR,
        RESULT_FAIL_INVALID_PARAMETERS, "Interp2_Affine", "Only INTERPOLATE_LINEAR is supported");

      AnkiConditionalErrorAndReturnValue(reference.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2_Affine", "reference is not valid");

      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2_Affine", "out is not valid");

      const s32 referenceHeight = reference.get_size(0);
      const s32 referenceWidth = reference.get_size(1);

      const s32 outHeight = out.get_size(0);
      const s32 outWidth = out.get_size(1);

      AnkiConditionalErrorAndReturnValue(reference.get_rawDataPointer() != out.get_rawDataPointer(),
        RESULT_FAIL_ALIASED_MEMORY, "Interp2_Affine", "out and reference cannot be the same as out");

      const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
      const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];

      const f32 xyReferenceMin = 0.0f;
      const f32 xReferenceMax = static_cast<f32>(referenceWidth) - 1.0f;
      const f32 yReferenceMax = static_cast<f32>(referenceHeight) - 1.0f;

      for(s32 y=0; y<outHeight; y++) {
        u8 * restrict pOut = out.Pointer(y,0);

        f32 xTransformed = h00*(0.5f) + h01*(static_cast<f32>(y)+0.5f) + h02 - 0.5f;
        f32 yTransformed = h10*(0.5f) + h11*(static_cast<f32>(y)+0.5f) + h12 - 0.5f;

        for(s32 x=0; x<outWidth; x++) {
          const f32 x0 = FLT_FLOOR(xTransformed);
          const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

          const f32 y0 = FLT_FLOOR(yTransformed);
          const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

          // If out of bounds, set as invalid and continue
          if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
            pOut[x] = invalidValue;
            continue;
          }

          const f32 alphaX = xTransformed - x0;
          const f32 alphaXinverse = 1 - alphaX;

          const f32 alphaY = yTransformed - y0;
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

          pOut[x] = interpolatedPixel;

          // strength reduction for the affine transformation along this horizontal line
          xTransformed += h00;
          yTransformed += h10;
        } // for(s32 x=0; x<xIterationMax; x++)
      } // for(s32 y=0; y<yIterationMax; y++)

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki