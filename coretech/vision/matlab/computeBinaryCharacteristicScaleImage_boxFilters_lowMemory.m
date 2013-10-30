% function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_lowMemory(image, numPyramidLevels, thresholdFraction)

%  binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_lowMemory(im, 5, 0.75);

function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_lowMemory(image, numPyramidLevels, thresholdFraction)

if ~exist('thresholdFraction' ,'var')
    thresholdFraction = 0.75;
end

numIntegralImageRowsToScroll = 300;

DEBUG_DISPLAY = false;
% DEBUG_DISPLAY = true;

assert(size(image,3)==1, 'Image should be scalar-valued.');
binaryImage = zeros(size(image));

maxFilterHalfWidth = 2 ^ (numPyramidLevels+1);

integralImageBorderWidth = maxFilterHalfWidth + 1;
numIntegralImageRows = (2*maxFilterHalfWidth + 2) + numIntegralImageRowsToScroll;

scrollingIntegralImage = computeScrollingIntegralImage(image,...
    zeros(numIntegralImageRows,size(image,2)+2*integralImageBorderWidth,'int32'),...
    1,...
    numIntegralImageRows,...
    integralImageBorderWidth);

filterNormalizationConstants = zeros(numPyramidLevels+1, 1);
for pyramidLevel = 1:(numPyramidLevels+1)
    filterHalfWidth = 2^pyramidLevel;
    filterNormalizationConstants(pyramidLevel) = 1 / ((2*filterHalfWidth+1)^2);
end

if DEBUG_DISPLAY
    blurredIms = cell(numPyramidLevels+1,1);
    dogMaxAll = -ones(size(image,1), size(image,2), 'uint32');
    scaleImageAll = -ones(size(image,1), size(image,2), 'uint32');
    for pyramidLevel = 1:(numPyramidLevels+1)
        blurredIms{pyramidLevel} = zeros(size(image));
    end
end

imageY = 1;
integralImageY = imageY + maxFilterHalfWidth + 1;
integralImageUpdateY = imageY + numIntegralImageRows - integralImageBorderWidth;
while imageY <= size(image,1)
    filteredLines = cell(numPyramidLevels+1, 1);

    % For the given row of the image, compute the blurred version for each
    % level of the pyramid
    for pyramidLevel = 1:(numPyramidLevels+1)
        filterHalfWidth = 2^pyramidLevel;

        filteredLines{pyramidLevel} = zeros(1, size(image,2));

        x = 1:size(image,2);
        offsetXs = x + maxFilterHalfWidth + 1;

        boxSums = scrollingIntegralImage(integralImageY+filterHalfWidth,   offsetXs+filterHalfWidth) -...
                  scrollingIntegralImage(integralImageY+filterHalfWidth,   offsetXs-filterHalfWidth-1) +...
                  scrollingIntegralImage(integralImageY-filterHalfWidth-1, offsetXs-filterHalfWidth-1) -...
                  scrollingIntegralImage(integralImageY-filterHalfWidth-1, offsetXs+filterHalfWidth);

        filteredLines{pyramidLevel} = filterNormalizationConstants(pyramidLevel) * boxSums;

        if DEBUG_DISPLAY
            blurredIms{pyramidLevel}(imageY,:) = filteredLines{pyramidLevel};
        end
    end % for pyramidLevel = 1:numPyramidLevels

    % 1. Compute all the Difference of Gaussians
    % 2. Select the winner
    % 3. Fill in the corresponding row for binaryImage
    dogMaxValues = -ones(1, size(image,2));
    scaleImage = -ones(1, size(image,2));
    for pyramidLevel = 1:numPyramidLevels
        dog = abs(filteredLines{pyramidLevel+1} - filteredLines{pyramidLevel});

        larger = dog > dogMaxValues;
        if any(larger(:))
            dogMaxValues(larger) = dog(larger);
            scaleImage(larger) = filteredLines{pyramidLevel+1}(larger);
        end
    end

    if DEBUG_DISPLAY
        dogMaxAll(imageY, :) = dogMaxValues;
        scaleImageAll(imageY, :) = scaleImage;
    end

    binaryImage(imageY,:) = image(imageY,:) < thresholdFraction*scaleImage;

    imageY = imageY + 1;
    integralImageY = integralImageY + 1;

    % If we've reached the bottom of this integral image, scroll it up
    if (integralImageY+filterHalfWidth) > numIntegralImageRows
        scrollingIntegralImage = computeScrollingIntegralImage(image,...
            scrollingIntegralImage,...
            integralImageUpdateY,...
            numIntegralImageRowsToScroll,...
            integralImageBorderWidth);

        integralImageY = integralImageY - numIntegralImageRowsToScroll;
        integralImageUpdateY = integralImageUpdateY + numIntegralImageRowsToScroll;
    end
end % while imageY <= size(image,1)

if DEBUG_DISPLAY
    figureHandle = figure(1); imshow(uint8(dogMaxAll));
    set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]);
    figureHandle = figure(11); imshow(uint8(scaleImageAll));
    set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]);

%     for pyramidLevel = 1:(numPyramidLevels+1)
%         figureHandle = figure(100+pyramidLevel); imshow(uint8(blurredIms{pyramidLevel}));
%         set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1])
%     end
end

end % FUNCTION computeCharacteristicScaleImage()