% function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_lowMemory(image, numLevels, thresholdFraction)

%  binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_lowMemory(im, 5, 0.75);

function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_lowMemory(image, numLevels, thresholdFraction)

if ~exist('thresholdFraction' ,'var')
    thresholdFraction = 0.75;
end

numIntegralImageRowsToScroll = 32;

DEBUG_DISPLAY = false;
% DEBUG_DISPLAY = true;

assert(size(image,3)==1, 'Image should be scalar-valued.');
% scaleImage = image;
% dog_max = zeros(size(image,1), size(image,2), 'uint32');
binaryImage = zeros(size(image));

maxFilterHalfWidth = 2 ^ (numLevels+1);

integralImageBorderWidth = maxFilterHalfWidth + 1;
numIntegralImageRows = (2*maxFilterHalfWidth + 2) + numIntegralImageRowsToScroll;

scrollingIntegralImage = computeScrollingIntegralImage(image,...
    zeros(numIntegralImageRows,size(image,2)+2*integralImageBorderWidth,'int32'),...
    1,...
    numIntegralImageRows,...
    integralImageBorderWidth);

filterNormalizationConstants = zeros(numLevels+1, 1);
for pyramidLevel = 1:(numLevels+1)
    filterHalfWidth = 2^pyramidLevel;
    filterNormalizationConstants(pyramidLevel) = 1 / ((2*filterHalfWidth+1)^2);
end

blurredIms = cell(numLevels+1,1);   
for pyramidLevel = 1:(numLevels+1)
    blurredIms{pyramidLevel} = zeros(size(image), 'uint8');
end

y = 1;
while y <= size(image,1)
    filteredLines = cell(numLevels+1, 1);
    
    % TODO: make work for the scrolling II
    offsetY = y + maxFilterHalfWidth + 1;
    
    for pyramidLevel = 1:(numLevels+1)
        filterHalfWidth = 2^pyramidLevel;
        
        filteredLines{pyramidLevel} = zeros(1, size(image,2));
        for x = 1:size(image,2)
            offsetX = x + maxFilterHalfWidth + 1;
                        
            boxSum = scrollingIntegralImage(offsetY+filterHalfWidth,   offsetX+filterHalfWidth) -...
                     scrollingIntegralImage(offsetY+filterHalfWidth,   offsetX-filterHalfWidth-1) +...
                     scrollingIntegralImage(offsetY-filterHalfWidth-1, offsetX-filterHalfWidth-1) -...
                     scrollingIntegralImage(offsetY-filterHalfWidth-1, offsetX+filterHalfWidth);
            
            filteredLines{pyramidLevel}(x) = filterNormalizationConstants(pyramidLevel) * boxSum;
        end % for x = 1:size(image,2)
        blurredIms{pyramidLevel}(y,:) = filteredLines{pyramidLevel}(x);
    end % for pyramidLevel = 1:numLevels    
    
    keyboard
end % Included at the top-level config

% for y = 1:numIntegralImageRowsToScroll:size(image,1)
%     
% end % for y = 1:numIntegralImageRowsToScroll:size(image,1)

% for pyramidLevel = 1:numLevels
%     halfWidthLarge = 2 ^ pyramidLevel;
%     filterAreaLarge = (2*halfWidthLarge+1) ^ 2;
%     
%     halfWidthSmall = 2 ^ (pyramidLevel+1);    
%     filterAreaSmall = (2*halfWidthSmall+1) ^ 2;
%     
%     filterL = [-halfWidthLarge, -halfWidthLarge, halfWidthLarge, halfWidthLarge, 1/filterAreaLarge];
%     filterS = [-halfWidthSmall, -halfWidthSmall, halfWidthSmall, halfWidthSmall, 1/filterAreaSmall];
%     
%     filteredLarge = integralfilter(integralImageWithBorders, filterL);
%     filteredSmall = integralfilter(integralImageWithBorders, filterS);
%         
%     filteredSmall = filteredSmall(validYIndexes,validXIndexes);
%     
%     dog = abs(filteredSmall - filteredLarge(validYIndexes,validXIndexes));
%        
%     larger = dog > dog_max;
%     if any(larger(:))
%         dog_max(larger) = dog(larger);
%         scaleImage(larger) = filteredSmall(larger);
%         if nargout > 1 || nargout == 0
%             whichScale(larger) = pyramidLevel;
%         end
%     end
% 
%     if DEBUG_DISPLAY
%         figureHandle = figure(100+pyramidLevel); subplot(2,4,1); imshow(filteredSmall); subplot(2,4,3); imshow(filteredLarge); subplot(2,4,5); imshow(dog*5); subplot(2,4,7); imshow(scaleImage);
%         set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]) 
%     end
% end
% 
% binaryImage = image < thresholdFraction*scaleImage;

end % FUNCTION computeCharacteristicScaleImage()