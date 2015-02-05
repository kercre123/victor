% function exhaustiveAlignment()

% maxOffset = 9;
% [bestDy, bestDx, bestDyIndex, bestDxIndex, meanDifferences] = exhaustiveAlignment(ims(:,:,1), ims(:,:,2), (-maxOffset):maxOffset, (-maxOffset):maxOffset, maxOffset);

function [bestDy, bestDx, bestDyIndex, bestDxIndex, meanDifferences] = exhaustiveAlignment(templateImage, queryImage, offsetsY, offsetsX, borderWidth, validRectangle)
    
    if ~exist('validRectangle', 'var') || isempty(validRectangle)
        validRectangle = [1, size(templateImage,2), 1, size(templateImage,1)];
    end
        
    templateImage = rgb2gray2(templateImage);
    queryImage = rgb2gray2(queryImage);
    
    assert(max(abs(offsetsX)) <= borderWidth)
    assert(max(abs(offsetsY)) <= borderWidth)
    
%     differences = zeros([size(templateImage), length(offsets), length(offsets)], 'int16');
    differences = zeros([size(templateImage(validRectangle(1):validRectangle(2),validRectangle(3):validRectangle(4),:)), length(offsetsY), length(offsetsX)], 'int16');

    for idy = 1:length(offsetsY)
        dy = offsetsY(idy);
        
        for idx = 1:length(offsetsX)
            dx = offsetsX(idx);
            
            curDifferences = exhuastiveAlignment_shiftImage(templateImage, 0, 0, borderWidth) - exhuastiveAlignment_shiftImage(queryImage, dy, dx, borderWidth);            
            differences(:,:,idy,idx) = curDifferences(validRectangle(3):validRectangle(4), validRectangle(1):validRectangle(2), :);
        end
    end
    
    meanDifferences = squeeze(mean(mean(abs(differences))));
    
    minDifference = min(meanDifferences(:));
    
    [bestDyIndex, bestDxIndex] = find(meanDifferences == minDifference);
    
    bestDy = offsetsY(bestDyIndex);
    bestDx = offsetsX(bestDxIndex);
end % function exhaustiveAlignment()



