% function images = decisionTree2_featureValuesToImage(featureValues, imageIndexes)

function images = decisionTree2_featureValuesToImage(featureValues, imageIndexes)
    numFeatures = size(featureValues,1);
    featureImageWidth = ceil(sqrt(numFeatures));
    
    images = cell(length(imageIndexes), 1);
    
    if iscell(imageIndexes)
        imageIndexes = cell2mat(imageIndexes);
    end
    
    for iImage = 1:length(imageIndexes)
        image = zeros([featureImageWidth, featureImageWidth], 'uint8');
        
        for i = 1:numFeatures
            image(i) = featureValues(i,imageIndexes(iImage));
        end
        
        images{iImage} = image;
    end
    