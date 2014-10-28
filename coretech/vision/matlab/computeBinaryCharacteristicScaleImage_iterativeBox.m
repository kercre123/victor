% function binaryImage = computeBinaryCharacteristicScaleImage_iterativeBox()

% binaryImage = computeBinaryCharacteristicScaleImage_iterativeBox(im, 5, 0.75, false);

function binaryImage = computeBinaryCharacteristicScaleImage_iterativeBox(image, boxWidth, numIterations, thresholdFraction, usePyramid)
    
    if ~exist('thresholdFraction' ,'var')
        thresholdFraction = 0.75;
    end
    
    DEBUG_DISPLAY = false;
    % DEBUG_DISPLAY = true;
    
    assert(size(image,3)==1, 'Image should be scalar-valued.');
    [nrows,ncols] = size(image);
    scaleImage = image;
    dog_max = zeros(nrows,ncols);
    
%     figure(); imshow(image);
    
    filteredImage = image;
    for iIteration = 1:numIterations
        filter = ones(boxWidth, boxWidth);
        filter = filter / sum(filter(:));
        
        filteredImage2 = imfilter(filteredImage, filter, 'replicate');
        
        if usePyramid
            filteredImageLarge = imresize(filteredImage, size(image), 'nearest');
            filteredImage2Large = imresize(filteredImage2, size(image), 'nearest');
        else
            filteredImageLarge = filteredImage2;
            filteredImage2Large = filteredImage;
        end

%         figure();
%         subplot(1,2,1); imshow(uint8(filteredImageLarge));
%         subplot(1,2,2); imshow(uint8(filteredImage2Large));
        
        dog = filteredImageLarge - filteredImage2Large;
        
        larger = dog > dog_max;
        if any(larger(:))
            dog_max(larger) = dog(larger);
            scaleImage(larger) = filteredImageLarge(larger);
        end
        
%         filteredImage = filteredImage2;
        if usePyramid
            filteredImage = imresize_bilinear(filteredImage2, floor(size(filteredImage2)/2));
        else
            filteredImage = filteredImage2;
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
