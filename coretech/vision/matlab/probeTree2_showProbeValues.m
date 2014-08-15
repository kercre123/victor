% function probeTree2_showProbeValues(probeValues, probeTree)

function probeTree2_showProbeValues(probeValues, probeTree)
    
    squareWidth = sqrt(length(probeValues));
    
    numImages = length(probeValues{1});
    
    numImagesX = ceil(sqrt(numImages));
    numImagesY = ceil(numImages / numImagesX);
    
    for iImage = 1:numImages
        subplot(numImagesX, numImagesY, iImage);
        curImage = zeros(length(probeValues), 1);
        
        for iProbe = 1:length(probeValues)
            curImage(iProbe) = probeValues{iProbe}(iImage);
        end
        
        curImage = reshape(curImage, [squareWidth, squareWidth]);
        
        keyboard
        imshow(curImage);
%         drawTree(probeTree, squareWidth)
    end
    
    keyboard
end  % probeTree2_showProbeValues()
