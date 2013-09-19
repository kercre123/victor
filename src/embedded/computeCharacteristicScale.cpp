#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
#define MAX_PYRAMID_LEVELS 8
#define MAX_ALPHAS 128
    // //function scaleImage = computeCharacteristicScaleImage_loopsAndFixedPoint(image, numPyramidLevels, computeDogAtFullSize, filterWithMex)
    IN_DDR Result ComputeCharacteristicScaleImage(const Array<u8> &image, const s32 numPyramidLevels, Array<u32> &scaleImage, MemoryStack scratch)
    {
      //double times[20];

      //times[0] = GetTime();

      AnkiConditionalErrorAndReturnValue(image.IsValid(),
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "image is not valid");

      AnkiConditionalErrorAndReturnValue(scaleImage.IsValid(),
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "scaleImage is not valid");

      AnkiConditionalErrorAndReturnValue(numPyramidLevels <= MAX_PYRAMID_LEVELS,
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "numPyramidLevels must be less than %d", MAX_PYRAMID_LEVELS+1);

      AnkiConditionalErrorAndReturnValue(image.get_size(0) == scaleImage.get_size(0) && image.get_size(1) == scaleImage.get_size(1),
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "image and scaleImage must be the same size");

      // TODO: add check for the required amount of scratch?

      const s32 fullSizeHeight = image.get_size(0);
      const s32 fullSizeWidth = image.get_size(1);

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
        Array<u8> curPyramidLevelBlurred(curPyramidLevel.get_size(0), curPyramidLevel.get_size(1), scratch);

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
          const s32 maxAlphaSquaredShift = static_cast<s32>(Log2(static_cast<u32>(maxAlpha*maxAlpha)));

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
          const s64 maxAlphaSquaredShift = static_cast<s64>(Log2(static_cast<u64>(maxAlpha*maxAlpha)));

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

    //end % FUNCTION computeCharacteristicScaleImage()
    //
    //% pixel* are UQ8.0
    //% alpha* are UQ?.? depending on the level of the pyramid
    //function interpolatedPixel = interpolate2d(pixel00, pixel01, pixel10, pixel11, alphaY, alphaYinverse, alphaX, alphaXinverse)
    //    interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
    //    interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;
    //
    //    interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;
    //end
  } // namespace Embedded
} // namespace Anki