/**
File: computeCharacteristicScale_binomial.cpp
Author: Peter Barnum
Created: 2014-10-21

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/utilities.h"
#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/interpolate.h"
#include "coretech/common/robot/hostIntrinsics_m4.h"

#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/integralImage.h"
#include "coretech/vision/robot/imageProcessing.h"

namespace Anki
{
  namespace Embedded
  {
    Result ExtractComponentsViaCharacteristicScale_binomial(
      const Array<u8> &image,
      const s32 numPyramidLevels,
      const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth,
      const s16 component1d_maxSkipDistance,
      ConnectedComponents &components,
      MemoryStack fastScratch,
      MemoryStack slowerScratch,
      MemoryStack slowestScratch)
    {
      const bool upsampleToFullSize = true;

      const s32 thresholdMultiplier_numFractionalBits = 16;

      BeginBenchmark("ecvcsB_init");

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      AnkiConditionalErrorAndReturnValue(AreValid(fastScratch, slowerScratch, slowestScratch),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale_binomial", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(NotAliased(fastScratch, slowerScratch, slowestScratch),
        RESULT_FAIL_ALIASED_MEMORY, "ExtractComponentsViaCharacteristicScale_binomial", "fast and slow scratch buffers cannot be the same object");

      AnkiConditionalErrorAndReturnValue(AreValid(image, components),
        RESULT_FAIL_INVALID_OBJECT, "ExtractComponentsViaCharacteristicScale_binomial", "Invalid objects");

      Result lastResult;

      for(s32 iLevel=1; iLevel<numPyramidLevels; iLevel++) {
        const s32 scaledHeight = imageHeight >> iLevel;
        const s32 scaledWidth = imageWidth >> iLevel;

        AnkiConditionalErrorAndReturnValue(scaledHeight%2 == 0 && scaledWidth%2 == 0,
          RESULT_FAIL, "ExtractComponentsViaCharacteristicScale_binomial", "Too many pyramid levels requested");
      }

      // Allocate all the memory for the pyramids

      FixedLengthList<Array<u8> > imagePyramid(numPyramidLevels, slowestScratch);
      FixedLengthList<Array<u8> > blurredImagePyramid(numPyramidLevels, slowestScratch);
      FixedLengthList<Array<u8> > dogPyramid(numPyramidLevels, slowestScratch);
      Array<u8> dogMax(imageHeight, imageWidth, slowestScratch);
      Array<u8> scaleImage(imageHeight, imageWidth, slowestScratch);

      AnkiConditionalErrorAndReturnValue(AreValid(imagePyramid, blurredImagePyramid, dogPyramid, dogMax, scaleImage),
        RESULT_FAIL_OUT_OF_MEMORY, "ExtractComponentsViaCharacteristicScale_binomial", "Out of memory");

      for(s32 iLevel=0; iLevel<numPyramidLevels; iLevel++) {
        const s32 scaledHeight = imageHeight >> iLevel;
        const s32 scaledWidth = imageWidth >> iLevel;

        if(iLevel == 0) {
          imagePyramid[0] = image;
        } else {
          imagePyramid[iLevel] = Array<u8>(scaledHeight, scaledWidth, slowestScratch);
        }

        blurredImagePyramid[iLevel] = Array<u8>(scaledHeight, scaledWidth, slowestScratch);
        dogPyramid[iLevel] = Array<u8>(scaledHeight, scaledWidth, slowestScratch);

        AnkiConditionalErrorAndReturnValue(AreValid(imagePyramid[iLevel], blurredImagePyramid[iLevel], dogPyramid[iLevel]),
          RESULT_FAIL_OUT_OF_MEMORY, "ExtractComponentsViaCharacteristicScale_binomial", "Out of memory");
      }

      Array<u8> binaryImageRow(1, imageWidth, fastScratch);

      AnkiConditionalErrorAndReturnValue(AreValid(binaryImageRow),
        RESULT_FAIL_OUT_OF_MEMORY, "ExtractComponentsViaCharacteristicScale_binomial", "binaryImageRow is not valid");

      u8 * restrict pBinaryImageRow = binaryImageRow[0];

      if((lastResult = components.Extract2dComponents_PerRow_Initialize(fastScratch, slowerScratch, slowestScratch)) != RESULT_OK)
        return lastResult;

      EndBenchmark("ecvcsB_init");

      //
      // First, create the whole unblurred and blurred pyramids
      //

      for(s32 iLevel=0; iLevel<numPyramidLevels; iLevel++) {
        if(iLevel != 0) {
          BeginBenchmark("ecvcsB_downsample");
          //if((lastResult = ImageProcessing::DownsampleByTwo<u8,u16,u8>(imagePyramid[iLevel-1], imagePyramid[iLevel])) != RESULT_OK)
          //  return lastResult;
          if((lastResult = ImageProcessing::DownsampleByTwo<u8,u16,u8>(blurredImagePyramid[iLevel-1], imagePyramid[iLevel])) != RESULT_OK)
            return lastResult;

          EndBenchmark("ecvcsB_downsample");
        }

        BeginBenchmark("ecvcsB_binomial");
        if((lastResult = ImageProcessing::BinomialFilter<u8,u8,u8>(imagePyramid[iLevel], blurredImagePyramid[iLevel], slowestScratch)) != RESULT_OK)
          return lastResult;
        EndBenchmark("ecvcsB_binomial");

        BeginBenchmark("ecvcsB_SAD");
        if((lastResult = Matrix::SumOfAbsDiff<u8,s16,u8>(imagePyramid[iLevel], blurredImagePyramid[iLevel], dogPyramid[iLevel])) != RESULT_OK)
          return lastResult;
        EndBenchmark("ecvcsB_SAD");

        //#if ANKICORETECH_EMBEDDED_USE_OPENCV
        //char name[1024];
        //snprintf(name, 1024, "image"); imagePyramid[iLevel].Show(name, false, false, true);
        //snprintf(name, 1024, "blurred"); blurredImagePyramid[iLevel].Show(name, false, false, true);
        //snprintf(name, 1024, "dog"); dogPyramid[iLevel].Show(name, false, false, true);
        //cv::waitKey();
        //#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV
      }

      BeginBenchmark("ecvcsB_scale");
      if(upsampleToFullSize) {
        Array<u8> upsampledImageTmp(imageHeight, imageWidth, slowestScratch);
        Array<u8> upsampledDogTmp(imageHeight, imageWidth, slowestScratch);

        AnkiAssert(numPyramidLevels < 6);

        for(s32 iLevel=0; iLevel<numPyramidLevels; iLevel++) {
          /*const s32 xyOutputStep = 1 << iLevel;*/

          /*const s32 scaledHeight = imageHeight >> iLevel;*/
          /*const s32 scaledWidth = imageWidth >> iLevel;*/

          Array<u8> upsampledImage;
          Array<u8> upsampledDog;
          if(iLevel == 0) {
            upsampledImage = imagePyramid[iLevel];
            upsampledDog = dogPyramid[iLevel];
          } else if(iLevel == 1) {
            BeginBenchmark("ecvcsB_scale_upsample");
            upsampledImage = upsampledImageTmp;
            upsampledDog = upsampledDogTmp;
            ImageProcessing::UpsampleByPowerOfTwoBilinear<1>(imagePyramid[iLevel], upsampledImage, slowestScratch);
            ImageProcessing::UpsampleByPowerOfTwoBilinear<1>(dogPyramid[iLevel], upsampledDog, slowestScratch);
            EndBenchmark("ecvcsB_scale_upsample");
          } else if(iLevel == 2) {
            BeginBenchmark("ecvcsB_scale_upsample");
            upsampledImage = upsampledImageTmp;
            upsampledDog = upsampledDogTmp;
            ImageProcessing::UpsampleByPowerOfTwoBilinear<2>(imagePyramid[iLevel], upsampledImage, slowestScratch);
            ImageProcessing::UpsampleByPowerOfTwoBilinear<2>(dogPyramid[iLevel], upsampledDog, slowestScratch);
            EndBenchmark("ecvcsB_scale_upsample");
          } else if(iLevel == 3) {
            BeginBenchmark("ecvcsB_scale_upsample");
            upsampledImage = upsampledImageTmp;
            upsampledDog = upsampledDogTmp;
            ImageProcessing::UpsampleByPowerOfTwoBilinear<3>(imagePyramid[iLevel], upsampledImage, slowestScratch);
            ImageProcessing::UpsampleByPowerOfTwoBilinear<3>(dogPyramid[iLevel], upsampledDog, slowestScratch);
            EndBenchmark("ecvcsB_scale_upsample");
          } else if(iLevel == 4) {
            BeginBenchmark("ecvcsB_scale_upsample");
            upsampledImage = upsampledImageTmp;
            upsampledDog = upsampledDogTmp;
            ImageProcessing::UpsampleByPowerOfTwoBilinear<4>(imagePyramid[iLevel], upsampledImage, slowestScratch);
            ImageProcessing::UpsampleByPowerOfTwoBilinear<4>(dogPyramid[iLevel], upsampledDog, slowestScratch);
            EndBenchmark("ecvcsB_scale_upsample");
          } else if(iLevel == 5) {
            BeginBenchmark("ecvcsB_scale_upsample");
            upsampledImage = upsampledImageTmp;
            upsampledDog = upsampledDogTmp;
            ImageProcessing::UpsampleByPowerOfTwoBilinear<5>(imagePyramid[iLevel], upsampledImage, slowestScratch);
            ImageProcessing::UpsampleByPowerOfTwoBilinear<5>(dogPyramid[iLevel], upsampledDog, slowestScratch);
            EndBenchmark("ecvcsB_scale_upsample");
          }

          BeginBenchmark("ecvcsB_scale_select");
          for(s32 yBig=0; yBig<imageHeight; yBig++) {
            const u8 * restrict pImage = upsampledImage.Pointer(yBig, 0);
            const u8 * restrict pDog = upsampledDog.Pointer(yBig, 0);

            u8 * restrict pDogMax = dogMax.Pointer(yBig, 0);
            u8 * restrict pScaleImage = scaleImage.Pointer(yBig, 0);

            for(s32 xBig=0; xBig<imageWidth; xBig++) {
              const u8 curDog = pDog[xBig];
              const u8 curDogMax = pDogMax[xBig];

              if(curDog > curDogMax) {
                pDogMax[xBig] = curDog;
                pScaleImage[xBig] = pImage[xBig];
              }
            } // for(s32 xBig=0; xBig<imageWidth; xBig++)
          } // for(s32 yBig=0; yBig<imageHeight; yBig++)
          EndBenchmark("ecvcsB_scale_select");

          //#if ANKICORETECH_EMBEDDED_USE_OPENCV
          //char name[1024];
          //snprintf(name, 1024, "dogMax"); dogMax.Show(name, false, false, true);
          //snprintf(name, 1024, "scaleImage"); scaleImage.Show(name, false, false, true);
          //cv::waitKey();
          //#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV
        } // for(s32 iLevel=0; iLevel<numPyramidLevels; iLevel++)
      } else { // if(upsampleToFullSize)
        for(s32 iLevel=0; iLevel<numPyramidLevels; iLevel++) {
          const s32 xyOutputStep = 1 << iLevel;

          //const s32 scaledHeight = imageHeight >> iLevel;
          const s32 scaledWidth = imageWidth >> iLevel;

          BeginBenchmark("ecvcsB_scale_select");

          for(s32 yBig=0; yBig<imageHeight; yBig++) {
            const u8 * restrict pImage = imagePyramid[iLevel].Pointer(yBig / xyOutputStep, 0);
            const u8 * restrict pDog = dogPyramid[iLevel].Pointer(yBig / xyOutputStep, 0);

            u8 * restrict pDogMax = dogMax.Pointer(yBig, 0);
            u8 * restrict pScaleImage = scaleImage.Pointer(yBig, 0);

            s32 xBig = 0;
            for(s32 xSmall=0; xSmall<scaledWidth; xSmall++) {
              const u8 curDog = pDog[xSmall];

              for(s32 dx=0; dx<xyOutputStep; dx++) {
                const u8 curDogMax = pDogMax[xBig];

                if(curDog > curDogMax) {
                  pDogMax[xBig] = curDog;
                  pScaleImage[xBig] = pImage[xSmall];
                }

                xBig++;
              } // for(s32 dx=0; dx<xyOutputStep; dx++)
            } // for(s32 xSmall=0; xSmall<scaledWidth; xSmall++)
          } // for(s32 yBig=0; yBig<imageHeight; yBig++)

          EndBenchmark("ecvcsB_scale_select");

          //#if ANKICORETECH_EMBEDDED_USE_OPENCV
          //          char name[1024];
          //          snprintf(name, 1024, "dogMax"); dogMax.Show(name, false, false, true);
          //          snprintf(name, 1024, "scaleImage"); scaleImage.Show(name, false, false, true);
          //          cv::waitKey();
          //#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV
        } // for(s32 iLevel=0; iLevel<numPyramidLevels; iLevel++)
      } // if(upsampleToFullSize) .. else

      EndBenchmark("ecvcsB_scale");

      Array<u8> binaryImageTmp(imageHeight, imageWidth, slowestScratch);

      BeginBenchmark("ecvcsB_binarize");
      for(s32 y=0; y<imageHeight; y++) {
        const u8 * restrict pImage = imagePyramid[0].Pointer(y, 0);
        const u8 * restrict pScaleImage = scaleImage.Pointer(y, 0);
        u8 * restrict pBinaryImageTmp = binaryImageTmp.Pointer(y,0);

        for(s32 x=0; x<imageWidth; x++) {
          const s32 thresholdValue = (pScaleImage[x]*scaleImage_thresholdMultiplier) >> thresholdMultiplier_numFractionalBits;

          if(pImage[x] < thresholdValue) {
            pBinaryImageRow[x] = 1;
          } else {
            pBinaryImageRow[x] = 0;
          }

          pBinaryImageTmp[x] = 255*pBinaryImageRow[x];
        } // for(s32 x=0; x<imageWidth; x++)

        if((lastResult = components.Extract2dComponents_PerRow_NextRow(pBinaryImageRow, imageWidth, y, component1d_minComponentWidth, component1d_maxSkipDistance)) != RESULT_OK)
          return lastResult;
      } // for(s32 y=0; y<imageHeight; y++)

      EndBenchmark("ecvcsB_binarize");

      //#if ANKICORETECH_EMBEDDED_USE_OPENCV
      //      binaryImageTmp.Show("binaryImageTmp", true, false, true);
      //#endif

      //BeginBenchmark("ecvcsB_finalize");
      if((lastResult = components.Extract2dComponents_PerRow_Finalize()) != RESULT_OK)
        return lastResult;
      //EndBenchmark("ecvcsB_finalize");

      return RESULT_OK;
    } // ExtractComponentsViaCharacteristicScale
  } // namespace Embedded
} // namespace Anki
