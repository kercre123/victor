
% function integralImage = computeScrollingIntegralImage(image, integralImage, firstImageY, numRowsToUpdate)
% Computes a scrolling integral image. This is for systems with little
% memory.

% Start the integralImage
% initialII = computeScrollingIntegralImage(im, zeros(64,640,'int32'), 1, 64);

% Repeatedly scroll the image by 32 rows
% scrolledII = computeScrollingIntegralImage(im, initialII, 1+64, 32);
% scrolledII = computeScrollingIntegralImage(im, scrolledII, 1+64+32, 32);
% scrolledII = computeScrollingIntegralImage(im, scrolledII, 1+64+32*2, 32);

function integralImage = computeScrollingIntegralImage(image, integralImage, firstImageY, numRowsToUpdate)

image = int32(image);
integralImage = int32(integralImage);

if numRowsToUpdate == size(integralImage, 1)
    firstRowToUpdate = 2;
    
    % Create the first line (not zeros, so everything is automatically
    % byte-aligned)
    integralImage(1,1) = image(firstImageY,1);
    for x = 2:size(image, 2)
        integralImage(1,x) = image(firstImageY,x) + integralImage(1,x-1);
    end
else
    firstRowToUpdate = 1 + numRowsToUpdate;
    integralImage(1:(end-numRowsToUpdate), :) = integralImage((1+numRowsToUpdate):end, :);
end

iy = firstImageY;
for y = firstRowToUpdate:size(integralImage,1)
    integralImage(y,1) = image(iy,1) + integralImage(y-1,1);
    for x = 2:size(image, 2)
        integralImage(y,x) = image(iy,x) + integralImage(y,x-1) + integralImage(y-1,x) - integralImage(y-1,x-1);
    end
    
    iy = iy + 1;
end % for y = firstRowToUpdate:size(image,1)
