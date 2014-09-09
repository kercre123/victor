% function [numCorrect, numTotal] = decisionTree2_testOnTrainingData(tree, featureValues, labels, labelNames, probeLocationsXGrid, probeLocationsYGrid)

function [numCorrect, numTotal] = decisionTree2_testOnTrainingData(tree, featureValues, labels, labelNames, probeLocationsXGrid, probeLocationsYGrid)
    numImages = length(featureValues{1});
    numFeatures = length(featureValues);
    featureImageWidth = sqrt(numFeatures);
    
    assert(length(labels) == numImages);
    
    tform = cp2tform(featureImageWidth*[0 0; 0 1; 1 0; 1 1], [0 0; 0 1; 1 0; 1 1], 'projective');
        
    pBar = ProgressBar('decisionTree2_train testOnTrainingData', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    pBar.set_message(sprintf('Testing %d inputs', numImages));
    pBar.set_increment(10000/numImages);
    pBar.set(0);
    
    featureValues = cell2mat(featureValues');
    
    maxLabel = max(unique(labels));
    numCorrect = zeros(maxLabel + 1, 1);
    numTotal = zeros(maxLabel + 1, 1);
    
    % Compute the location mapping
    samplePositions = zeros(length(probeLocationsXGrid), 1);
    for iFeature = 1:length(probeLocationsXGrid)
        [xp, yp] = tforminv(tform, probeLocationsXGrid(iFeature), probeLocationsYGrid(iFeature));
        samplePositions(iFeature) = sub2ind([featureImageWidth,featureImageWidth], round(yp + 0.5), round(xp + 0.5));
    end
    
    for iImage = 1:numImages
        % reshape the featureValues
        curImage = featureValues(iImage, :);
%         curImage = reshape(curImage, [featureImageWidth,featureImageWidth]);
        
        [~, labelID] = decisionTree2_queryFast(tree, curImage, samplePositions, 0, 255);
        
        numTotal(labelID) = numTotal(labelID) + 1;
        
        if labelID == labels(iImage);
            numCorrect(labelID) = numCorrect(labelID) + 1;
        end
        
        if mod(iImage, 10000) == 0
            if pBar.cancelled
                break;
            end
            
            pBar.set_message(sprintf('Testing %d inputs. Current accuracy %d/%d=%f', numImages, sum(numCorrect), iImage, sum(numCorrect)/iImage));
            pBar.increment();
        end
    end
    
    assert(iImage == sum(numTotal));
    
    labelNames{end+1} = 'Unknown';
    
    assert(length(numCorrect) == length(labelNames) || length(numCorrect) == (length(labelNames)+1));
    
    % Sort from worst to best accuracy
    all = zeros(2, 461); 
    all(1,:) = numCorrect./numTotal; 
    all(2,:) = 1:461;
    all = sortrows(all', 1);
    all = all(:,2);
    
    for iName = 1:length(numCorrect)
        disp(sprintf('%s %d/%d=%f', labelNames{all(iName)}, numCorrect(all(iName)), numTotal(all(iName)), numCorrect(all(iName))/numTotal(all(iName))));
    end
    
    disp(' ');
    
    disp(sprintf('Total accuracy %d/%d = %f', sum(numCorrect), sum(numTotal), sum(numCorrect)/sum(numTotal)))
end % decisionTree2_testOnTrainingData()
