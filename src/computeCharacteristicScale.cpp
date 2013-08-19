#include "anki/vision.h"

namespace Anki
{
#define MAX_PYRAMID_LEVELS 8
  // //function scaleImage = computeCharacteristicScaleImage_loopsAndFixedPoint(img, numLevels, computeDogAtFullSize, filterWithMex)
  Result ComputeCharacteristicScaleImage(const Matrix<u8> &img, u32 numLevels, FixedPointMatrix<u32> &scaleImage, MemoryStack scratch)
  {
    DASConditionalErrorAndReturnValue(numLevels <= MAX_PYRAMID_LEVELS,
      RESULT_FAIL, "ComputeCharacteristicScaleImage", "numLevels must be less than %d", MAX_PYRAMID_LEVELS+1);

    DASConditionalErrorAndReturnValue(AreMatricesEqual_Size(img, scaleImage),
      RESULT_FAIL, "ComputeCharacteristicScaleImage", "img and scaleImage must be the same size");

    DASConditionalErrorAndReturnValue(scaleImage.get_numFractionalBits() == 16,
      RESULT_FAIL, "ComputeCharacteristicScaleImage", "img and scaleImage must be the same size");

    // TODO: add check for the required amount of scratch?

    const u32 nrows = img.get_size(0);
    const u32 ncols = img.get_size(1);

    //scaleImage = uint32(img)*(2^8); % UQ16.16
    //dogMax = zeros(nrows,ncols,'uint32'); % UQ16.16
    Matrix<u32> dogMax(nrows, ncols, scratch); // UQ16.16
    dogMax.Set(0);

    //imgPyramid = cell(1, numLevels+1);
    Matrix<u8> imgPyramid[MAX_PYRAMID_LEVELS+1];
    imgPyramid[0] = Matrix<u8>(nrows, ncols, scratch, false);
    for(u32 i=1; i<=numLevels; i++) {
      imgPyramid[i] = Matrix<u8>(nrows >> i, ncols >> i, scratch, false);
    }

    //scaleFactors = int32(2.^[0:(numLevels)]); %#ok<NBRAK>
    s32 scaleFactors[MAX_PYRAMID_LEVELS+1];
    for(u32 i=0; i<=numLevels; i++) {
      scaleFactors[i] = powf(2.0f, static_cast<float>(i));
    }

    //    imgPyramid{1} = mexBinomialFilter(img);
    //
    //for pyramidLevel = 2:numLevels+1
    //    curPyramidLevel = imgPyramid{pyramidLevel-1}; %UQ8.0
    //    if filterWithMex
    //        curPyramidLevelBlurred = mexBinomialFilter(curPyramidLevel); %UQ8.0
    //    else
    //        curPyramidLevelBlurred = binomialFilter_loopsAndFixedPoint(curPyramidLevel); %UQ8.0
    //    end
    //    largeDoG = zeros([nrows,ncols],'uint32'); % UQ16.16
    //
    //    if pyramidLevel == 2
    //        for y = 1:nrows
    //            for x = 1:ncols
    //                DoG = uint32(abs(int32(curPyramidLevelBlurred(y,x)) - int32(curPyramidLevel(y,x)))) * (2^16); % (SQ15.16 - SQ15.16) -> UQ16.16
    //                largeDoG(y,x) = DoG;
    //
    //                % The first level is always correct
    //                dogMax(y,x) = DoG;
    //                scaleImage(y,x) = uint32(curPyramidLevel(y,x)) * (2^16); % UQ8.0 -> UQ16.16
    //
    //                % The first level is not always correct
    //            end
    //        end
    //    elseif pyramidLevel <= 5 % if pyramidLevel == 2
    //        largeYIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, nrows-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
    //        largeXIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, ncols-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
    //
    //        alphas = int32((2^8)*(.5 + double(0:(scaleFactors(pyramidLevel-1)-1)))); % SQ23.8
    //        maxAlpha = int32((2^8)*scaleFactors(pyramidLevel-1)); % SQ31.0 -> SQ23.8
    //
    //        largeY = largeYIndexRange(1);
    //        for smallY = 1:(size(curPyramidLevel,1)-1)
    //            for iAlphaY = 1:length(alphas)
    //                alphaY = alphas(iAlphaY);
    //                alphaYinverse = maxAlpha - alphas(iAlphaY);
    //
    //                largeX = largeXIndexRange(1);
    //
    //                for smallX = 1:(size(curPyramidLevel,2)-1)
    //                    if computeDogAtFullSize
    //                        curPyramidLevel_00 = int32(curPyramidLevel(smallY, smallX)); % SQ31.0
    //                        curPyramidLevel_01 = int32(curPyramidLevel(smallY, smallX+1)); % SQ31.0
    //                        curPyramidLevel_10 = int32(curPyramidLevel(smallY+1, smallX)); % SQ31.0
    //                        curPyramidLevel_11 = int32(curPyramidLevel(smallY+1, smallX+1)); % SQ31.0
    //
    //                        curPyramidLevelBlurred_00 = int32(curPyramidLevelBlurred(smallY, smallX)); % SQ31.0
    //                        curPyramidLevelBlurred_01 = int32(curPyramidLevelBlurred(smallY, smallX+1)); % SQ31.0
    //                        curPyramidLevelBlurred_10 = int32(curPyramidLevelBlurred(smallY+1, smallX)); % SQ31.0
    //                        curPyramidLevelBlurred_11 = int32(curPyramidLevelBlurred(smallY+1, smallX+1)); % SQ31.0
    //                    else
    //                        curPyramidLevel_00 = int32(curPyramidLevel(smallY, smallX)); % SQ31.0
    //                        curPyramidLevel_01 = int32(curPyramidLevel(smallY, smallX+1)); % SQ31.0
    //                        curPyramidLevel_10 = int32(curPyramidLevel(smallY+1, smallX)); % SQ31.0
    //                        curPyramidLevel_11 = int32(curPyramidLevel(smallY+1, smallX+1)); % SQ31.0
    //
    //                        DoG_00 = abs(int32(curPyramidLevel(smallY,   smallX))   - int32(curPyramidLevelBlurred(smallY,   smallX))); % SQ31.0
    //                        DoG_01 = abs(int32(curPyramidLevel(smallY,   smallX+1)) - int32(curPyramidLevelBlurred(smallY,   smallX+1))); % SQ31.0
    //                        DoG_10 = abs(int32(curPyramidLevel(smallY+1, smallX))   - int32(curPyramidLevelBlurred(smallY+1, smallX))); % SQ31.0
    //                        DoG_11 = abs(int32(curPyramidLevel(smallY+1, smallX+1)) - int32(curPyramidLevelBlurred(smallY+1, smallX+1))); % SQ31.0
    //                    end
    //
    //                    for iAlphaX = 1:length(alphas)
    //                        alphaX = alphas(iAlphaX);
    //                        alphaXinverse = maxAlpha - alphas(iAlphaX);
    //
    //                        if computeDogAtFullSize
    //                            curPyramidLevelInterpolated = interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
    //                            curPyramidLevelBlurredInterpolated = interpolate2d(curPyramidLevelBlurred_00, curPyramidLevelBlurred_01, curPyramidLevelBlurred_10, curPyramidLevelBlurred_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
    //                            DoG = uint32( abs(curPyramidLevelInterpolated - curPyramidLevelBlurredInterpolated) / (maxAlpha^2/(2^16)) ); % (SQ?.? - SQ?.?) -> UQ16.16
    //                        else
    //                            DoG = uint32( interpolate2d(DoG_00, DoG_01, DoG_10, DoG_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) ); % SQ?.? -> UQ16.16
    //                        end
    //
    //                        largeDoG(largeY,largeX) = DoG;
    //
    //                        if DoG > dogMax(largeY,largeX)
    //                            dogMax(largeY,largeX) = DoG;
    //                            curPyramidLevelInterpolated = uint32( interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) );  % SQ?.? -> UQ16.16
    //                            scaleImage(largeY,largeX) = curPyramidLevelInterpolated;
    //                        end
    //
    //                        largeX = largeX + 1;
    //                    end % for iAlphaX = 1:length(alphas)
    //                end % for smallX = 1:(size(curPyramidLevel,2)-1)
    //
    //                largeY = largeY + 1;
    //            end % for iAlphaY = 1:length(alphas)
    //        end % for smallY = 1:(size(curPyramidLevel,1)-1)
    //    else % if pyramidLevel == 2 ... elseif pyramidLevel <= 5
    //        largeYIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, nrows-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
    //        largeXIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, ncols-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
    //
    //        alphas = int64((2^8)*(.5 + double(0:(scaleFactors(pyramidLevel-1)-1)))); % SQ55.8
    //        maxAlpha = int64((2^8)*scaleFactors(pyramidLevel-1)); % SQ31.0 -> SQ55.8
    //
    //        largeY = largeYIndexRange(1);
    //        for smallY = 1:(size(curPyramidLevel,1)-1)
    //            for iAlphaY = 1:length(alphas)
    //                alphaY = alphas(iAlphaY);
    //                alphaYinverse = maxAlpha - alphas(iAlphaY);
    //
    //                largeX = largeXIndexRange(1);
    //
    //                for smallX = 1:(size(curPyramidLevel,2)-1)
    //                    if computeDogAtFullSize
    //                        curPyramidLevel_00 = int64(curPyramidLevel(smallY, smallX)); % SQ61.0
    //                        curPyramidLevel_01 = int64(curPyramidLevel(smallY, smallX+1)); % SQ61.0
    //                        curPyramidLevel_10 = int64(curPyramidLevel(smallY+1, smallX)); % SQ61.0
    //                        curPyramidLevel_11 = int64(curPyramidLevel(smallY+1, smallX+1)); % SQ61.0
    //
    //                        curPyramidLevelBlurred_00 = int64(curPyramidLevelBlurred(smallY, smallX)); % SQ61.0
    //                        curPyramidLevelBlurred_01 = int64(curPyramidLevelBlurred(smallY, smallX+1)); % SQ61.0
    //                        curPyramidLevelBlurred_10 = int64(curPyramidLevelBlurred(smallY+1, smallX)); % SQ61.0
    //                        curPyramidLevelBlurred_11 = int64(curPyramidLevelBlurred(smallY+1, smallX+1)); % SQ61.0
    //                    else
    //                        curPyramidLevel_00 = int64(curPyramidLevel(smallY, smallX)); % SQ61.0
    //                        curPyramidLevel_01 = int64(curPyramidLevel(smallY, smallX+1)); % SQ61.0
    //                        curPyramidLevel_10 = int64(curPyramidLevel(smallY+1, smallX)); % SQ61.0
    //                        curPyramidLevel_11 = int64(curPyramidLevel(smallY+1, smallX+1)); % SQ61.0
    //
    //                        DoG_00 = abs(int64(curPyramidLevel(smallY,   smallX))   - int64(curPyramidLevelBlurred(smallY,   smallX))); % SQ61.0
    //                        DoG_01 = abs(int64(curPyramidLevel(smallY,   smallX+1)) - int64(curPyramidLevelBlurred(smallY,   smallX+1))); % SQ61.0
    //                        DoG_10 = abs(int64(curPyramidLevel(smallY+1, smallX))   - int64(curPyramidLevelBlurred(smallY+1, smallX))); % SQ61.0
    //                        DoG_11 = abs(int64(curPyramidLevel(smallY+1, smallX+1)) - int64(curPyramidLevelBlurred(smallY+1, smallX+1))); % SQ61.0
    //                    end
    //
    //                    for iAlphaX = 1:length(alphas)
    //                        alphaX = alphas(iAlphaX);
    //                        alphaXinverse = maxAlpha - alphas(iAlphaX);
    //
    //                        if computeDogAtFullSize
    //                            curPyramidLevelInterpolated = interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
    //                            curPyramidLevelBlurredInterpolated = interpolate2d(curPyramidLevelBlurred_00, curPyramidLevelBlurred_01, curPyramidLevelBlurred_10, curPyramidLevelBlurred_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
    //                            DoG = uint32( abs(curPyramidLevelInterpolated - curPyramidLevelBlurredInterpolated) / (maxAlpha^2/(2^16)) ); % (SQ?.? - SQ?.?) -> UQ16.16
    //                        else
    //                            DoG = uint32( interpolate2d(DoG_00, DoG_01, DoG_10, DoG_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) ); % SQ?.? -> UQ16.16
    //                        end
    //
    //                        largeDoG(largeY,largeX) = DoG;
    //
    //                        if DoG > dogMax(largeY,largeX)
    //                            dogMax(largeY,largeX) = DoG;
    //                            curPyramidLevelInterpolated = uint32( interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) );  % SQ?.? -> UQ16.16
    //                            scaleImage(largeY,largeX) = curPyramidLevelInterpolated;
    //                        end
    //
    //                        largeX = largeX + 1;
    //                    end % for iAlphaX = 1:length(alphas)
    //                end % for smallX = 1:(size(curPyramidLevel,2)-1)
    //
    //                largeY = largeY + 1;
    //            end % for iAlphaY = 1:length(alphas)
    //        end % for smallY = 1:(size(curPyramidLevel,1)-1)
    //    end % if k == 2 ... else
    //
    //    if DEBUG_DISPLAY
    //%         figureHandle = figure(350+pyramidLevel); imshow(double(largeDoG)/(2^16)/255*5);
    //%         figureHandle = figure(300+pyramidLevel); subplot(2,2,1); imshow(curPyramidLevel); subplot(2,2,2); imshow(curPyramidLevelBlurred); subplot(2,2,3); imshow(double(largeDoG)/(2^16)/255*5); subplot(2,2,4); imshow(double(scaleImage)/(2^16));
    //        figureHandle = figure(100+pyramidLevel); subplot(2,4,2); imshow(curPyramidLevel); subplot(2,4,4); imshow(curPyramidLevelBlurred); subplot(2,4,6); imshow(double(largeDoG)/(2^16)/255*5); subplot(2,4,8); imshow(double(scaleImage)/(255*2^16));
    //
    //        % figureHandle = figure(300+pyramidLevel); imshow(curPyramidLevelBlurred);
    //%         set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1])
    //        jFig = get(handle(figureHandle), 'JavaFrame');
    //        jFig.setMaximized(true);
    //        pause(.1);
    //    end
    //
    //    %imgPyramid{pyramidLevel} = uint8(imresize_bilinear(curPyramidLevelBlurred, size(curPyramidLevelBlurred)/2)); % TODO: implement fixed point version
    //    imgPyramid{pyramidLevel} = uint8(downsampleByFactor(curPyramidLevelBlurred, 2));
    //
    //%     keyboard;
    //end
    //
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

    return Anki::RESULT_OK;
  } // Result ComputeCharacteristicScaleImage(const Matrix<u8> &img, u32 numLevels, Matrix<u32> &scaleImage, MemoryStack scratch)
} // namespace Anki