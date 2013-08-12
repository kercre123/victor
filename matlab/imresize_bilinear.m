% function imgResized = imresize_bilinear(img, newSize)
%
% Resizes an image uising bilinear interpolation. Unlike Matlab's built-in
% imgresize, 'bilinear' actually is bilinear interpolation, with NaNs for
% uncomputable values.
%
% newSize is [nrows, ncols]

function imgResized = imresize_bilinear(img, newSize)

interpolationMethod = 'linear';
newSize = double(newSize);

% Compute coordinates corresponding to input and transformed coordinates
% for imgResized

[xo,yo] = meshgrid(0.5+(0:(size(img,2)-1)), 0.5+(0:(size(img,1)-1)));
[x,y] = meshgrid(0.5+(0:(newSize(2)-1)), 0.5+(0:(newSize(1)-1)));

yScale = size(img,1) / newSize(1);
xScale = size(img,2) / newSize(2);

xPrime = x * xScale;
yPrime = y * yScale;

% keyboard

if ndims(img) == 3
    imgResized = zeros([newSize(1), newSize(2), size(img,3)]);
    for i = 1:size(img,3)
        imgResizedTmp = interp2(xo, yo, double(img(:,:,i)), xPrime, yPrime, interpolationMethod);
        imgResized(:,:,i) = reshape(imgResizedTmp, newSize);
    end
else
    imgResized = interp2(xo, yo, double(img), xPrime, yPrime, interpolationMethod);
    imgResized = reshape(imgResized, newSize);
end
