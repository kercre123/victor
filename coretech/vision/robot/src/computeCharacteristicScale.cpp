#include "anki/common/robot/utilities.h"
#include "anki/common/robot/benchmarking_c.h"

#include "anki/vision/robot/miscVisionKernels.h"
#include "anki/vision/robot/integralImage.h"

namespace Anki
{
  namespace Embedded
  {
#define MAX_PYRAMID_LEVELS 8
#define MAX_ALPHAS 128
    // //function scaleImage = computeCharacteristicScaleImage_loopsAndFixedPoint(image, numPyramidLevels, computeDogAtFullSize, filterWithMex)
    IN_DDR Result ComputeCharacteristicScaleImage(const Array<u8> &image, const s32 numPyramidLevels, FixedPointArray<u32> &scaleImage, MemoryStack scratch)
    {
      //double times[20];

      //times[0] = GetTime();

      const s32 imageHeight = image.get_size(0);
      const s32 fullSizeHeight = imageHeight;

      const s32 imageWidth = image.get_size(1);
      const s32 fullSizeWidth = imageWidth;

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "image is not valid");

      AnkiConditionalErrorAndReturnValue(scaleImage.IsValid(),
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "scaleImage is not valid");

      AnkiConditionalErrorAndReturnValue(numPyramidLevels <= MAX_PYRAMID_LEVELS,
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "numPyramidLevels must be less than %d", MAX_PYRAMID_LEVELS+1);

      AnkiConditionalErrorAndReturnValue(imageHeight == scaleImage.get_size(0) && imageWidth == scaleImage.get_size(1),
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "image and scaleImage must be the same size");

      AnkiConditionalErrorAndReturnValue(scaleImage.get_numFractionalBits() == 16,
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "scaleImage must be UQ16.16");

      // TODO: add check for the required amount of scratch?

      //scaleImage = uint32(image)*(2^8); % UQ16.16
      //dogMax = zeros(fullSizeHeight,fullSizeWidth,'uint32'); % UQ16.16
      Array<u32> dogMax(fullSizeHeight, fullSizeWidth, scratch); // UQ16.16
      dogMax.Set(static_cast<u32>(0));

      //imagePyramid = cell(1, numPyramidLevels+1);
      Array<u8> imagePyramid[MAX_PYRAMID_LEVELS+1];
      imagePyramid[0] = Array<u8>(fullSizeHeight, fullSizeWidth, scratch, false);
      for(s32 i=1; i<=numPyramidLevels; i++) {
        imagePyramid[i] = Array<u8>(fullSizeHeight >> i, fullSizeWidth >> i, scratch, false);
      }

      //scaleFactors = int32(2.^[0:(numPyramidLevels)]); %#ok<NBRAK>
      s32 scaleFactors[MAX_PYRAMID_LEVELS+1];
      scaleFactors[0] = 1;
      scaleFactors[1] = 2;
      for(s32 i=2; i<=numPyramidLevels; i++) {
        scaleFactors[i] = 2 << (i-1);
      }

      //imagePyramid{1} = mexBinomialFilter(image);
      const Result binomialFilterResult = BinomialFilter(image, imagePyramid[0], scratch);

      AnkiConditionalErrorAndReturnValue(binomialFilterResult == RESULT_OK,
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "BinomialFilter failed");

      /*{
      Matlab matlab(false);

      matlab.PutArray2d(image, "image");
      matlab.PutArray2d(imagePyramid[0], "imagePyramid");
      }*/

      /*
      image.Show("image", false);
      imagePyramid[0].Show("imagePyramid[0]", true);
      */

      //for pyramidLevel = 2:numPyramidLevels+1
      for(s32 pyramidLevel=1; pyramidLevel<=numPyramidLevels; pyramidLevel++) {
        PUSH_MEMORY_STACK(scratch); // Push the current state of the scratch buffer onto the system stack

        //    curPyramidLevel = imagePyramid{pyramidLevel-1}; %UQ8.0
        const Array<u8> curPyramidLevel = imagePyramid[pyramidLevel-1]; // UQ8.0
        const s32 curLevelHeight = curPyramidLevel.get_size(0);
        const s32 curLevelWidth = curPyramidLevel.get_size(1);

        //    if filterWithMex
        //        curPyramidLevelBlurred = mexBinomialFilter(curPyramidLevel); %UQ8.0
        //    else
        //        curPyramidLevelBlurred = binomialFilter_loopsAndFixedPoint(curPyramidLevel); %UQ8.0
        //    end
        Array<u8> curPyramidLevelBlurred(curLevelHeight, curLevelWidth, scratch);

        const Result binomialFilterResult = BinomialFilter(curPyramidLevel, curPyramidLevelBlurred, scratch);

        AnkiConditionalErrorAndReturnValue(binomialFilterResult == RESULT_OK,
          RESULT_FAIL, "ComputeCharacteristicScaleImage", "In-loop BinomialFilter failed");

        //    largeDoG = zeros([fullSizeHeight,fullSizeWidth],'uint32'); % UQ16.16
#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
        Array<u32> largeDog(fullSizeHeight, fullSizeWidth, scratch);
        largeDog.Set(u32(0));
#endif

        //    if pyramidLevel == 2
        if(pyramidLevel == 1) {
          // The first level is always correct, so just set everything in dogMax and scaleImage
          //        for y = 1:fullSizeHeight
          //            for x = 1:fullSizeWidth
          for(s32 y=0; y<curLevelHeight; y++) {
            const u8 * restrict curPyramidLevel_rowPointer = curPyramidLevel.Pointer(y, 0);
            const u8 * restrict curPyramidLevelBlurred_rowPointer = curPyramidLevelBlurred.Pointer(y, 0);

            u32 * restrict dogMax_rowPointer = dogMax.Pointer(y, 0);
            u32 * restrict scaleImage_rowPointer = scaleImage.Pointer(y, 0);

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
            u32 * restrict largeDog_rowPointer = largeDog.Pointer(y, 0);
#endif

            for(s32 x=0; x<curLevelWidth; x++) {
              // DoG = uint32(abs(int32(curPyramidLevelBlurred(y,x)) - int32(curPyramidLevel(y,x)))) * (2^16); % abs(UQ8.0 - UQ8.0) -> UQ16.16;
              const u32 dog = static_cast<u32>(abs(static_cast<s16>(curPyramidLevelBlurred_rowPointer[x]) - static_cast<s16>(curPyramidLevel_rowPointer[x]))) << 16; // abs(UQ8.0 - UQ8.0) -> UQ16.16;

              // largeDoG(y,x) = DoG;
#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
              largeDog_rowPointer[x] = dog;
#endif

              // dogMax(y,x) = DoG;
              // scaleImage(y,x) = uint32(curPyramidLevel(y,x)) * (2^16); % UQ8.0 -> UQ16.16
              dogMax_rowPointer[x] = dog;
              scaleImage_rowPointer[x] = static_cast<u32>(curPyramidLevel_rowPointer[x]) << 16; // UQ8.0 -> UQ16.16
            }
          }

          //{
          //  Matlab matlab(false);

          //  matlab.PutArray2d(curPyramidLevel, "curPyramidLevel_c");
          //  matlab.PutArray2d(curPyramidLevelBlurred, "curPyramidLevelBlurred_c");
          //  matlab.PutArray2d(dogMax, "dogMax_c");
          //  matlab.PutArray2d(scaleImage, "scaleImage_c");
          //  matlab.PutArray2d(largeDog, "largeDog_c");
          //  printf("\n");
          //}
        } else if (pyramidLevel < 5) { //    elseif pyramidLevel <= 5 % if pyramidLevel == 2
          //        largeYIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, fullSizeHeight-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
          //        largeXIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, fullSizeWidth-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
          const s32 largeIndexMin = scaleFactors[pyramidLevel-1] / 2; // SQ31.0

          const s32 numAlphas = scaleFactors[pyramidLevel-1];

          //        alphas = int32((2^8)*(.5 + double(0:(scaleFactors(pyramidLevel-1)-1)))); % SQ23.8
          s32 alphas[MAX_ALPHAS]; // SQ23.8
          for(s32 iAlpha=0; iAlpha<numAlphas; iAlpha++) {
            alphas[iAlpha] = (static_cast<s32>(iAlpha)<<8) + (1<<7); // SQ23.8
            //printf("%d) %d\n",iAlpha, alphas[iAlpha]);
          }

          // maxAlpha = int32((2^8)*scaleFactors(pyramidLevel-1)); % SQ31.0 -> SQ23.8
          const s32 maxAlpha = scaleFactors[pyramidLevel-1] << 8; // SQ31.0 -> SQ23.8
          const s32 maxAlphaSquaredShift = static_cast<s32>(Log2u32(static_cast<u32>(maxAlpha*maxAlpha)));

          //        largeY = largeYIndexRange(1);
          s32 largeY = largeIndexMin;
          for(s32 smallY = 0; smallY < (curLevelHeight-1); smallY++) { //        for smallY = 1:(size(curPyramidLevel,1)-1)
            const u8 * restrict curPyramidLevel_rowPointerY0 = curPyramidLevel.Pointer(smallY,   0);
            const u8 * restrict curPyramidLevel_rowPointerYp1 = curPyramidLevel.Pointer(smallY+1, 0);

            const u8 * restrict curPyramidLevelBlurred_rowPointerY0 = curPyramidLevelBlurred.Pointer(smallY,   0);
            const u8 * restrict curPyramidLevelBlurred_rowPointerYp1 = curPyramidLevelBlurred.Pointer(smallY+1, 0);

            for(s32 iAlphaY = 0; iAlphaY < numAlphas; iAlphaY++, largeY++) { //            for iAlphaY = 1:length(alphas)
              u32 * restrict dogMax_rowPointer = dogMax.Pointer(largeY, 0);
              u32 * restrict scaleImage_rowPointer = scaleImage.Pointer(largeY, 0);

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
              u32 * restrict largeDog_rowPointer = largeDog.Pointer(largeY, 0);
#endif

              //alphaY = alphas(iAlphaY);
              //alphaYinverse = maxAlpha - alphas(iAlphaY);
              const s32 alphaY = alphas[iAlphaY];
              const s32 alphaYinverse = maxAlpha - alphaY;

              // largeX = largeXIndexRange(1);
              s32 largeX = largeIndexMin;
              for(s32 smallX = 0; smallX < (curLevelWidth-1); smallX++) { //                for smallX = 1:(size(curPyramidLevel,2)-1)
                //curPyramidLevel_00 = int32(curPyramidLevel(smallY, smallX)); % SQ31.0
                //curPyramidLevel_01 = int32(curPyramidLevel(smallY, smallX+1)); % SQ31.0
                //curPyramidLevel_10 = int32(curPyramidLevel(smallY+1, smallX)); % SQ31.0
                //curPyramidLevel_11 = int32(curPyramidLevel(smallY+1, smallX+1)); % SQ31.0
                const s32 curPyramidLevel_00 = static_cast<s32>(curPyramidLevel_rowPointerY0[smallX]); // SQ31.0
                const s32 curPyramidLevel_01 = static_cast<s32>(curPyramidLevel_rowPointerY0[smallX+1]); // SQ31.0
                const s32 curPyramidLevel_10 = static_cast<s32>(curPyramidLevel_rowPointerYp1[smallX]); // SQ31.0
                const s32 curPyramidLevel_11 = static_cast<s32>(curPyramidLevel_rowPointerYp1[smallX+1]); // SQ31.0

                //DoG_00 = abs(int32(curPyramidLevel(smallY,   smallX))   - int32(curPyramidLevelBlurred(smallY,   smallX))); % SQ31.0
                //DoG_01 = abs(int32(curPyramidLevel(smallY,   smallX+1)) - int32(curPyramidLevelBlurred(smallY,   smallX+1))); % SQ31.0
                //DoG_10 = abs(int32(curPyramidLevel(smallY+1, smallX))   - int32(curPyramidLevelBlurred(smallY+1, smallX))); % SQ31.0
                //DoG_11 = abs(int32(curPyramidLevel(smallY+1, smallX+1)) - int32(curPyramidLevelBlurred(smallY+1, smallX+1))); % SQ31.0
                const s32 dog_00 = abs(static_cast<s32>(curPyramidLevel_rowPointerY0[smallX])    - static_cast<s32>(curPyramidLevelBlurred_rowPointerY0[smallX])); // SQ31.0
                const s32 dog_01 = abs(static_cast<s32>(curPyramidLevel_rowPointerY0[smallX+1])  - static_cast<s32>(curPyramidLevelBlurred_rowPointerY0[smallX+1])); // SQ31.0
                const s32 dog_10 = abs(static_cast<s32>(curPyramidLevel_rowPointerYp1[smallX])   - static_cast<s32>(curPyramidLevelBlurred_rowPointerYp1[smallX])); // SQ31.0
                const s32 dog_11 = abs(static_cast<s32>(curPyramidLevel_rowPointerYp1[smallX+1]) - static_cast<s32>(curPyramidLevelBlurred_rowPointerYp1[smallX+1])); // SQ31.0

                for(s32 iAlphaX = 0; iAlphaX < numAlphas; iAlphaX++, largeX++) { //                    for iAlphaX = 1:length(alphas)
                  //    alphaX = alphas(iAlphaX);
                  //    alphaXinverse = maxAlpha - alphas(iAlphaX);
                  const s32 alphaX = alphas[iAlphaX];
                  const s32 alphaXinverse = maxAlpha - alphaX;

                  // DoG = uint32( interpolate2d(DoG_00, DoG_01, DoG_10, DoG_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) ); % SQ?.? -> UQ16.16
                  const u32 dog = static_cast<u32>(Interpolate2d(dog_00, dog_01, dog_10, dog_11, alphaY, alphaYinverse, alphaX, alphaXinverse) >> (maxAlphaSquaredShift - 16)); // SQ?.? -> UQ16.16

                  //printf("(%d,%d) %d %d %d %d %d %d\n", largeY, largeX, dog_00, dog_01, dog_10, dog_11, Interpolate2d(dog_00, dog_01, dog_10, dog_11, alphaY, alphaYinverse, alphaX, alphaXinverse), dog);

                  //    largeDoG(largeY,largeX) = DoG;
#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
                  largeDog_rowPointer[largeX] = dog;
#endif

                  //    if DoG > dogMax(largeY,largeX)
                  if(dog > dogMax_rowPointer[largeX]) {
                    //        dogMax(largeY,largeX) = DoG;
                    //        curPyramidLevelInterpolated = uint32( interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) );  % SQ?.? -> UQ16.16
                    //        scaleImage(largeY,largeX) = curPyramidLevelInterpolated;
                    dogMax_rowPointer[largeX] = dog;
                    const s32 interpolatedUnscaled = Interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse);
                    const u32 curPyramidLevelInterpolated = static_cast<u32>(interpolatedUnscaled >> (maxAlphaSquaredShift - 16)); // SQ?.? -> UQ16.16
                    scaleImage_rowPointer[largeX] = curPyramidLevelInterpolated;
                  }
                } // for(s32 iAlphaX = 0; iAlphaX < numAlphas; iAlphaX++)
              }// for(s32 smallX = 0; smallX < curLevelHeight; smallX++)
            } // for(s32 iAlphaY = 0; iAlphaY < numAlphas; iAlphaY++)
          } // for(s32 smallY = 0; smallY < curLevelHeight; smallY++)

          /*{
          Matlab matlab(false);

          matlab.PutArray2d(curPyramidLevel, "curPyramidLevel_c");
          matlab.PutArray2d(curPyramidLevelBlurred, "curPyramidLevelBlurred_c");
          matlab.PutArray2d(dogMax, "dogMax_c");
          matlab.PutArray2d(scaleImage, "scaleImage_c");
          matlab.PutArray2d(largeDog, "largeDog_c");
          printf("\n");
          }*/
        } else {         //    else % if pyramidLevel == 2 ... elseif pyramidLevel <= 5
          const s32 largeIndexMin = scaleFactors[pyramidLevel-1] / 2; // SQ31.0

          const s32 numAlphas = scaleFactors[pyramidLevel-1];

          s64 alphas[MAX_ALPHAS]; // SQ55.8
          for(s32 iAlpha=0; iAlpha<numAlphas; iAlpha++) {
            alphas[iAlpha] = (static_cast<s64>(iAlpha)<<8) + (1<<7); // SQ55.8
          }

          const s64 maxAlpha = scaleFactors[pyramidLevel-1] << 8; // SQ31.0 -> SQ55.8
          const s64 maxAlphaSquaredShift = static_cast<s64>(Log2u64(static_cast<u64>(maxAlpha*maxAlpha)));

          s32 largeY = largeIndexMin;
          for(s32 smallY = 0; smallY < (curLevelHeight-1); smallY++) { //        for smallY = 1:(size(curPyramidLevel,1)-1)
            const u8 * restrict curPyramidLevel_rowPointerY0 = curPyramidLevel.Pointer(smallY,   0);
            const u8 * restrict curPyramidLevel_rowPointerYp1 = curPyramidLevel.Pointer(smallY+1, 0);

            const u8 * restrict curPyramidLevelBlurred_rowPointerY0 = curPyramidLevelBlurred.Pointer(smallY,   0);
            const u8 * restrict curPyramidLevelBlurred_rowPointerYp1 = curPyramidLevelBlurred.Pointer(smallY+1, 0);

            for(s32 iAlphaY = 0; iAlphaY < numAlphas; iAlphaY++, largeY++) { //            for iAlphaY = 1:length(alphas)
              u32 * restrict dogMax_rowPointer = dogMax.Pointer(largeY, 0);
              u32 * restrict scaleImage_rowPointer = scaleImage.Pointer(largeY, 0);

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
              u32 * restrict largeDog_rowPointer = largeDog.Pointer(largeY, 0);
#endif

              const s64 alphaY = alphas[iAlphaY];
              const s64 alphaYinverse = maxAlpha - alphaY;

              s32 largeX = largeIndexMin;
              for(s32 smallX = 0; smallX < (curLevelWidth-1); smallX++) { //                for smallX = 1:(size(curPyramidLevel,2)-1)
                const s64 curPyramidLevel_00 = static_cast<s64>(curPyramidLevel_rowPointerY0[smallX]); // SQ63.0
                const s64 curPyramidLevel_01 = static_cast<s64>(curPyramidLevel_rowPointerY0[smallX+1]); // SQ63.0
                const s64 curPyramidLevel_10 = static_cast<s64>(curPyramidLevel_rowPointerYp1[smallX]); // SQ63.0
                const s64 curPyramidLevel_11 = static_cast<s64>(curPyramidLevel_rowPointerYp1[smallX+1]); // SQ63.0

                const s64 dog_00 = abs(static_cast<s64>(curPyramidLevel_rowPointerY0[smallX])    - static_cast<s64>(curPyramidLevelBlurred_rowPointerY0[smallX])); // SQ63.0
                const s64 dog_01 = abs(static_cast<s64>(curPyramidLevel_rowPointerY0[smallX+1])  - static_cast<s64>(curPyramidLevelBlurred_rowPointerY0[smallX+1])); // SQ63.0
                const s64 dog_10 = abs(static_cast<s64>(curPyramidLevel_rowPointerYp1[smallX])   - static_cast<s64>(curPyramidLevelBlurred_rowPointerYp1[smallX])); // SQ63.0
                const s64 dog_11 = abs(static_cast<s64>(curPyramidLevel_rowPointerYp1[smallX+1]) - static_cast<s64>(curPyramidLevelBlurred_rowPointerYp1[smallX+1])); // SQ63.0

                for(s32 iAlphaX = 0; iAlphaX < numAlphas; iAlphaX++, largeX++) { //                    for iAlphaX = 1:length(alphas)
                  const s64 alphaX = alphas[iAlphaX];
                  const s64 alphaXinverse = maxAlpha - alphaX;

                  const u32 dog = static_cast<u32>(Interpolate2d(dog_00, dog_01, dog_10, dog_11, alphaY, alphaYinverse, alphaX, alphaXinverse) >> (maxAlphaSquaredShift - 16)); // SQ?.? -> UQ16.16

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
                  largeDog_rowPointer[largeX] = dog;
#endif

                  /* if(largeY==9 && largeX==9){
                  printf("\n");
                  }*/
                  if(dog > dogMax_rowPointer[largeX]) {
                    dogMax_rowPointer[largeX] = dog;
                    const s64 interpolatedUnscaled = Interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse);
                    //const u32 curPyramidLevelInterpolated = static_cast<u32>(Interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) >> (maxAlphaSquaredShift + 16)); // SQ?.? -> UQ16.16
                    const u32 curPyramidLevelInterpolated = static_cast<u32>(interpolatedUnscaled >> (maxAlphaSquaredShift - 16)); // SQ?.? -> UQ16.16
                    scaleImage_rowPointer[largeX] = curPyramidLevelInterpolated;
                    //scaleImage_rowPointer[largeX] = 10000;
                  }
                } // for(s32 iAlphaX = 0; iAlphaX < numAlphas; iAlphaX++)
              }// for(s32 smallX = 0; smallX < curLevelHeight; smallX++)
            } // for(s32 iAlphaY = 0; iAlphaY < numAlphas; iAlphaY++)
          } // for(s32 smallY = 0; smallY < curLevelHeight; smallY++)
          /*
          {
          Matlab matlab(false);

          matlab.PutArray2d(curPyramidLevel, "curPyramidLevel_c");
          matlab.PutArray2d(curPyramidLevelBlurred, "curPyramidLevelBlurred_c");
          matlab.PutArray2d(dogMax, "dogMax_c");
          matlab.PutArray2d(scaleImage, "scaleImage_c");
          matlab.PutArray2d(largeDog, "largeDog_c");
          printf("\n");
          }*/
        }   //    end % if k == 2 ... else

        //    imagePyramid{pyramidLevel} = uint8(downsampleByFactor(curPyramidLevelBlurred, 2));

        const Result downsampleByFactorResult = DownsampleByFactor(curPyramidLevelBlurred, 2, imagePyramid[pyramidLevel]);
        AnkiConditionalErrorAndReturnValue(downsampleByFactorResult == RESULT_OK,
          RESULT_FAIL, "ComputeCharacteristicScaleImage", "In-loop DownsampleByFactor failed");
      } // for(s32 pyramidLevel=1; pyramidLevel<=numPyramidLevels; pyramidLevel++) {
      //times[19] = GetTime();

      //printf("t20-t0 = %f\n", times[19]-times[0]);

      return RESULT_OK;
    } // Result ComputeCharacteristicScaleImage(const Array<u8> &image, s32 numPyramidLevels, Array<u32> &scaleImage, MemoryStack scratch)

    IN_DDR Result ComputeCharacteristicScaleImageAndBinarize(const Array<u8> &image, const s32 numPyramidLevels, Array<u8> &binaryImage, const s32 thresholdMultiplier, MemoryStack scratch)
    {
      const s32 thresholdMultiplier_numFractionalBits = 16;

      // Integral image constants
      const s32 numBorderPixels = 33; // 2^5 + 1 = 33
      const s32 integralImageHeight = 96; // 96*(640+33*2)*4 = 271104, though with padding, it is 96*720*4 = 276480
      const s32 numRowsToScroll = integralImageHeight - 2*numBorderPixels;

      // To normalize a sum of 1 / ((2*n+1)^2), we approximate a division as a mulitiply and shift.
      // These multiply coefficients were computed in matlab by typing:
      // " for i=1:5 disp(sprintf('i:%d',i')); computeClosestMultiplicationShift((2*(2^i)+1)^2, 31, 8); disp(sprintf('\n')); end; "
      const s32 normalizationMultiply[5] = {41, 101, 227, 241, 31}; // For halfWidths in [1,5]
      const s32 normalizationBitShifts[5] = {10, 13, 16, 18, 17};

      const s32 maxFilterHalfWidth = 1 << (numPyramidLevels+1);

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      C_Acceleration acceleration;
      acceleration.type = C_ACCELERATION_NATURAL_CPP;
      acceleration.version = 1;

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL, "ComputeCharacteristicScaleImageAndBinarize", "image is not valid");

      AnkiConditionalErrorAndReturnValue(binaryImage.IsValid(),
        RESULT_FAIL, "ComputeCharacteristicScaleImageAndBinarize", "binaryImage is not valid");

      AnkiConditionalErrorAndReturnValue(numPyramidLevels <= 4 && numPyramidLevels > 0,
        RESULT_FAIL, "ComputeCharacteristicScaleImageAndBinarize", "numPyramidLevels must be less than %d", 4+1);

      // TODO: support numPyramidLevels==5 ?
      /*AnkiConditionalWarn(numPyramidLevels <= 4,
      "ComputeCharacteristicScaleImageAndBinarize", "If numPyramidLevels is greater than 4, and mean value of any large rectangle is above 252, then this function will overflow.");*/

      AnkiConditionalErrorAndReturnValue(imageHeight == binaryImage.get_size(0) && imageWidth == binaryImage.get_size(1),
        RESULT_FAIL, "ComputeCharacteristicScaleImageAndBinarize", "image and binaryImage must be the same size");

      // Initialize the first integralImageHeight rows of the integralImage
      ScrollingIntegralImage_u8_s32 integralImage(integralImageHeight, imageWidth, numBorderPixels, scratch);
      if(integralImage.ScrollDown(image, integralImageHeight, scratch) != RESULT_OK)
        return RESULT_FAIL;

      // Prepare the memory for the filtered rows for each level of the pyramid
      Array<s32> filteredRows[5];
      for(s32 i=0; i<=numPyramidLevels; i++) {
        filteredRows[i] = Array<s32>(1, imageWidth, scratch, false);
      }

      s32 imageY = 0;

      while(imageY < imageHeight) {
        // For the given row of the image, compute the blurred version for each level of the pyramid
        for(s32 pyramidLevel=0; pyramidLevel<=numPyramidLevels; pyramidLevel++) {
          const s32 filterHalfWidth = 1 << (pyramidLevel+1); // filterHalfWidth = 2^pyramidLevel;
          const Rectangle<s16> filter(-filterHalfWidth, filterHalfWidth, -filterHalfWidth, filterHalfWidth);

          // Compute the sums using the integral image
          integralImage.FilterRow(acceleration, filter, imageY, filteredRows[pyramidLevel]);

          // Normalize the sums
          s32 * restrict filteredRow_rowPointer = filteredRows[pyramidLevel][0];
          const s32 multiply = normalizationMultiply[pyramidLevel];
          const s32 shift = normalizationBitShifts[pyramidLevel];
          for(s32 x=0; x<imageWidth; x++) {
            filteredRow_rowPointer[x] = (filteredRow_rowPointer[x] * multiply) >> shift;
          }
        } // for(s32 pyramidLevel=0; pyramidLevel<=numLevels; pyramidLevel++)

        const s32 * restrict filteredRows_rowPointers[5];
        for(s32 pyramidLevel=0; pyramidLevel<=numPyramidLevels; pyramidLevel++) {
          filteredRows_rowPointers[pyramidLevel] = filteredRows[pyramidLevel][0];
        }

        const u8 * restrict image_rowPointer = image[imageY];
        u8 * restrict binaryImage_rowPointer = binaryImage[imageY];
        for(s32 x=0; x<imageWidth; x++) {
          s32 dogMax = s32_MIN;
          s32 scaleValue = -1;
          for(s32 pyramidLevel=0; pyramidLevel<numPyramidLevels; pyramidLevel++) {
            const s32 dog = ABS(filteredRows_rowPointers[pyramidLevel+1][x] - filteredRows_rowPointers[pyramidLevel][x]);

            if(dog > dogMax) {
              dogMax = dog;
              scaleValue = filteredRows_rowPointers[pyramidLevel+1][x];
            } // if(dog > dogMax)
          } // for(s32 pyramidLevel=0; pyramidLevel<numPyramidLevels; numPyramidLevels++)

          const s32 thresholdValue = (scaleValue*thresholdMultiplier) >> thresholdMultiplier_numFractionalBits;
          if(image_rowPointer[x] < thresholdValue) {
            binaryImage_rowPointer[x] = 1;
          } else {
            binaryImage_rowPointer[x] = 0;
          }
        } // for(s32 x=0; x<imageWidth; x++)

        imageY++;

        //% If we've reached the bottom of this integral image, scroll it up
        if(integralImage.get_maxRow(maxFilterHalfWidth) < imageY) {
          if(integralImage.ScrollDown(image, numRowsToScroll, scratch) != RESULT_OK)
            return RESULT_FAIL;
        }
      } // while(imageY < size(image,1)) {
      return RESULT_OK;
    } // ComputeCharacteristicScaleImageAndBinarize()

    Result ComputeCharacteristicScaleImageAndBinarizeAndExtractComponents(
      const Array<u8> &image,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      ConnectedComponents &components,
      MemoryStack scratch)
    {
      BeginBenchmark("ccsiabaec_init");
      const s32 thresholdMultiplier_numFractionalBits = 16;

      // Integral image constants
      const s32 numBorderPixels = 33; // 2^5 + 1 = 33
      const s32 integralImageHeight = 96; // 96*(640+33*2)*4 = 271104, though with padding, it is 96*720*4 = 276480
      const s32 numRowsToScroll = integralImageHeight - 2*numBorderPixels;

      // To normalize a sum of 1 / ((2*n+1)^2), we approximate a division as a mulitiply and shift.
      // These multiply coefficients were computed in matlab by typing:
      // " for i=1:5 disp(sprintf('i:%d',i')); computeClosestMultiplicationShift((2*(2^i)+1)^2, 31, 8); disp(sprintf('\n')); end; "
      const s32 normalizationMultiply[5] = {41, 101, 227, 241, 31}; // For halfWidths in [1,5]
      const s32 normalizationBitShifts[5] = {10, 13, 16, 18, 17};

      const s32 maxFilterHalfWidth = 1 << (scaleImage_numPyramidLevels+1);

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      C_Acceleration acceleration;
      acceleration.type = C_ACCELERATION_NATURAL_CPP;
      acceleration.version = 1;

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL, "ComputeCharacteristicScaleImageAndBinarize", "image is not valid");

      AnkiConditionalErrorAndReturnValue(scaleImage_numPyramidLevels <= 4 && scaleImage_numPyramidLevels > 0,
        RESULT_FAIL, "ComputeCharacteristicScaleImageAndBinarize", "scaleImage_numPyramidLevels must be less than %d", 4+1);

      // Initialize the first integralImageHeight rows of the integralImage
      ScrollingIntegralImage_u8_s32 integralImage(integralImageHeight, imageWidth, numBorderPixels, scratch);
      if(integralImage.ScrollDown(image, integralImageHeight, scratch) != RESULT_OK)
        return RESULT_FAIL;

      // Prepare the memory for the filtered rows for each level of the pyramid
      Array<s32> filteredRows[5];
      for(s32 i=0; i<=scaleImage_numPyramidLevels; i++) {
        filteredRows[i] = Array<s32>(1, imageWidth, scratch, false);
      }

      Array<u8> binaryImageRow(1, imageWidth, scratch);
      u8 * restrict binaryImageRow_rowPointer = binaryImageRow[0];

      if(components.Extract2dComponents_PerRow_Initialize() != RESULT_OK)
        return RESULT_FAIL;

      s32 imageY = 0;

      EndBenchmark("ccsiabaec_init");

      BeginBenchmark("ccsiabaec_mainLoop");
      while(imageY < imageHeight) {
        BeginBenchmark("ccsiabaec_filterRows");

        // For the given row of the image, compute the blurred version for each level of the pyramid
        for(s32 pyramidLevel=0; pyramidLevel<=scaleImage_numPyramidLevels; pyramidLevel++) {
          const s32 filterHalfWidth = 1 << (pyramidLevel+1); // filterHalfWidth = 2^pyramidLevel;
          const Rectangle<s16> filter(-filterHalfWidth, filterHalfWidth, -filterHalfWidth, filterHalfWidth);

          // Compute the sums using the integral image
          integralImage.FilterRow(acceleration, filter, imageY, filteredRows[pyramidLevel]);

          // Normalize the sums
          s32 * restrict filteredRow_rowPointer = filteredRows[pyramidLevel][0];
          const s32 multiply = normalizationMultiply[pyramidLevel];
          const s32 shift = normalizationBitShifts[pyramidLevel];
          for(s32 x=0; x<imageWidth; x++) {
            filteredRow_rowPointer[x] = (filteredRow_rowPointer[x] * multiply) >> shift;
          }
        } // for(s32 pyramidLevel=0; pyramidLevel<=numLevels; pyramidLevel++)

        const s32 * restrict filteredRows_rowPointers[5];
        for(s32 pyramidLevel=0; pyramidLevel<=scaleImage_numPyramidLevels; pyramidLevel++) {
          filteredRows_rowPointers[pyramidLevel] = filteredRows[pyramidLevel][0];
        }

        EndBenchmark("ccsiabaec_filterRows");

        BeginBenchmark("ccsiabaec_computeBinaryImage");

        const u8 * restrict image_rowPointer = image[imageY];

        for(s32 x=0; x<imageWidth; x++) {
          s32 dogMax = s32_MIN;
          s32 scaleValue = -1;
          for(s32 pyramidLevel=0; pyramidLevel<scaleImage_numPyramidLevels; pyramidLevel++) {
            const s32 dog = ABS(filteredRows_rowPointers[pyramidLevel+1][x] - filteredRows_rowPointers[pyramidLevel][x]);

            if(dog > dogMax) {
              dogMax = dog;
              scaleValue = filteredRows_rowPointers[pyramidLevel+1][x];
            } // if(dog > dogMax)
          } // for(s32 pyramidLevel=0; pyramidLevel<scaleImage_numPyramidLevels; scaleImage_numPyramidLevels++)

          const s32 thresholdValue = (scaleValue*scaleImage_thresholdMultiplier) >> thresholdMultiplier_numFractionalBits;
          if(image_rowPointer[x] < thresholdValue) {
            binaryImageRow_rowPointer[x] = 1;
          } else {
            binaryImageRow_rowPointer[x] = 0;
          }
        } // for(s32 x=0; x<imageWidth; x++)

        EndBenchmark("ccsiabaec_computeBinaryImage");

        BeginBenchmark("ccsiabaec_extractNextRowOfComponents");

        // Extract the next line of connected components
        if(components.Extract2dComponents_PerRow_NextRow(binaryImageRow_rowPointer, imageWidth, imageY, component1d_minComponentWidth, component1d_maxSkipDistance) != RESULT_OK)
          return RESULT_FAIL;

        EndBenchmark("ccsiabaec_extractNextRowOfComponents");

        BeginBenchmark("ccsiabaec_scrollIntegralImage");

        imageY++;

        //% If we've reached the bottom of this integral image, scroll it up
        if(integralImage.get_maxRow(maxFilterHalfWidth) < imageY) {
          if(integralImage.ScrollDown(image, numRowsToScroll, scratch) != RESULT_OK)
            return RESULT_FAIL;
        }

        EndBenchmark("ccsiabaec_scrollIntegralImage");
      } // while(imageY < size(image,1))

      EndBenchmark("ccsiabaec_mainLoop");

      BeginBenchmark("ccsiabaec_finalize");
      if(components.Extract2dComponents_PerRow_Finalize() != RESULT_OK)
        return RESULT_FAIL;
      EndBenchmark("ccsiabaec_finalize");

      return RESULT_OK;
    } // ComputeCharacteristicScaleImageAndBinarizeAndExtractComponents
  } // namespace Embedded
} // namespace Anki