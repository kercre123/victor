
% function integralImage = computeScrollingIntegralImage(image, integralImage, firstImageY, numRowsToScroll)
% Computes a scrolling integral image. This is for systems with little
% memory.

% Start the integralImage, with an extra border of ten pixels
% initialII = computeScrollingIntegralImage(im, zeros(64,size(im,2)+20,'int32'), 1, 64, 10);

% Repeatedly scroll the image by 32 rows
% scrolledII = computeScrollingIntegralImage(im, initialII, 1+64, 32, 10);
% scrolledII = computeScrollingIntegralImage(im, scrolledII, 1+64+32, 32, 10);
% scrolledII = computeScrollingIntegralImage(im, scrolledII, 1+64+32*2, 32, 10);

% Examples:

% Test with border of 2
% im = [1,2,3;9,9,9;0,0,1];
% imBig = [1,1,1,2,3,3,3;1,1,1,2,3,3,3;1,1,1,2,3,3,3;9,9,9,9,9,9,9;0,0,0,0,1,1,1;0,0,0,0,1,1,1;0,0,0,0,1,1,1]
% groundTruthII = integralimage(double(imBig))
% initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2)+4,'int32'), 1, 3, 2)
% scrolledII = computeScrollingIntegralImage(im, initialII, 1+3-2, 2, 2)
% scrolledII2 = computeScrollingIntegralImage(im, scrolledII, 1+3-2+2, 2, 2)

% Test with border of 1
% im = [1,2,3;9,9,9;0,0,1];
% imBig2 = [1,1,2,3,3;1,1,2,3,3;9,9,9,9,9;0,0,0,1,1;0,0,0,1,1];
% groundTruthII = integralimage(double(imBig2))
% initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2)+2,'int32'), 1, 3, 1)
% scrolledII = computeScrollingIntegralImage(im, initialII, 1+3-1, 1, 1)
% scrolledII2 = computeScrollingIntegralImage(im, scrolledII, 1+3-1+1, 1, 1)

% Test with border of 0
% im = [1,2,3;9,9,9;0,0,1;5,5,5];
% groundTruthII = integralimage(double(im))
% initialII = computeScrollingIntegralImage(im, zeros(3,size(im,2),'int32'), 1, 3, 0)
% scrolledII = computeScrollingIntegralImage(im, initialII, 1+3, 1, 0)

function integralImage = computeScrollingIntegralImage(image, integralImage, firstImageY, numRowsToScroll, numBorderPixels)

image = int32(image);
integralImage = int32(integralImage);

assert(size(integralImage,1) > 2);

% If this is the first call, fill in the entire buffer
if numRowsToScroll == size(integralImage, 1)
    assert(size(integralImage,1) >= numBorderPixels);
    assert(numRowsToScroll >= numBorderPixels);

    paddedImageRow = padImageRow(image, firstImageY, numBorderPixels);

    assert(size(paddedImageRow,2) == size(integralImage,2));

    % Create the first line (not zeros, so everything is automatically
    % byte-aligned)
    integralImage(1,1) = image(firstImageY,1);
    for x = 2:size(paddedImageRow, 2)
        integralImage(1,x) = paddedImageRow(1,x) + integralImage(1,x-1);
    end

    % Fill in all the top padded rows
    for curIntegralImageY = 2:(1+numBorderPixels)
        integralImage(curIntegralImageY,1) = paddedImageRow(1,1) + integralImage(curIntegralImageY-1,1);

        for x = 2:size(paddedImageRow, 2)
            integralImage(curIntegralImageY,x) = paddedImageRow(1,x) + integralImage(curIntegralImageY,x-1) + integralImage(curIntegralImageY-1,x) - integralImage(curIntegralImageY-1,x-1);
        end
    end % for curIntegralImageY = 2:(1+numBorderPixels)

    if isempty(curIntegralImageY)
        curIntegralImageY = 1;
    end

    curImageY = firstImageY + 1;
    curIntegralImageY = curIntegralImageY + 1;
    numRowsToScroll = numRowsToScroll - numBorderPixels - 1;
else
    curImageY = firstImageY;
    curIntegralImageY = size(integralImage,1) - (numRowsToScroll-1);
    integralImage(1:(end-numRowsToScroll), :) = integralImage((1+numRowsToScroll):end, :);
end

% Compute the non-padded integral image rows
if curImageY <= size(image,1)
    for iy = 1:numRowsToScroll
        paddedImageRow = padImageRow(image, curImageY, numBorderPixels);

        integralImage(curIntegralImageY,1) = paddedImageRow(1,1) + integralImage(curIntegralImageY-1,1);
        for x = 2:size(paddedImageRow, 2)
            integralImage(curIntegralImageY,x) = paddedImageRow(1,x) + integralImage(curIntegralImageY,x-1) + integralImage(curIntegralImageY-1,x) - integralImage(curIntegralImageY-1,x-1);
        end

        curImageY = curImageY + 1;
        curIntegralImageY = curIntegralImageY + 1;

        % If we've run out of image, exit this loop, then start the
        % bottom-padding loop
        if curImageY > size(image,1)
            curImageY = curImageY - 1;
            break;
        end
    end % for iy = 1:numRowsToScroll
else
    curImageY = size(image,1);
    iy = 0;
end

% If we're at the bottom of the image, compute the extra bottom padded rows
paddedImageRow = padImageRow(image, curImageY, numBorderPixels);
for iy = (iy+1):numRowsToScroll
    integralImage(curIntegralImageY,1) = paddedImageRow(1,1) + integralImage(curIntegralImageY-1,1);
    for x = 2:size(paddedImageRow, 2)
        integralImage(curIntegralImageY,x) = paddedImageRow(1,x) + integralImage(curIntegralImageY,x-1) + integralImage(curIntegralImageY-1,x) - integralImage(curIntegralImageY-1,x-1);
    end
    curIntegralImageY = curIntegralImageY + 1;
end

end % function computeScrollingIntegralImage()

function paddedImageRow = padImageRow(image, y, numBorderPixels)
    paddedImageRow = zeros([1,size(image,2)+2*numBorderPixels], 'int32');
    paddedImageRow(1:numBorderPixels) = image(y,1);
    paddedImageRow((1+numBorderPixels):(numBorderPixels+size(image,2))) = image(y,:);
    paddedImageRow((1+numBorderPixels+size(image,2)):end) = image(y,end);
end % function padImageRow()




