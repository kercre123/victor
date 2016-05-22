% function imageFiltered = binomialFilter_loopsAndFixedPoint(image)
%
% image is 5xN and UQ8.0
% imageFilteredLine is 5xN and UQ8.0
% Handles edges by replicating the border pixel

function imageFilteredLine = binomialFilter_heightFive(image)
    kernelU32 = uint32([1, 4, 6, 4, 1]);
    kernelSum = uint32(sum(kernelU32));
    assert(kernelSum == 16);
    
    assert(size(image,1) == 5);

    image = uint32(image);

    imageFilteredLineTmp = zeros([1,size(image,2)], 'uint32');

    % 1. Horizontally filter
    for y = 1:size(image,1)
        for x = 3:(size(image,2)-2)
            imageFilteredLineTmp(y,x) = sum(image(y,(x-2):(x+2)) .* kernelU32);
        end
        
        imageFilteredLineTmp(y, 1:2) = imageFilteredLineTmp(y,3);
        imageFilteredLineTmp(y, (size(image,2)-2):end) = imageFilteredLineTmp(y, size(image,2)-3);
    end

    % 2. Vertically filter
    imageFilteredLine = zeros([1,size(image,2)], 'uint8');

    for x = 1:size(image,2)
        imageFilteredLine(1,x) = uint8( floor(double(sum(imageFilteredTmp(1:5,x) .* kernelU32')) / double(kernelSum^2)) );
    end
end
