% function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_multiple(image, numLevels, thresholdFraction)

%  binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_multiple(im, 5, 0.75);

function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_multiple(image, numLevels, thresholdFraction)

if ~exist('thresholdFraction' ,'var')
    thresholdFraction = 0.75;
end

DEBUG_DISPLAY = false;
% DEBUG_DISPLAY = true;

assert(size(image,3)==1, 'Image should be scalar-valued.');
[nrows,ncols] = size(image);
scaleImage = image;
dog_max = zeros(nrows,ncols);

if nargout > 1 || nargout == 0
    whichScale = ones(nrows,ncols);
end

maxFilterHalfWidth = 2 ^ (numLevels+1);

imageWithBorders = zeros([nrows+2*maxFilterHalfWidth+1, ncols+2*maxFilterHalfWidth+1]);

validYIndexes = (maxFilterHalfWidth+1):(maxFilterHalfWidth+nrows);
validXIndexes = (maxFilterHalfWidth+1):(maxFilterHalfWidth+ncols);

% Replicate the last pixel on the borders
imageWithBorders(1:maxFilterHalfWidth, validXIndexes) = repmat(image(1,:), [maxFilterHalfWidth,1]);
imageWithBorders((maxFilterHalfWidth+nrows+1):end, validXIndexes) = repmat(image(end,:), [maxFilterHalfWidth+1,1]);
imageWithBorders(validYIndexes, 1:maxFilterHalfWidth) = repmat(image(:,1), [1,maxFilterHalfWidth]);
imageWithBorders(validYIndexes, (maxFilterHalfWidth+ncols+1):end) = repmat(image(:,end), [1,maxFilterHalfWidth+1]);

% Replicate the corners from the corner pixel
imageWithBorders(1:maxFilterHalfWidth, 1:maxFilterHalfWidth) = image(1,1);
imageWithBorders((maxFilterHalfWidth+nrows+1):end, 1:maxFilterHalfWidth) = image(end,1);
imageWithBorders(1:maxFilterHalfWidth, (maxFilterHalfWidth+ncols+1):end) = image(1,1);
imageWithBorders((maxFilterHalfWidth+nrows+1):end, (maxFilterHalfWidth+nrows+1):end) = image(end,end);

imageWithBorders(validYIndexes, validXIndexes) = image;
integralImageWithBorders = integralimage(imageWithBorders);

for pyramidLevel = 1:numLevels
    halfWidths = 2 .^ ((pyramidLevel-1):(pyramidLevel+1));
%     filterAreas = (2.*halfWidths+1) .^ 2;    
%     filters = [-halfWidths', -halfWidths', halfWidths', halfWidths', 1./filterAreas'];
    filters = [-halfWidths(1), -halfWidths(2), halfWidths(1), halfWidths(2), 1/((2*halfWidths(1)+1)*(2*halfWidths(2)+1));
               -halfWidths(2), -halfWidths(1), halfWidths(2), halfWidths(1), 1/((2*halfWidths(1)+1)*(2*halfWidths(2)+1));
               -halfWidths(2), -halfWidths(3), halfWidths(2), halfWidths(3), 1/((2*halfWidths(2)+1)*(2*halfWidths(3)+1));
               -halfWidths(3), -halfWidths(2), halfWidths(3), halfWidths(2), 1/((2*halfWidths(2)+1)*(2*halfWidths(3)+1))];
    
    numFilters = size(filters,1);
           
    filtered = cell(numFilters,1);
    for iFilt = 1:numFilters
        filtered{iFilt} = integralfilter(integralImageWithBorders, filters(iFilt,:));
        filtered{iFilt} = filtered{iFilt}(validYIndexes,validXIndexes);
    end
    
    dog1 = abs(filtered{1} - filtered{3});
    dog2 = abs(filtered{2} - filtered{4});
    
    dog = min(dog1, dog2);
    
    larger = dog > dog_max;
    if any(larger(:))
        dog_max(larger) = dog(larger);
        scaleImage(larger) = min(filtered{1}(larger), filtered{2}(larger));
    end
    
%     minFiltered1 = min(filtered{1}, filtered{2});
%     minFiltered2 = min(filtered{3}, filtered{4});
%     dog = abs(minFiltered1 - minFiltered2);
% 
%     larger = dog > dog_max;
%     if any(larger(:))
%         dog_max(larger) = dog(larger);
%         scaleImage(larger) = minFiltered2(larger);
%     end

    if DEBUG_DISPLAY
%         figureHandle = figure(100+pyramidLevel); subplot(2,4,1); imshow(filteredSmall); subplot(2,4,3); imshow(filteredLarge); subplot(2,4,5); imshow(dog*5); subplot(2,4,7); imshow(scaleImage);
%         set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1])
%         figureHandle = figure(200+pyramidLevel); imshow(uint8(filteredLarge(validYIndexes,validXIndexes)));
%         set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1])
    end
end

if DEBUG_DISPLAY
    figureHandle = figure(2); imshow(uint8(dog_max));
    set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]);
    figureHandle = figure(12); imshow(uint8(scaleImage));
    set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]);
end

binaryImage = image < thresholdFraction*scaleImage;

end % FUNCTION computeCharacteristicScaleImage()