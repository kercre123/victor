function scaleImage = computeCharacteristicScaleImage_loopsAndFixedPoint( ...
    img, numLevels, computeDogAtFullSize, filterWithMex)
% Returns an image with each pixel at its characteristic scale. This
% version has explicit loops.
%
% [scaleImage, whichScale, imgPyramid] = computeCharacteristicScaleImage( ...
%    img, numLevels, computeDogAtFullSize)
%
%  Computes a scale-space pyramid and then finds the characteristic scale
%  of each pixel by finding extrema in the Laplacian response at each pixel
%  across scales.  Note that (a) the Laplacian is approximated by a
%  Difference-of-Gaussian (DoG) computation, and (b) bilinear interpolation
%  is used to resample coarser scales to compare them to finer scales.
%
%  A pixel in the output 'scaleImage' comes from the level of the image
%  pyramid at the corresponding scale, also computed by bilinear
%  interpolation.
%
%  Note that no interpolation or parabola-fitting is performed along the
%  scale dimension to get sub-scale-level characteristic scales.  I.e., the
%  scale is simpler the integer-level scale.
%
%  The smoothing operation used here is repeated filtering with a simple
%  binomial filter: [1 4 6 4 1].  This kernel (somewhat roughly)
%  approximates a Gaussian and should lend itself well to efficient
%  implementation.  Other kernels can be supplied as an optional input.
%
% ------------
% Andrew Stein
%

DEBUG_DISPLAY = false;
% DEBUG_DISPLAY = true;

assert(numLevels <= 8);

assert(size(img,3)==1, 'Image should be scalar-valued.');

img = im2uint8(img); % UQ8.0

[nrows,ncols] = size(img);
scaleImage = uint32(img)*(2^8); % UQ16.16
DoG_max = zeros(nrows,ncols,'uint32'); % UQ16.16

imgPyramid = cell(1, numLevels+1);
scaleFactors = int32(2.^[0:(numLevels)]); %#ok<NBRAK>

% keyboard
if filterWithMex
    imgPyramid{1} = mexBinomialFilter(img);
else
    imgPyramid{1} = binomialFilter_loopsAndFixedPoint(img);
end

for pyramidLevel = 2:numLevels+1
    curPyramidLevel = imgPyramid{pyramidLevel-1}; %UQ8.0
    if filterWithMex
        curPyramidLevelBlurred = mexBinomialFilter(curPyramidLevel); %UQ8.0
    else
        curPyramidLevelBlurred = binomialFilter_loopsAndFixedPoint(curPyramidLevel); %UQ8.0
    end

    if DEBUG_DISPLAY
        if computeDogAtFullSize
            DoGorig = abs( imresize(double(curPyramidLevelBlurred),[nrows,ncols],'bilinear') - imresize(double(curPyramidLevel),[nrows,ncols],'bilinear') );
        else
            DoGorig = abs(double(curPyramidLevelBlurred) - double(curPyramidLevel));
            DoGorig = imresize(DoGorig, [nrows ncols], 'bilinear');
        end
    end
    
    largeDoG = zeros([nrows,ncols],'uint32'); % UQ16.16
    
    if pyramidLevel == 2
        for y = 1:nrows
            for x = 1:ncols
                DoG = uint32(abs(int32(curPyramidLevelBlurred(y,x)) - int32(curPyramidLevel(y,x)))) * (2^16); % abs(UQ8.0 - UQ8.0) -> UQ16.16;
                largeDoG(y,x) = DoG;

                % The first level is always correct
                DoG_max(y,x) = DoG;
                scaleImage(y,x) = uint32(curPyramidLevel(y,x)) * (2^16); % UQ8.0 -> UQ16.16

                % The first level is not always correct
%                 if DoG > DoG_max(y,x)
%                     DoG_max(y,x) = DoG;
%                     scaleImage(y,x) = uint32(curPyramidLevel(y,x)) * (2^16); % UQ8.0 -> UQ16.16
%                 end
            end
        end
%         keyboard
    elseif pyramidLevel <= 5 % if pyramidLevel == 2
        largeYIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, nrows-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
        largeXIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, ncols-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0

        alphas = int32((2^8)*(.5 + double(0:(scaleFactors(pyramidLevel-1)-1)))); % SQ23.8
        maxAlpha = int32((2^8)*scaleFactors(pyramidLevel-1)); % SQ31.0 -> SQ23.8

        largeY = largeYIndexRange(1);
        for smallY = 1:(size(curPyramidLevel,1)-1)
            for iAlphaY = 1:length(alphas)
                alphaY = alphas(iAlphaY);
                alphaYinverse = maxAlpha - alphas(iAlphaY);

                largeX = largeXIndexRange(1);

                for smallX = 1:(size(curPyramidLevel,2)-1)
                    if computeDogAtFullSize
                        curPyramidLevel_00 = int32(curPyramidLevel(smallY, smallX)); % SQ31.0
                        curPyramidLevel_01 = int32(curPyramidLevel(smallY, smallX+1)); % SQ31.0
                        curPyramidLevel_10 = int32(curPyramidLevel(smallY+1, smallX)); % SQ31.0
                        curPyramidLevel_11 = int32(curPyramidLevel(smallY+1, smallX+1)); % SQ31.0

                        curPyramidLevelBlurred_00 = int32(curPyramidLevelBlurred(smallY, smallX)); % SQ31.0
                        curPyramidLevelBlurred_01 = int32(curPyramidLevelBlurred(smallY, smallX+1)); % SQ31.0
                        curPyramidLevelBlurred_10 = int32(curPyramidLevelBlurred(smallY+1, smallX)); % SQ31.0
                        curPyramidLevelBlurred_11 = int32(curPyramidLevelBlurred(smallY+1, smallX+1)); % SQ31.0
                    else
                        curPyramidLevel_00 = int32(curPyramidLevel(smallY, smallX)); % SQ31.0
                        curPyramidLevel_01 = int32(curPyramidLevel(smallY, smallX+1)); % SQ31.0
                        curPyramidLevel_10 = int32(curPyramidLevel(smallY+1, smallX)); % SQ31.0
                        curPyramidLevel_11 = int32(curPyramidLevel(smallY+1, smallX+1)); % SQ31.0
                        
                        DoG_00 = abs(int32(curPyramidLevel(smallY,   smallX))   - int32(curPyramidLevelBlurred(smallY,   smallX))); % SQ31.0
                        DoG_01 = abs(int32(curPyramidLevel(smallY,   smallX+1)) - int32(curPyramidLevelBlurred(smallY,   smallX+1))); % SQ31.0
                        DoG_10 = abs(int32(curPyramidLevel(smallY+1, smallX))   - int32(curPyramidLevelBlurred(smallY+1, smallX))); % SQ31.0
                        DoG_11 = abs(int32(curPyramidLevel(smallY+1, smallX+1)) - int32(curPyramidLevelBlurred(smallY+1, smallX+1))); % SQ31.0
                    end

                    for iAlphaX = 1:length(alphas)
                        alphaX = alphas(iAlphaX);
                        alphaXinverse = maxAlpha - alphas(iAlphaX);

                        if computeDogAtFullSize
                            curPyramidLevelInterpolated = interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
                            curPyramidLevelBlurredInterpolated = interpolate2d(curPyramidLevelBlurred_00, curPyramidLevelBlurred_01, curPyramidLevelBlurred_10, curPyramidLevelBlurred_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
                            DoG = uint32( abs(curPyramidLevelInterpolated - curPyramidLevelBlurredInterpolated) / (maxAlpha^2/(2^16)) ); % (SQ?.? - SQ?.?) -> UQ16.16
                        else
                            DoG = uint32( interpolate2d(DoG_00, DoG_01, DoG_10, DoG_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) ); % SQ?.? -> UQ16.16
                        end
                        
                        largeDoG(largeY,largeX) = DoG;
                        
%                         if largeY == 219 && largeX == 334
%                             disp(sprintf('DoGorig(%d,%d) = %f', largeY, largeX, DoGorig(largeY,largeX)));
%                             disp(sprintf('DoG(%d,%d) = %f', largeY, largeX, double(DoG)/(2^16)));
%                             keyboard
%                         end
                        
                        if DoG > DoG_max(largeY,largeX)
                            DoG_max(largeY,largeX) = DoG;
                            curPyramidLevelInterpolated = uint32( interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) );  % SQ?.? -> UQ16.16
                            scaleImage(largeY,largeX) = curPyramidLevelInterpolated;
                        end
                        
                        largeX = largeX + 1;
                    end % for iAlphaX = 1:length(alphas)
                end % for smallX = 1:(size(curPyramidLevel,2)-1)

                largeY = largeY + 1;
            end % for iAlphaY = 1:length(alphas)
        end % for smallY = 1:(size(curPyramidLevel,1)-1)
    else % if pyramidLevel == 2 ... elseif pyramidLevel <= 5
        largeYIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, nrows-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0
        largeXIndexRange = int32(1 + [scaleFactors(pyramidLevel-1)/2, ncols-scaleFactors(pyramidLevel-1)/2-1]); % SQ31.0

        alphas = int64((2^8)*(.5 + double(0:(scaleFactors(pyramidLevel-1)-1)))); % SQ55.8
        maxAlpha = int64((2^8)*scaleFactors(pyramidLevel-1)); % SQ31.0 -> SQ55.8

        largeY = largeYIndexRange(1);
        for smallY = 1:(size(curPyramidLevel,1)-1)
            for iAlphaY = 1:length(alphas)
                alphaY = alphas(iAlphaY);
                alphaYinverse = maxAlpha - alphas(iAlphaY);

                largeX = largeXIndexRange(1);

                for smallX = 1:(size(curPyramidLevel,2)-1)
                    if computeDogAtFullSize
                        curPyramidLevel_00 = int64(curPyramidLevel(smallY, smallX)); % SQ61.0
                        curPyramidLevel_01 = int64(curPyramidLevel(smallY, smallX+1)); % SQ61.0
                        curPyramidLevel_10 = int64(curPyramidLevel(smallY+1, smallX)); % SQ61.0
                        curPyramidLevel_11 = int64(curPyramidLevel(smallY+1, smallX+1)); % SQ61.0

                        curPyramidLevelBlurred_00 = int64(curPyramidLevelBlurred(smallY, smallX)); % SQ61.0
                        curPyramidLevelBlurred_01 = int64(curPyramidLevelBlurred(smallY, smallX+1)); % SQ61.0
                        curPyramidLevelBlurred_10 = int64(curPyramidLevelBlurred(smallY+1, smallX)); % SQ61.0
                        curPyramidLevelBlurred_11 = int64(curPyramidLevelBlurred(smallY+1, smallX+1)); % SQ61.0
                    else
                        curPyramidLevel_00 = int64(curPyramidLevel(smallY, smallX)); % SQ61.0
                        curPyramidLevel_01 = int64(curPyramidLevel(smallY, smallX+1)); % SQ61.0
                        curPyramidLevel_10 = int64(curPyramidLevel(smallY+1, smallX)); % SQ61.0
                        curPyramidLevel_11 = int64(curPyramidLevel(smallY+1, smallX+1)); % SQ61.0
                        
                        DoG_00 = abs(int64(curPyramidLevel(smallY,   smallX))   - int64(curPyramidLevelBlurred(smallY,   smallX))); % SQ61.0
                        DoG_01 = abs(int64(curPyramidLevel(smallY,   smallX+1)) - int64(curPyramidLevelBlurred(smallY,   smallX+1))); % SQ61.0
                        DoG_10 = abs(int64(curPyramidLevel(smallY+1, smallX))   - int64(curPyramidLevelBlurred(smallY+1, smallX))); % SQ61.0
                        DoG_11 = abs(int64(curPyramidLevel(smallY+1, smallX+1)) - int64(curPyramidLevelBlurred(smallY+1, smallX+1))); % SQ61.0
                    end

                    for iAlphaX = 1:length(alphas)
                        alphaX = alphas(iAlphaX);
                        alphaXinverse = maxAlpha - alphas(iAlphaX);

                        if computeDogAtFullSize
                            curPyramidLevelInterpolated = interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
                            curPyramidLevelBlurredInterpolated = interpolate2d(curPyramidLevelBlurred_00, curPyramidLevelBlurred_01, curPyramidLevelBlurred_10, curPyramidLevelBlurred_11, alphaY, alphaYinverse, alphaX, alphaXinverse); % SQ15.16
                            DoG = uint32( abs(curPyramidLevelInterpolated - curPyramidLevelBlurredInterpolated) / (maxAlpha^2/(2^16)) ); % (SQ?.? - SQ?.?) -> UQ16.16
                        else
                            DoG = uint32( interpolate2d(DoG_00, DoG_01, DoG_10, DoG_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) ); % SQ?.? -> UQ16.16
                        end
                        
                        largeDoG(largeY,largeX) = DoG;
                        
%                         if largeY == 219 && largeX == 334
%                             disp(sprintf('DoGorig(%d,%d) = %f', largeY, largeX, DoGorig(largeY,largeX)));
%                             disp(sprintf('DoG(%d,%d) = %f', largeY, largeX, double(DoG)/(2^16)));
%                             keyboard
%                         end
                        
                        if DoG > DoG_max(largeY,largeX)
                            DoG_max(largeY,largeX) = DoG;
                            curPyramidLevelInterpolated = uint32( interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^16)) );  % SQ?.? -> UQ16.16
                            scaleImage(largeY,largeX) = curPyramidLevelInterpolated;
                        end
                        
                        largeX = largeX + 1;
                    end % for iAlphaX = 1:length(alphas)
                end % for smallX = 1:(size(curPyramidLevel,2)-1)

                largeY = largeY + 1;
            end % for iAlphaY = 1:length(alphas)
        end % for smallY = 1:(size(curPyramidLevel,1)-1)
    end % if k == 2 ... else
    
    if DEBUG_DISPLAY
%         figureHandle = figure(350+pyramidLevel); imshow(double(largeDoG)/(2^16)/255*5);
%         figureHandle = figure(300+pyramidLevel); subplot(2,2,1); imshow(curPyramidLevel); subplot(2,2,2); imshow(curPyramidLevelBlurred); subplot(2,2,3); imshow(double(largeDoG)/(2^16)/255*5); subplot(2,2,4); imshow(double(scaleImage)/(2^16));
        figureHandle = figure(100+pyramidLevel); subplot(2,4,2); imshow(curPyramidLevel); subplot(2,4,4); imshow(curPyramidLevelBlurred); subplot(2,4,6); imshow(double(largeDoG)/(2^16)/255*5); subplot(2,4,8); imshow(double(scaleImage)/(255*2^16));

        % figureHandle = figure(300+pyramidLevel); imshow(curPyramidLevelBlurred);
%         set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]) 
        jFig = get(handle(figureHandle), 'JavaFrame'); 
        jFig.setMaximized(true);
        pause(.1);
    end

    %imgPyramid{pyramidLevel} = uint8(imresize_bilinear(curPyramidLevelBlurred, size(curPyramidLevelBlurred)/2)); % TODO: implement fixed point version
    imgPyramid{pyramidLevel} = uint8(downsampleByFactor(curPyramidLevelBlurred, 2));
    
%     keyboard;
end

end % FUNCTION computeCharacteristicScaleImage()

% pixel* are UQ8.0
% alpha* are UQ?.? depending on the level of the pyramid
function interpolatedPixel = interpolate2d(pixel00, pixel01, pixel10, pixel11, alphaY, alphaYinverse, alphaX, alphaXinverse)
    interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
    interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;

    interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;
end

