% function [numCorrect, numTotal] = decisionTree2_testOnTrainingData(tree, featureValues, labels, labelNames, probeLocationsXGrid, probeLocationsYGrid, varargin)

% Normal matlab
% [numCorrect, numTotal] = decisionTree2_testOnTrainingData(tree, featureValues, labels, labelNames, probeLocationsXGrid, probeLocationsYGrid);

% Mex
% [numCorrect, numTotal] = decisionTree2_testOnTrainingData(cTree, featureValues, labels, labelNames, probeLocationsXGrid, probeLocationsYGrid, 'useMex', true);

function [numCorrect, numTotal] = decisionTree2_testOnTrainingData(tree, featureValues, labels, labelNames, probeLocationsXGrid, probeLocationsYGrid, varargin)
    useMex = false;
    numThreads = 1;
    
    parseVarargin(varargin{:});
    
    numImages = size(featureValues,2);
    numFeatures = size(featureValues,1);
    featureImageWidth = sqrt(numFeatures);
    
    assert(length(labels) == numImages);
    
    tform = cp2tform(featureImageWidth*[0 0; 0 1; 1 0; 1 1], [0 0; 0 1; 1 0; 1 1], 'projective');
        
    pBar = ProgressBar('decisionTree2_train testOnTrainingData', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    pBar.set_message(sprintf('Testing %d inputs', numImages));
    pBar.set_increment(10000/numImages);
    pBar.set(0);   
   
    maxLabel = max(unique(labels));
    numCorrect = zeros(maxLabel + 1, 1);
    numTotal = zeros(maxLabel + 1, 1);
    
    % Compute the location mapping
    samplePositions = zeros(length(probeLocationsXGrid), 1, 'int32');
    for iFeature = 1:length(probeLocationsXGrid)
        [xp, yp] = tforminv(tform, probeLocationsXGrid(iFeature), probeLocationsYGrid(iFeature));
        samplePositions(iFeature) = sub2ind([featureImageWidth,featureImageWidth], round(yp + 0.5), round(xp + 0.5));
    end
    
    if useMex       
        whichFeatures = tree.whichFeatures;
        u8Thresholds = tree.u8Thresholds;
        leftChildIndexs = tree.leftChildIndexs;
        
        if size(whichFeatures,1) == 1
            whichFeatures = whichFeatures';
        end
        
        if size(u8Thresholds,1) == 1
            u8Thresholds = u8Thresholds';
        end
        
        if size(leftChildIndexs,1) == 1
            leftChildIndexs = leftChildIndexs';
        end
        
%         tic
        labelIds = mexDecisionTree2_testOnTrainingData_innerLoop(featureValues, whichFeatures, u8Thresholds, leftChildIndexs, samplePositions - 1, numThreads);
%         toc
        
%         tic
        labelIds(labelIds == -1) = maxLabel + 1;
%         toc
        
%         tic
        for iImage = 1:numImages
            labelID = labelIds(iImage);
            
            numTotal(labelID) = numTotal(labelID) + 1;

            if labelID == labels(iImage);
                numCorrect(labelID) = numCorrect(labelID) + 1;
            end
        end
%         toc
    else % if useMex
        for iImage = 1:numImages
            curImage = featureValues(:, iImage);

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
    end % if useMex ... else
    
    assert(iImage == sum(numTotal));
    
    labelNames{end+1} = 'Unknown';
    
    assert(length(numCorrect) == length(labelNames) || length(numCorrect) == (length(labelNames)+1));
    
    % Sort from worst to best accuracy
    all = zeros(2, length(numCorrect)); 
    all(1,:) = numCorrect./numTotal; 
    all(2,:) = 1:length(numCorrect);
    all = sortrows(all', 1);
    all = all(:,2);
    
    for iName = 1:length(numCorrect)
        disp(sprintf('%s %d/%d=%f', labelNames{all(iName)}, numCorrect(all(iName)), numTotal(all(iName)), numCorrect(all(iName))/numTotal(all(iName))));
    end
    
    disp(' ');
    
    disp(sprintf('Total accuracy %d/%d = %f', sum(numCorrect), sum(numTotal), sum(numCorrect)/sum(numTotal)))
end % decisionTree2_testOnTrainingData()
