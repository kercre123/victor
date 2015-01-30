% function exhaustiveAlignment()

% [bestDy, bestDx, bestDyIndex, bestDxIndex, meanDifferences] = exhaustiveAlignment(ims(:,:,1), ims(:,:,2), 9);

function [bestDy, bestDx, bestDyIndex, bestDxIndex, meanDifferences] = exhaustiveAlignment(templateImage, queryImage, maxOffset)
    
    templateImage = rgb2gray2(templateImage);
    queryImage = rgb2gray2(queryImage);
    
    offsets = (-maxOffset):maxOffset;
    
    differences = zeros([size(templateImage), length(offsets), length(offsets)], 'int16');
    
    for idy = 1:length(offsets)
        dy = offsets(idy);
        
        for idx = 1:length(offsets)
            dx = offsets(idx);
            
            differences(:,:,idy,idx) = exhuastiveAlignment_shiftImage(templateImage, 0, 0, maxOffset) - exhuastiveAlignment_shiftImage(queryImage, dy, dx, maxOffset);
        end
    end
    
    meanDifferences = squeeze(mean(mean(abs(differences))));
    
    minDifference = min(meanDifferences(:));
    
    [bestDyIndex, bestDxIndex] = find(meanDifferences == minDifference);
    
    bestDy = offsets(bestDyIndex);
    bestDx = offsets(bestDxIndex);
end % function exhaustiveAlignment()



