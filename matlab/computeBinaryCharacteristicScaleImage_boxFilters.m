% function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters(image, numLevels, thresholdFraction)

%  binaryImage = computeBinaryCharacteristicScaleImage_boxFilters(im, 5, 0.75);

function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters(image, numLevels, thresholdFraction)

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

integralIm = integralimage(image);

for pyramidLevel = 1:numLevels
    halfWidthLarge = 2 ^ pyramidLevel;
    filterAreaLarge = (2*halfWidthLarge+1) ^ 2;
    
    halfWidthSmall = 2 ^ (pyramidLevel+1);    
    filterAreaSmall = (2*halfWidthSmall+1) ^ 2;
    
    filterL = [-halfWidthLarge, -halfWidthLarge, halfWidthLarge, halfWidthLarge, 1/filterAreaLarge];
    filterS = [-halfWidthSmall, -halfWidthSmall, halfWidthSmall, halfWidthSmall, 1/filterAreaSmall];
    
    filteredLarge = integralfilter(integralIm, filterL);
    filteredSmall = integralfilter(integralIm, filterS);
    
    dog = abs(filteredSmall - filteredLarge);
       
    larger = dog > dog_max;
    if any(larger(:))
        dog_max(larger) = dog(larger);
        scaleImage(larger) = filteredSmall(larger);
        if nargout > 1 || nargout == 0
            whichScale(larger) = pyramidLevel;
        end
    end

    if DEBUG_DISPLAY
        figureHandle = figure(100+pyramidLevel); subplot(2,4,1); imshow(filteredSmall); subplot(2,4,3); imshow(filteredLarge); subplot(2,4,5); imshow(dog*5); subplot(2,4,7); imshow(scaleImage);
        set(figureHandle, 'Units', 'normalized', 'Position', [0, 0, 1, 1]) 
    end
end

binaryImage = image < thresholdFraction*scaleImage;

end % FUNCTION computeCharacteristicScaleImage()