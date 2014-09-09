% function [numCorrect, numTotal] = decisionTree2_testOnTrainingData(tree, featureValues, labels, probeLocationsXGrid, probeLocationsYGrid)

function [numCorrect, numTotal] = decisionTree2_testOnTrainingData(tree, featureValues, labels, probeLocationsXGrid, probeLocationsYGrid)
    numImages = length(featureValues{1});
    numFeatures = length(featureValues);
    featureImageWidth = sqrt(numFeatures);
    
    assert(length(labels) == numImages);
    
    tform = cp2tform(featureImageWidth*[0 0; 0 1; 1 0; 1 1], [0 0; 0 1; 1 0; 1 1], 'projective');
    
    numTotal = length(labels);
    
    pBar = ProgressBar('decisionTree2_train testOnTrainingData', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    pBar.set_message(sprintf('Testing %d inputs', numImages));
    pBar.set_increment(100/numImages);
    pBar.set(0);
    
    featureValues = cell2mat(featureValues');
    
    % Compute the location mapping
    samplePositions = zeros(length(probeLocationsXGrid), 1);
    for iFeature = 1:length(probeLocationsXGrid)
        [xp, yp] = tforminv(tform, probeLocationsXGrid(iFeature), probeLocationsYGrid(iFeature));
        samplePositions(iFeature) = sub2ind([featureImageWidth,featureImageWidth], round(yp + 0.5), round(xp + 0.5));
    end
    
    numCorrect = 0;
    for iImage = 1:numImages
        % reshape the featureValues
        curImage = featureValues(iImage, :);
%         curImage = reshape(curImage, [featureImageWidth,featureImageWidth]);
        
        [~, labelID] = decisionTree2_queryFast(tree, curImage, samplePositions, 0, 255);
        
        if labelID == labels(iImage);
            numCorrect = numCorrect + 1;
        end
        
        if mod(iImage, 100) == 0
            if pBar.cancelled
                break;
            end
            
            pBar.set_message(sprintf('Testing %d inputs. Current accuracy %d/%d=%f', numImages, numCorrect, iImage, numCorrect/iImage));
            pBar.increment();
        end
    end
    
    sprintf('Total accuracy %d/%d=%f', numCorrect, iImage, numCorrect/iImage)
end % decisionTree2_testOnTrainingData()