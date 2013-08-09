function scaleImage = computeCharacteristicScaleImage_loops( ...
    img, numLevels)
% Returns an image with each pixel at it's characteristic scale.
%
% [scaleImage, whichScale, imgPyramid] = computeCharacteristicScaleImage( ...
%    img, numLevels, <kernel>)
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

kernelSum = sum([1, 4, 6, 4, 1]);

assert(size(img,3)==1, 'Image should be scalar-valued.');

[nrows,ncols] = size(img);
scaleImage = img;
DoG_max = zeros(nrows,ncols);

if nargout > 1 || nargout == 0
    whichScale = ones(nrows,ncols);
end

imgPyramid = cell(1, numLevels+1);
scaleFactors = 2.^[0:(numLevels)]; %#ok<NBRAK>

imgPyramid{1} = binomialFilter(img);

for k = 2:numLevels+1
    curPyramidLevel = imgPyramid{k-1};
    curPyramidLevelBlurred = binomialFilter(curPyramidLevel);
    
    bigDoG = abs(curPyramidLevelBlurred - curPyramidLevel);
    bigDoG = imresize(bigDoG, [nrows ncols], 'bilinear');
    
    bigDoGG = zeros(size(bigDoG));
    curPyramidLevelBig = imresize(curPyramidLevel, [nrows ncols], 'bilinear');
    curPyramidLevelBlurredBig = imresize(curPyramidLevelBlurred, [nrows ncols], 'bilinear');
      
    if k == 2
        for y = 1:nrows
            for x = 1:ncols
                DoG = abs(curPyramidLevelBlurred(y,x) - curPyramidLevel(y,x));
                bigDoGG(y,x) = DoG;
                if DoG > DoG_max(y,x)
                    DoG_max(y,x) = DoG;
                    scaleImage(y,x) = imgPyramid{1}(y,x);
                end
            end
        end
%         imshows(bigDoG*20, bigDoGG*20);
%         keyboard
    else
        interpolationScaleFactor = scaleFactors(k-1)^2;
        
        largeYs = (1:scaleFactors(k-1):(nrows-scaleFactors(k-1)));
%         smallYs = 1:((nrows/scaleFactors(k-1))-1);       
%         assert(length(largeYs) == length(smallYs));
        
        largeXs = (1:scaleFactors(k-1):(ncols-scaleFactors(k-1)));
%         smallXs = 1:((ncols/scaleFactors(k-1))-1);
%         assert(length(largeXs) == length(smallXs));
                
        for smallY = 1:length(largeYs)
            for alphaY = 0:scaleFactors(k-1)
                largeY = largeYs(smallY) + alphaY;
                alphaYinverse = scaleFactors(k-1) - alphaY;
                
                for smallX = 1:length(largeXs)
                    % Corners
                    % 00 01
                    % 10 11
                    curPyramidLevel_00 = curPyramidLevel(smallY, smallX);
                    curPyramidLevel_01 = curPyramidLevel(smallY, smallX+1);
                    curPyramidLevel_10 = curPyramidLevel(smallY+1, smallX);
                    curPyramidLevel_11 = curPyramidLevel(smallY+1, smallX+1);
                    
                    curPyramidLevelBlurred_00 = curPyramidLevelBlurred(smallY, smallX);
                    curPyramidLevelBlurred_01 = curPyramidLevelBlurred(smallY, smallX+1);
                    curPyramidLevelBlurred_10 = curPyramidLevelBlurred(smallY+1, smallX);
                    curPyramidLevelBlurred_11 = curPyramidLevelBlurred(smallY+1, smallX+1);
                    
                    for alphaX = 0:scaleFactors(k-1)
                        largeX = largeXs(smallX) + alphaX;
                        alphaXinverse = scaleFactors(k-1) - alphaX;
                        
                        curPyramidLevelInterpolated = interpolate2d(curPyramidLevel_00, curPyramidLevel_01, curPyramidLevel_10, curPyramidLevel_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (alphaX+alphaXinverse+alphaY+alphaYinverse);
                        curPyramidLevelBlurredInterpolated = interpolate2d(curPyramidLevelBlurred_00, curPyramidLevelBlurred_01, curPyramidLevelBlurred_10, curPyramidLevelBlurred_11, alphaY, alphaYinverse, alphaX, alphaXinverse) / (alphaX+alphaXinverse+alphaY+alphaYinverse);
                       
                        DoG = abs(curPyramidLevelInterpolated - curPyramidLevelBlurredInterpolated);
                        bigDoGG(largeY,largeX) = DoG;
                        
                        keyboard
%                         if largeX == 40 && largeY == 18
%                             keyboard
%                         end
                        
                        if DoG > DoG_max(largeY,largeX)
                            DoG_max(largeY,largeX) = DoG;
                            scaleImage(largeY,largeX) = imgPyramid{1}(largeY,largeX);
                        end
                    end
                end
            end
        end
    end
    
    keyboard
    
    imgPyramid{k} = curPyramidLevel(1:2:end,1:2:end);
end

if nargout == 0
    subplot 131
    imshow(img)
    subplot 132
    imshow(scaleImage)
    subplot 133
    imagesc(whichScale), axis image
    
    clear scaleImage
end

end % FUNCTION computeCharacteristicScaleImage()

function interpolated = interpolate2d(img00, img01, img10, img11, alphaY, alphaYinverse, alphaX, alphaXinverse)
    interpolatedTop = alphaXinverse*img00 + alphaX*img01;
    interpolatedBottom = alphaXinverse*img10 + alphaX*img11;

    interpolated = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;                                                                                       
end

function imgFiltered = binomialFilter(img)
    kernel = [1, 4, 6, 4, 1];
    kernelSum = sum(kernel);
%     imgFiltered = imfilter(imfilter(img, kernel, 'same', 'symmetric')/kernelSum, kernel', 'same', 'symmetric')/kernelSum;
    imgFiltered = imfilter(imfilter(img, kernel, 'same', 'symmetric'), kernel', 'same', 'symmetric')/(kernelSum*kernelSum);
end


