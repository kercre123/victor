#include "anki/vision.h"

namespace Anki
{
#define MAX_PYRAMID_LEVELS 8
#define MAX_ALPHAS 128
  // //function scaleImage = computeCharacteristicScaleImage_loopsAndFixedPoint(img, numLevels, computeDogAtFullSize, filterWithMex)
  Result ComputeCharacteristicScaleImage(const Array2dUnmanaged<u8> &img, s32 numLevels, Array2dUnmanagedFixedPoint<u32> &scaleImage, MemoryStack scratch)
  {
    //double times[20];

    //times[0] = GetTime();

    DASConditionalErrorAndReturnValue(img.IsValid(),
      RESULT_FAIL, "ComputeCharacteristicScaleImage", "img is not valid");

    DASConditionalErrorAndReturnValue(scaleImage.IsValid(),
      RESULT_FAIL, "ComputeCharacteristicScaleImage", "scaleImage is not valid");

    DASConditionalErrorAndReturnValue(numLevels <= MAX_PYRAMID_LEVELS,
      RESULT_FAIL, "ComputeCharacteristicScaleImage", "numLevels must be less than %d", MAX_PYRAMID_LEVELS+1);

    DASConditionalErrorAndReturnValue(AreMatricesEqual_Size(img, scaleImage),
      RESULT_FAIL, "ComputeCharacteristicScaleImage", "img and scaleImage must be the same size");

    DASConditionalErrorAndReturnValue(scaleImage.get_numFractionalBits() == 16,
      RESULT_FAIL, "ComputeCharacteristicScaleImage", "scaleImage must be UQ16.16");

    // TODO: add check for the required amount of scratch?

    const s32 fullSizeHeight = img.get_size(0);
    const s32 fullSizeWidth = img.get_size(1);

    //scaleImage = uint32(img)*(2^8); % UQ16.16
    //dogMax = zeros(fullSizeHeight,fullSizeWidth,'uint32'); % UQ16.16
    Array2dUnmanagedFixedPoint<u32> dogMax(fullSizeHeight, fullSizeWidth, 16, scratch); // UQ16.16
    dogMax.Set(0);

    //imgPyramid = cell(1, numLevels+1);
    Array2dUnmanaged<u8> imgPyramid[MAX_PYRAMID_LEVELS+1];
    imgPyramid[0] = Array2dUnmanaged<u8>(fullSizeHeight, fullSizeWidth, scratch, false);
    for(s32 i=1; i<=numLevels; i++) {
      imgPyramid[i] = Array2dUnmanaged<u8>(fullSizeHeight >> i, fullSizeWidth >> i, scratch, false);
    }

    //scaleFactors = int32(2.^[0:(numLevels)]); %#ok<NBRAK>
    s32 scaleFactors[MAX_PYRAMID_LEVELS+1];
    scaleFactors[0] = 1;
    scaleFactors[1] = 2;
    for(s32 i=2; i<=numLevels; i++) {
      scaleFactors[i] = 2 << (i-1);
    }

    //imgPyramid{1} = mexBinomialFilter(img);
    DASConditionalErrorAndReturnValue(BinomialFilter(img, imgPyramid[0], scratch) == RESULT_OK,
      RESULT_FAIL, "ComputeCharacteristicScaleImage", "BinomialFilter failed");

    /*{
    Anki::Matlab matlab(false);

    matlab.PutArray2dUnmanaged(img, "img");
    matlab.PutArray2dUnmanaged(imgPyramid[0], "imgPyramid");
    }*/

    /*
    img.Show("img", false);
    imgPyramid[0].Show("imgPyramid[0]", true);
    */

    //for pyramidLevel = 2:numLevels+1
    for(s32 pyramidLevel=1; pyramidLevel<=numLevels; pyramidLevel++) {
      MemoryStack scratch(scratch); // Push the current state of the scratch buffer onto the system stack

      //    curPyramidLevel = imgPyramid{pyramidLevel-1}; %UQ8.0
      const Array2dUnmanaged<u8> curPyramidLevel = imgPyramid[pyramidLevel-1]; // UQ8.0
      const s32 curLevelHeight = curPyramidLevel.get_size(0);
      const s32 curLevelWidth = curPyramidLevel.get_size(1);

      //    if filterWithMex
      //        curPyramidLevelBlurred = mexBinomialFilter(curPyramidLevel); %UQ8.0
      //    else
      //        curPyramidLevelBlurred = binomialFilter_loopsAndFixedPoint(curPyramidLevel); %UQ8.0
      //    end
      Array2dUnmanaged<u8> curPyramidLevelBlurred(curPyramidLevel.get_size(0), curPyramidLevel.get_size(1), scratch);

      DASConditionalErrorAndReturnValue(BinomialFilter(curPyramidLevel, curPyramidLevelBlurred, scratch) == RESULT_OK,
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "In-loop BinomialFilter failed");

      //    largeDoG = zeros([fullSizeHeight,fullSizeWidth],'uint32'); % UQ16.16
#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_OFF
      Array2dUnmanaged<u32> largeDog(fullSizeHeight, fullSizeWidth, scratch);
      largeDog.Set(0);
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

#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_OFF
          u32 * restrict largeDog_rowPointer = largeDog.Pointer(y, 0);
#endif

          for(s32 x=0; x<curLevelWidth; x++) {
            // DoG = uint32(abs(int32(curPyramidLevelBlurred(y,x)) - int32(curPyramidLevel(y,x)))) * (2^16); % abs(UQ8.0 - UQ8.0) -> UQ16.16;
            const u32 dog = static_cast<u32>(abs(static_cast<s16>(curPyramidLevelBlurred_rowPointer[x]) - static_cast<s16>(curPyramidLevel_rowPointer[x]))) << 16; // abs(UQ8.0 - UQ8.0) -> UQ16.16;

            // largeDoG(y,x) = DoG;
#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_OFF
            largeDog_rowPointer[x] = dog;
#endif

            // dogMax(y,x) = DoG;
            // scaleImage(y,x) = uint32(curPyramidLevel(y,x)) * (2^16); % UQ8.0 -> UQ16.16
            dogMax_rowPointer[x] = dog;
            scaleImage_rowPointer[x] = static_cast<u32>(curPyramidLevel_rowPointer[x]) << 16; // UQ8.0 -> UQ16.16
          }
        }

        //{
        //  Anki::Matlab matlab(false);

        //  matlab.PutArray2dUnmanaged(curPyramidLevel, "curPyramidLevel_c");
        //  matlab.PutArray2dUnmanaged(curPyramidLevelBlurred, "curPyramidLevelBlurred_c");
        //  matlab.PutArray2dUnmanaged(dogMax, "dogMax_c");
        //  matlab.PutArray2dUnmanaged(scaleImage, "scaleImage_c");
        //  matlab.PutArray2dUnmanaged(largeDog, "largeDog_c");
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

#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_OFF
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
#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_OFF
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
        Anki::Matlab matlab(false);

        matlab.PutArray2dUnmanaged(curPyramidLevel, "curPyramidLevel_c");
        matlab.PutArray2dUnmanaged(curPyramidLevelBlurred, "curPyramidLevelBlurred_c");
        matlab.PutArray2dUnmanaged(dogMax, "dogMax_c");
        matlab.PutArray2dUnmanaged(scaleImage, "scaleImage_c");
        matlab.PutArray2dUnmanaged(largeDog, "largeDog_c");
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

#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_OFF
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

#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_OFF
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
        Anki::Matlab matlab(false);

        matlab.PutArray2dUnmanaged(curPyramidLevel, "curPyramidLevel_c");
        matlab.PutArray2dUnmanaged(curPyramidLevelBlurred, "curPyramidLevelBlurred_c");
        matlab.PutArray2dUnmanaged(dogMax, "dogMax_c");
        matlab.PutArray2dUnmanaged(scaleImage, "scaleImage_c");
        matlab.PutArray2dUnmanaged(largeDog, "largeDog_c");
        printf("\n");
        }*/
      }   //    end % if k == 2 ... else

      //    imgPyramid{pyramidLevel} = uint8(downsampleByFactor(curPyramidLevelBlurred, 2));

      DASConditionalErrorAndReturnValue(DownsampleByFactor(curPyramidLevelBlurred, 2, imgPyramid[pyramidLevel]) == RESULT_OK,
        RESULT_FAIL, "ComputeCharacteristicScaleImage", "In-loop DownsampleByFactor failed");
    } // for(s32 pyramidLevel=1; pyramidLevel<=numLevels; pyramidLevel++) {
    //times[19] = GetTime();

    //printf("t20-t0 = %f\n", times[19]-times[0]);

    return Anki::RESULT_OK;
  } // Result ComputeCharacteristicScaleImage(const Array2dUnmanaged<u8> &img, s32 numLevels, Array2dUnmanaged<u32> &scaleImage, MemoryStack scratch)

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
} // namespace Anki