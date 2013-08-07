function [scaleImage, whichScale, imgPyr] = computeCharacteristicScaleImage( ...
    img, numLevels, kernel)
% Returns an image with each pixel at it's characteristic scale.
%
% [scaleImage, whichScale, imgPyr] = computeCharacteristicScaleImage( ...
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

imgPyr = cell(1, numLevels);
if nargin < 3 || isempty(kernel)
    kernel = [1 4 6 4 1];
end
kernel = kernel / sum(kernel); % make sure kernel is normalized

assert(size(img,3)==1, 'Image should be scalar-valued.');
[nrows,ncols] = size(img);
scaleImage = img;
DoG_max = zeros(nrows,ncols);

if nargout > 1 || nargout == 0
    whichScale = ones(nrows,ncols);
end

imgPyr{1} = img;
for k = 1:numLevels
   p1 = separable_filter(imgPyr{k}, kernel);
   p2 = separable_filter(separable_filter(p1, kernel), kernel);
   DoG = abs(p1 - p2); % Difference of Gaussians
  
   DoG = imresize(DoG, [nrows ncols], 'bilinear');
   larger = DoG > DoG_max;
   if any(larger(:))
       DoG_max(larger) = DoG(larger);
       p2_upsample = imresize(p2, [nrows ncols], 'bilinear');
       scaleImage(larger) = p2_upsample(larger);
       if nargout > 1 || nargout == 0
           whichScale(larger) = k;
       end
   end
   
   imgPyr{k+1} = p2(1:2:end,1:2:end);   
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