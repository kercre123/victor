function scaleImage = computeCharacteristicScaleImage_loopsAndFixedPoint( ...
    img, numLevels, computeDogAtFullSize)
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

if nargin < 3
    computeDogAtFullSize = false;
%     computeDogAtFullSize = true;
end

assert(size(img,3)==1, 'Image should be scalar-valued.');

img = im2uint8(img);

[nrows,ncols] = size(img);
scaleImage = uint16(img)*(2^8); % Q8.8
DoG_max = zeros(nrows,ncols,'uint8');

imgPyramid = cell(1, numLevels+1);
scaleFactors = int32(2.^[0:(numLevels)]); %#ok<NBRAK>

imgPyramid{1} = binomialFilter(img);

for k = 2:numLevels+1
    curPyramidLevel = imgPyramid{k-1};
    curPyramidLevelBlurred = binomialFilter(curPyramidLevel);

    if DEBUG_DISPLAY
        if computeDogAtFullSize
            DoGorig = abs( imresize(double(curPyramidLevelBlurred),[nrows,ncols],'bilinear') - imresize(double(curPyramidLevel),[nrows,ncols],'bilinear') );
        else
            DoGorig = abs(double(curPyramidLevelBlurred) - double(curPyramidLevel));
            DoGorig = imresize(DoGorig, [nrows ncols], 'bilinear');
        end
    end
    
    largeDoG = zeros([nrows,ncols],'uint16'); % Q8.8
    
    if k == 2
        for y = 1:nrows
            for x = 1:ncols
                DoG = uint16(abs(int32(curPyramidLevelBlurred(y,x)) - int32(curPyramidLevel(y,x)))) * (2^8);
                largeDoG(y,x) = DoG;
                if DoG > DoG_max(y,x)
                    DoG_max(y,x) = DoG;
                    scaleImage(y,x) = uint16(curPyramidLevel(y,x)) * (2^8); % Q8.0 -> Q8.8
                end
            end
        end
    else % if k == 2
        largeYIndexRange = 1 + [scaleFactors(k-1)/2, nrows-scaleFactors(k-1)/2-1];
        largeXIndexRange = 1 + [scaleFactors(k-1)/2, ncols-scaleFactors(k-1)/2-1];

        alphas = int32((2^8)*(.5 + double(0:(scaleFactors(k-1)-1)))); % SQ23.8
        maxAlphaInteger = int32(scaleFactors(k-1)); % SQ31.0
        maxAlpha = int32((2^8)*maxAlphaInteger); % SQ23.8

%         disp(sprintf('(scaleFactors(k-1)^2)) = %d', (scaleFactors(k-1)^2)));
        
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
                            DoG = uint16( abs(curPyramidLevelInterpolated - curPyramidLevelBlurredInterpolated)/(maxAlpha^2/(2^8)) ); % (SQ15.16 - SQ15.16) -> Q8.8
                        else
                            DoG = uint16( interpolate2d(DoG_00, DoG_01, DoG_10, DoG_11, alphaY, alphaYinverse, alphaX, alphaXinverse)/(maxAlpha^2/(2^8)) );
                        end
                        
                        largeDoG(largeY,largeX) = DoG;

%                         if largeY == 219 && largeX == 334
%                             disp(sprintf('DoGorig(%d,%d) = %f', largeY, largeX, DoGorig(largeY,largeX)));
%                             disp(sprintf('DoG(%d,%d) = %f', largeY, largeX, double(DoG)/(2^8)));
% %                             keyboard
%                         end
                        
                        if DoG > DoG_max(largeY,largeX)
                            DoG_max(largeY,largeX) = DoG;
                            curPyramidLevelInterpolated = uint16( interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (maxAlpha^2/(2^8)) );  % SQ15.16 -> Q8.8
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
        % figureHandle = figure(350+k); imshow(largeDoG*5);
%         figureHandle = figure(300+k); imshow(double(largeDoG(150:190,260:300))/255*5);
        figureHandle = figure(300+k); subplot(1,3,1); imshow(curPyramidLevel); subplot(1,3,2); imshow(curPyramidLevelBlurred); subplot(1,3,3); imshow(double(largeDoG)/(2^8)/255*5);
        % figureHandle = figure(300+k); imshow(curPyramidLevelBlurred);
        set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]) 
    end

    imgPyramid{k} = uint8(imresize_bilinear(curPyramidLevelBlurred, size(curPyramidLevelBlurred)/2)); % TODO: implement fixed point version
end

end % FUNCTION computeCharacteristicScaleImage()

function interpolatedPixel = interpolate2d(pixel00, pixel01, pixel10, pixel11, alphaY, alphaYinverse, alphaX, alphaXinverse)
    interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
    interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;

    interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;
end

function imgFiltered = binomialFilter(img)
    kernelU32 = uint32([1, 4, 6, 4, 1]);
    kernelSum = uint32(sum(kernelU32));
    assert(kernelSum == 16);
    
    img = uint32(img);

    imgFilteredTmp = zeros(size(img), 'uint32');
    for y = 3:(size(img,1)-2)
        for x = 3:(size(img,2)-2)
            imgFilteredTmp(y,x) = sum(img(y,(x-2):(x+2)) .* kernelU32);
        end
    end
    
    imgFiltered = zeros(size(img), 'uint8');
    for y = 3:(size(img,1)-2)
        for x = 3:(size(img,2)-2)
            imgFiltered(y,x) = uint8( sum(imgFilteredTmp((y-2):(y+2),x) .* kernelU32') / (kernelSum^2) );
        end
    end
    
%     keyboard
end


