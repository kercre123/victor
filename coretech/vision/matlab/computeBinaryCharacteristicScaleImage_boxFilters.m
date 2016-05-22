% function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters(image, levels, thresholdFraction)

% binaryImage = computeBinaryCharacteristicScaleImage_boxFilters(im, 5, 0.75);
% binaryImage = computeBinaryCharacteristicScaleImage_boxFilters(im, {1,2,4,8,16}, 0.75);

function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters(image, levels, thresholdFraction)
    
    if ~exist('thresholdFraction' ,'var')
        thresholdFraction = 0.75;
    end
    
    DEBUG_DISPLAY = false;
    % DEBUG_DISPLAY = true;
    
    assert(size(image,3)==1, 'Image should be scalar-valued.');
    [nrows,ncols] = size(image);
    scaleImage = image;
    dog_max = zeros(nrows,ncols);
    
    if iscell(levels)
        filterHalfWidths = cell2mat(levels);
        maxFilterHalfWidth = levels{end};
    else
        filterHalfWidths = 2 .^ (1:(levels+1));
        maxFilterHalfWidth = max(filterHalfWidths);
    end
    
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
    
    %     for pyramidLevel = levels
    for iHalfWidth = 1:(length(filterHalfWidths)-1)
        %         halfWidthLarge = 2 ^ pyramidLevel;
        halfWidthLarge = filterHalfWidths(iHalfWidth+1);
        filterAreaLarge = (2*halfWidthLarge+1) ^ 2;
        
        %         halfWidthSmall = 2 ^ (pyramidLevel+1);
        halfWidthSmall = filterHalfWidths(iHalfWidth);
        filterAreaSmall = (2*halfWidthSmall+1) ^ 2;
        
        filterL = [-halfWidthLarge, -halfWidthLarge, halfWidthLarge, halfWidthLarge, 1/filterAreaLarge];
        filterS = [-halfWidthSmall, -halfWidthSmall, halfWidthSmall, halfWidthSmall, 1/filterAreaSmall];
        
        filteredLarge = integralfilter(integralImageWithBorders, filterL);
        filteredSmall = integralfilter(integralImageWithBorders, filterS);
        
        filteredSmall = filteredSmall(validYIndexes,validXIndexes);
        
%         dog = abs(filteredSmall - filteredLarge(validYIndexes,validXIndexes));
        dog = filteredLarge(validYIndexes,validXIndexes) - filteredSmall;
        
        larger = dog > dog_max;
        if any(larger(:))
            dog_max(larger) = dog(larger);
            scaleImage(larger) = filteredSmall(larger);
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
