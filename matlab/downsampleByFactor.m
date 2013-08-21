% function imgResized = downsampleByFactor(img, downsampleFactor)
%
% Downsamples img using bilinear interpolation
%
% img must be uint8
% downsampleFactor must be a power of 2

function imgDownsampled = downsampleByFactor(img, downsampleFactor)

assert(ndims(img) == 2);
assert(strcmp(class(img), 'uint8')); %#ok<*STISA>

% downsampleFactor must be a power of 2
[f,~] = log2(downsampleFactor);
assert(f == 0.5);

imgDownsampled = zeros(floor(size(img)/downsampleFactor), class(img));

maxY = downsampleFactor * size(imgDownsampled,1);
maxX = downsampleFactor * size(imgDownsampled,2);

img = uint32(img);

smallY = 1;
for y = 1:downsampleFactor:maxY

    smallX = 1;
    for x = 1:downsampleFactor:maxX
%         imgDownsampled(smallY, smallX) = uint8((img(y,x) + img(y+1,x) + img(y,x+1) + img(y+1,x+1)) / 4);
        imgDownsampled(smallY, smallX) = uint8( floor(double(sum(sum(img(y:(y+downsampleFactor-1),x:(x+downsampleFactor-1))))) / double(downsampleFactor^2) ));

        smallX = smallX + 1;
    end

    smallY = smallY + 1;
end