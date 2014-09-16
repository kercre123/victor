
% [labelName, labelID] = svm_predict(trainedSvmClassifiers, featureValues(:,1), [], probeLocationsXGrid, probeLocationsYGrid, labelNames, 0, 255)

function [labelName, labelID] = svm_predict(trainedSvmClassifiers, img, tform, probeLocationsXGrid, probeLocationsYGrid, labelNames, blackValue, whiteValue)
    
    featureVector = zeros(1, length(probeLocationsXGrid));
    
    % If img is just a feature vector, we don't need to sample
    if max(size(img)) == length(img)
        featureVector = double(img);
        
        if size(featureVector, 2) == 1
            featureVector = featureVector';
        end
    else
        for iFeature = 1:length(probeLocationsXGrid)
            [xp, yp] = tforminv(tform, probeLocationsXGrid(iFeature), probeLocationsYGrid(iFeature));
            xp = round(xp + 0.5);
            yp = round(yp + 0.5);
            
            minSubtractedValue = int32(img(yp,xp)) - blackValue;
            normalizedPixelValue = (minSubtractedValue * 255) / (whiteValue - blackValue);
            normalizedPixelValue = uint8(normalizedPixelValue);
            
            featureVector(iFeature) = normalizedPixelValue;
        end
    end
    
    predicted_labels = zeros(length(trainedSvmClassifiers), 1);
    
    labelName = {};
    labelID = [];
    
    for iClassfier = 1:length(trainedSvmClassifiers)
        predicted_labels(iClassfier) = svmpredict(zeros(1,1), featureVector, trainedSvmClassifiers{iClassfier}, '-q');
        
        if predicted_labels(iClassfier) == 1
            labelID(end+1) = iClassfier; %#ok<AGROW>
            labelName{end+1} = labelNames{iClassfier}; %#ok<AGROW>
        end
    end
    
    if length(labelName) == 1
        labelName = labelName{1};
    end
    
    
    