% function imgResized = imgresize_bilinearFixedPoint(img, downsampleFactor)
%
% Downsamples an image uising bilinear interpolation, using fixed point.

function imgDownsampled = downsample_fixedPoint(img, downsampleFactor)

assert(downsampleFactor == 2); % TODO: implement non-halving downsampling
assert(ndims(img) == 2);
assert(strcmp(class(img), 'uint8'));

imgDownsampled = zeros(floor(size(img)/downsampleFactor), class(img));

maxY = 2 * size(imgDownsampled,1);
maxX = 2 * size(imgDownsampled,2);

img = uint16(img);

smallY = 1;
for y = 1:2:maxY

    smallX = 1;
    for x = 1:2:maxX
        imgDownsampled(smallY, smallX) = uint8((img(y,x) + img(y+1,x) + img(y,x+1) + img(y+1,x+1)) / 4);

        smallX = smallX + 1;
    end

    smallY = smallY + 1;
end