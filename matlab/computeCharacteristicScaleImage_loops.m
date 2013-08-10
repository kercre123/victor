function scaleImage = computeCharacteristicScaleImage_loops( ...
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

if nargin < 3
    computeDogAtFullSize = false;
end

assert(size(img,3)==1, 'Image should be scalar-valued.');

[nrows,ncols] = size(img);
scaleImage = img;
DoG_max = zeros(nrows,ncols);

imgPyramid = cell(1, numLevels+1);
scaleFactors = 2.^[0:(numLevels)]; %#ok<NBRAK>

imgPyramid{1} = binomialFilter(img);

for k = 2:numLevels+1
    curPyramidLevel = imgPyramid{k-1};
    curPyramidLevelBlurred = binomialFilter(curPyramidLevel);

    largeDoG = zeros([nrows,ncols]);
    
    if k == 2
        for y = 1:nrows
            for x = 1:ncols
                DoG = abs(curPyramidLevelBlurred(y,x) - curPyramidLevel(y,x));
                largeDoG(y,x) = DoG;
                if DoG > DoG_max(y,x)
                    DoG_max(y,x) = DoG;
                    scaleImage(y,x) = curPyramidLevel(y,x);
                end
            end
        end
    else % if k == 2
        largeYIndexRange = 1 + [scaleFactors(k-1)/2, nrows-scaleFactors(k-1)/2-1];
        largeXIndexRange = 1 + [scaleFactors(k-1)/2, ncols-scaleFactors(k-1)/2-1];

        alphas = .5 + (0:(scaleFactors(k-1)-1));

        largeY = largeYIndexRange(1);
        for smallY = 1:(size(curPyramidLevel,1)-1)
            for iAlphaY = 1:length(alphas)
                alphaY = alphas(iAlphaY);
                alphaYinverse = scaleFactors(k-1) - alphas(iAlphaY);

                largeX = largeXIndexRange(1);

                for smallX = 1:(size(curPyramidLevel,2)-1)
                    if computeDogAtFullSize
                        curPyramidLevel_00 = curPyramidLevel(smallY, smallX);
                        curPyramidLevel_01 = curPyramidLevel(smallY, smallX+1);
                        curPyramidLevel_10 = curPyramidLevel(smallY+1, smallX);
                        curPyramidLevel_11 = curPyramidLevel(smallY+1, smallX+1);

                        curPyramidLevelBlurred_00 = curPyramidLevelBlurred(smallY, smallX);
                        curPyramidLevelBlurred_01 = curPyramidLevelBlurred(smallY, smallX+1);
                        curPyramidLevelBlurred_10 = curPyramidLevelBlurred(smallY+1, smallX);
                        curPyramidLevelBlurred_11 = curPyramidLevelBlurred(smallY+1, smallX+1);
                    else
                        curPyramidLevel_00 = curPyramidLevel(smallY, smallX);
                        curPyramidLevel_01 = curPyramidLevel(smallY, smallX+1);
                        curPyramidLevel_10 = curPyramidLevel(smallY+1, smallX);
                        curPyramidLevel_11 = curPyramidLevel(smallY+1, smallX+1);
                        
                        DoG_00 = abs(curPyramidLevel(smallY,   smallX)   - curPyramidLevelBlurred(smallY,   smallX));
                        DoG_01 = abs(curPyramidLevel(smallY,   smallX+1) - curPyramidLevelBlurred(smallY,   smallX+1));
                        DoG_10 = abs(curPyramidLevel(smallY+1, smallX)   - curPyramidLevelBlurred(smallY+1, smallX));
                        DoG_11 = abs(curPyramidLevel(smallY+1, smallX+1) - curPyramidLevelBlurred(smallY+1, smallX+1));
                    end

                    for iAlphaX = 1:length(alphas)
                        alphaX = alphas(iAlphaX);
                        alphaXinverse = scaleFactors(k-1) - alphas(iAlphaX);

                        if computeDogAtFullSize
                            curPyramidLevelInterpolated = interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (scaleFactors(k-1)^2);
                            curPyramidLevelBlurredInterpolated = interpolate2d(curPyramidLevelBlurred_00, curPyramidLevelBlurred_01, curPyramidLevelBlurred_10, curPyramidLevelBlurred_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (scaleFactors(k-1)^2);
                            DoG = abs(curPyramidLevelInterpolated - curPyramidLevelBlurredInterpolated);
                        else
                            DoG = interpolate2d(DoG_00, DoG_01, DoG_10, DoG_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (scaleFactors(k-1)^2);
                        end
                        
                        largeDoG(largeY,largeX) = DoG;

                        if DoG > DoG_max(largeY,largeX)
                            DoG_max(largeY,largeX) = DoG;
                            curPyramidLevelInterpolated = interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (scaleFactors(k-1)^2);
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
        % figureHandle = figure(250+k); imshow(largeDoG*5);
        figureHandle = figure(200+k); imshow(largeDoG(150:190,260:300)*5);
        % figureHandle = figure(200+k); subplot(1,3,1); imshow(curPyramidLevel); subplot(1,3,2); imshow(curPyramidLevelBlurred); subplot(1,3,3); imshow(largeDoG*5);
        % figureHandle = figure(200+k); imshow(curPyramidLevelBlurred);
        set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]) 
    end

    imgPyramid{k} = imresize_bilinear(curPyramidLevelBlurred, size(curPyramidLevelBlurred)/2);
end

end % FUNCTION computeCharacteristicScaleImage()

function interpolatedPixel = interpolate2d(pixel00, pixel01, pixel10, pixel11, alphaY, alphaYinverse, alphaX, alphaXinverse)
    interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
    interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;

    interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;
end

function imgFiltered = binomialFilter(img)
    kernel = [1, 4, 6, 4, 1];
    kernelSum = sum(kernel);

    imgFiltered = imfilter(imfilter(img, kernel, 'same', 'symmetric'), kernel', 'same', 'symmetric') / (kernelSum^2);
end


