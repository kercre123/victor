% function images = probeTree2_probeValuesToImage(probeValues, imageIndexes)

function images = probeTree2_probeValuesToImage(probeValues, imageIndexes)
    numProbes = length(probeValues);
    probeImageWidth = sqrt(numProbes);
    
    images = cell(length(imageIndexes), 1); 
    
    if iscell(imageIndexes)
        imageIndexes = cell2mat(imageIndexes);
    end
    
    for iImage = 1:length(imageIndexes)
        image = zeros(numProbes, 1, 'uint8');
        
        for i = 1:numProbes
            image(i) = probeValues{i}(imageIndexes(iImage));
        end
        
        image = reshape(image, [probeImageWidth, probeImageWidth]);
        
        images{iImage} = image;
    end
    
    