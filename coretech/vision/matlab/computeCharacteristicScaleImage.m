function [scaleImage, whichScale, imgPyr, DoG_pyr] = computeCharacteristicScaleImage( ...
    img, numLevels, kernel, computeDogAtFullSize)
% Returns an image with each pixel at its characteristic scale.
%
% [scaleImage, whichScale, imgPyr, <DoG_pyr>] = computeCharacteristicScaleImage( ...
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

DEBUG_DISPLAY = false;
% DEBUG_DISPLAY = true;

if nargin < 3 || isempty(kernel)
    kernel = [1 4 6 4 1];
end
kernel = kernel / sum(kernel); % make sure kernel is normalized

if nargin < 4
    computeDogAtFullSize = false;
%     computeDogAtFullSize = true;
end

assert(size(img,3)==1, 'Image should be scalar-valued.');
[nrows,ncols] = size(img);
scaleImage = img;
DoG_max = zeros(nrows,ncols);

if nargout > 1 || nargout == 0
    whichScale = ones(nrows,ncols);
end

imgPyr = cell(1, numLevels+1);
imgPyr{1} = separable_filter(img, kernel);

if nargout > 3
  DoG_pyr = cell(1, numLevels);
end

for k = 2:numLevels+1

    blurred = separable_filter(imgPyr{k-1}, kernel);
    
    if computeDogAtFullSize
        DoG = abs( imresize(blurred,[nrows,ncols],'bilinear') - imresize(imgPyr{k-1},[nrows,ncols],'bilinear') );
    else
        DoG = abs(blurred - imgPyr{k-1});
        DoG = imresize(DoG, [nrows ncols], 'bilinear');
    end
    
    if nargout > 3
      DoG_pyr{k-1} = DoG;
    end
    
    larger = DoG > DoG_max;
    if any(larger(:))
        DoG_max(larger) = DoG(larger);
        imgPyr_upsample = imresize(imgPyr{k-1}, [nrows ncols], 'bilinear');
        scaleImage(larger) = imgPyr_upsample(larger);
        if nargout > 1 || nargout == 0
            whichScale(larger) = k-1;
        end
    end
    
%     disp(sprintf('min:%f max:%f', min(DoG(:)), max(DoG(:))));

    if DEBUG_DISPLAY
%         figureHandle = figure(100+k); imshow(DoG(150:190,260:300)*5);
%         figureHandle = figure(100+k); subplot(2,2,1); imshow(imgPyr{k-1}); subplot(2,2,2); imshow(blurred); subplot(2,2,3); imshow(DoG*5); subplot(2,2,4); imshow(scaleImage);
        figureHandle = figure(100+k); subplot(2,4,1); imshow(imgPyr{k-1}); subplot(2,4,3); imshow(blurred); subplot(2,4,5); imshow(DoG*5); subplot(2,4,7); imshow(scaleImage);
        set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]) 
    end
    
    imgPyr{k} = imresize_bilinear(blurred, floor(size(blurred)/2));
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