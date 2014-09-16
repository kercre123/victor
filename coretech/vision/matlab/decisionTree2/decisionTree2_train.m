% function tree = decisionTree2_train()
%
% Based off of VisionMarkerTrained.TrainProbeTree, but split into a
% separate function, to prevent too much messing around with the
% VisionMarkerTrained class hierarchy

% As input, this function needs a list of labels and featureValues. Start with the following two lines for any example:
% fiducialClassesList = decisionTree2_createClassesList();
% [labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(fiducialClassesList);

% Example:
% [tree, minimalTree, trainingFailures] = decisionTree2_train(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid);

% Example using only 128 as the grayvalue threshold
% [tree, minimalTree, trainingFailures] = decisionTree2_train(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);

function [tree, minimalTree, trainingFailures, testOnTrain_numCorrect, testOnTrain_numTotal] = decisionTree2_train(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, varargin)
    global g_trainingFailures;
    global nodeId;
    global pBar;
    
    pBar = ProgressBar('decisionTree2_train', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    nodeId = 1;
    
    t_start = tic();
    
    assert(size(labels,2) == 1);
    assert(size(labels,1) == size(featureValues, 2));
    
    leafNodeFraction = 1.0; % What percent of the labels must be the same for it to be considered a leaf node?
    leafNodeNumItems = 1; % If a node has less-than-or-equal-to this number of items, it is a leaf no matter what
    u8MinDistanceForSplits = 20; % How close can the value for splitting on two values of one feature? Set to 255 to disallow splitting more than once per feature (e.g. multiple splits for one probe in the same location).
    u8MinDistanceFromThreshold = 2; % If a feature value is closer than this to the threshold, pass it to both subpaths
    u8ThresholdsToUse = []; % If set, only use these grayvalues to threshold (For example, set to [128] to only split on 128);
    featuresUsed = zeros(size(featureValues,1), 256, 'uint8'); % If you don't want to train on some of the features or u8 thresholds, set some of the featuresUsed to true
    
    parseVarargin(varargin{:});
    
    u8ThresholdsToUse = uint8(u8ThresholdsToUse);
    
    % Train the decision tree
    tree = struct('depth', 0, 'infoGain', 0, 'remaining', int32(1:length(labels)));
    tree.remainingLabels = labelNames;
    
    tree = buildTree(tree, featuresUsed, labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8ThresholdsToUse);
    
    if isempty(tree)
        error('Training failed!');
    end
    
    t_train = toc(t_start);
    
    [numInternalNodes, numLeaves] = decisionTree2_countNumNodes(tree);
    
    disp(sprintf('Training completed in %f seconds. Tree has %d nodes.', t_train, numInternalNodes + numLeaves));
    
    minimalTree = decisionTree2_stripTree(tree);
    
    t_test = tic();
    
    [testOnTrain_numCorrect, testOnTrain_numTotal] = decisionTree2_testOnTrainingData(cTree, featureValues, labels, labelNames, probeLocationsXGrid, probeLocationsYGrid);
    
    t_test = toc(t_test);
    
    disp(sprintf('Tested tree on training set in %f seconds.', t_test));
    
    trainingFailures = g_trainingFailures;
    
    %     keyboard
end % decisionTree2_train()

function node = buildTree(node, featuresUsed, labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8ThresholdsToUse)
    global g_trainingFailures;
    global nodeId;
    global pBar;
    
    maxDepth = inf;
    
    %if all(labels(node.remaining)==labels(node.remaining(1)))
    counts = hist(labels(node.remaining), 1:length(labelNames));
    [maxCount, maxIndex] = max(counts);
    
    node.nodeId = nodeId;
    nodeId = nodeId + 1;
    
    if (maxCount >= leafNodeFraction*length(node.remaining)) || (length(node.remaining) < leafNodeNumItems)
        % Are more than leafNodeFraction percent of the remaining labels the same? If so, we're done.
        
        node.labelID = maxIndex(1);
        node.labelName = labelNames{node.labelID};
        
        % Comment or uncomment the next line as desired
        fprintf('LeafNode for label = %d, or "%s" at depth %d\n', node.labelID, node.labelName, node.depth);
        
        pBar.add(length(node.remaining) / length(labels));
    elseif node.depth == maxDepth || all(featuresUsed(:))
        % Have we reached the max depth, or have we used all features? If so, we're done.
        
        node.labelID = mexUnique(labels(node.remaining));
        node.labelName = labelNames(node.labelID);
        
        if isempty(g_trainingFailures)
            g_trainingFailures = node;
        else
            g_trainingFailures(end+1) = node;
        end
        
        fprintf('MaxDepth LeafNode for labels = {%s\b}  {%s\b} at depth %d\n', sprintf('%s,', node.labelName{:}), sprintf('%d,', node.remaining), node.depth);
        pBar.add(length(node.remaining) / length(labels));
    else
        % We have unused features. So find the best one to split on.
        
        useMex = true;
        %         useMex = false;
        numThreads = 16;
        
        [node.infoGain, node.whichFeature, node.u8Threshold, updatedFeaturesUsed] = computeInfoGain(labels, featureValues, featuresUsed, node.remaining, u8MinDistanceForSplits, u8ThresholdsToUse, useMex, numThreads);
        
        node.remainingLabels = sprintf('%s ', labelNames{mexUnique(labels(node.remaining))});
        
        % If the entropy is incredibly large, there was no valid split
        if node.infoGain > 100000.0 || node.whichFeature < 1
            % PB: Is this still a reasonable thing to happen? I think no.
            
            node.labelID = mexUnique(labels(node.remaining));
            node.labelName = labelNames(node.labelID);
            
            if isempty(g_trainingFailures)
                g_trainingFailures = node;
            else
                g_trainingFailures = [g_trainingFailures, node];
            end
            
            fprintf('Could not split LeafNode for labels = {%s\b} {%s\b} at depth %d\n', sprintf('%s,', node.labelName{:}), sprintf('%d,', node.remaining), node.depth);
            pBar.add(length(node.remaining) / length(labels));
            
            %debugging stuff
            images = decisionTree2_featureValuesToImage(featureValues, node.remaining);
            imshows(images);
            title(sprintf('Could not split:\n\n%s', sprintf('%s\n', node.labelName{:})));
            pause(.01);
            %             keyboard
            
            return;
        end
        
        node.x = probeLocationsXGrid(node.whichFeature);
        node.y = probeLocationsYGrid(node.whichFeature);
        
        featuresUsed = updatedFeaturesUsed;
        
        % Recurse left and right from this node
        goLeft = featureValues(node.whichFeature,node.remaining) < node.u8Threshold;
        leftRemaining = node.remaining(goLeft);
        
        % PB: Is this still ever a reasonable thing to happen? I think this would be from a bug now.
        assert(~(isempty(leftRemaining) || length(leftRemaining) == length(node.remaining)));
        
        % Recurse left
        leftChild.remaining = leftRemaining;
        leftChild.depth = node.depth+1;
        node.leftChild = buildTree(leftChild, featuresUsed, labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8ThresholdsToUse);
        
        % Recurse right
        rightRemaining = node.remaining(~goLeft);
        
        % TODO: is this ever possible?
        assert(~isempty(rightRemaining) && length(rightRemaining) < length(node.remaining));
        
        rightChild.remaining = rightRemaining;
        rightChild.depth = node.depth + 1;
        node.rightChild = buildTree(rightChild, featuresUsed, labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8ThresholdsToUse);
    end % end We have unused features. So find the best one to split on
end % buildTree()

function [bestEntropy, bestU8Threshold] = computeInfoGain_innerLoop(curLabels, curFeatureValues, u8Thresholds)
    bestEntropy = Inf;
    bestU8Threshold = -1;
    
    for iU8Threshold = 1:length(u8Thresholds)
        labelsLessThan = curLabels(curFeatureValues < u8Thresholds(iU8Threshold));
        labelsGreaterThan = curLabels(curFeatureValues >= u8Thresholds(iU8Threshold));
        
        if isempty(labelsLessThan) || isempty(labelsGreaterThan)
            continue
        end
        
        [~, countsLessThan] = count_unique(labelsLessThan);
        [~, countsGreaterThan] = count_unique(labelsGreaterThan);
        
        numLessThan = sum(countsLessThan);
        numGreaterThan = sum(countsGreaterThan);
        
        probabilitiesLessThan = countsLessThan / sum(countsLessThan);
        probabilitiesGreaterThan = countsGreaterThan / sum(countsGreaterThan);
        
        entropyLessThan = -sum(probabilitiesLessThan .* log2(max(eps, probabilitiesLessThan)));
        entropyGreaterThan = -sum(probabilitiesGreaterThan .* log2(max(eps, probabilitiesGreaterThan)));
        
        weightedAverageEntropy = ...
            numLessThan    / (numLessThan + numGreaterThan) * entropyLessThan +...
            numGreaterThan / (numLessThan + numGreaterThan) * entropyGreaterThan;
        
        if weightedAverageEntropy < bestEntropy
            bestEntropy = weightedAverageEntropy;
            bestU8Threshold = u8Thresholds(iU8Threshold);
        end
    end % for iU8Threshold = 1:length(u8Thresholds)
end % computeInfoGain_innerLoop()

function [bestEntropy, bestU8Threshold, bestFeatureIndex] = computeInfoGain_outerLoop(featureValues, featuresUsed, unusedFeatures, numFeaturesUnused, remainingImages, curLabels, maxLabel, u8ThresholdsToUse, numWorkItems, useMex, numThreads)
    bestEntropy = Inf;
    bestFeatureIndex = -1;
    bestU8Threshold = -1;
    
    for iUnusedFeature = 1:numFeaturesUnused
        curFeatureIndex = unusedFeatures(iUnusedFeature);
        curFeatureValues = featureValues(curFeatureIndex,remainingImages)';
        
        if numWorkItems > 10000000 % 10,000,000 takes about 3 seconds
            if mod(iUnusedFeature,100) == 0
                fprintf('%d',iUnusedFeature);
                pause(.001);
            elseif mod(iUnusedFeature,25) == 0
                fprintf('.');
                pause(.001);
            end
        end
        
        if isempty(u8ThresholdsToUse)
            uniqueU8Thresholds = mexUnique(curFeatureValues);
            
            if length(uniqueU8Thresholds) <= 1
                continue;
            end
            
            u8Thresholds = (int32(uniqueU8Thresholds(2:end)) + int32(uniqueU8Thresholds(1:(end-1)))) / 2;
            
            unusedU8Thresholds = find(~featuresUsed(iUnusedFeature, :)) - 1;
            
            % Compute the intersection between the u8 thresholds and the thresholds not used
            u8ThresholdCounts = zeros([256,1], 'int32');
            u8ThresholdCounts(u8Thresholds + 1) = int32(1);
            u8ThresholdCounts(unusedU8Thresholds + 1) = u8ThresholdCounts(unusedU8Thresholds + 1) + int32(1);
            u8Thresholds = uint8(find(u8ThresholdCounts == int32(2)) - 1);
            
        else
            u8Thresholds = zeros(0,1,'uint8');
            
            for i = 1:length(u8ThresholdsToUse)
                if ~featuresUsed(iUnusedFeature, u8ThresholdsToUse(i)+1)
                    u8Thresholds(end+1) = uint8(u8ThresholdsToUse(i)); %#ok<AGROW>
                end
            end
        end
        
        if isempty(u8Thresholds)
            continue;
        end
        
        if useMex
            [curBestEntropy, curbestU8Threshold] = mexComputeInfoGain2_innerLoop(curLabels, curFeatureValues, u8Thresholds, maxLabel, numThreads);
        else
            [curBestEntropy, curbestU8Threshold] = computeInfoGain_innerLoop(curLabels, curFeatureValues, u8Thresholds);
        end
        
        % The extra tiny amount is to make the result more consistent between C and Matlab, and methods with different amounts of precision
        if curBestEntropy < (bestEntropy - 1e-5)
            bestEntropy = curBestEntropy;
            bestFeatureIndex = curFeatureIndex;
            bestU8Threshold = curbestU8Threshold;
        end
    end % for iUnusedFeature = 1:numFeaturesUnused
end % computeInfoGain_outerLoop()

function [bestEntropy, bestFeatureIndex, bestU8Threshold, featuresUsed] = computeInfoGain(labels, featureValues, featuresUsed, remainingImages, u8MinDistanceForSplits, u8ThresholdsToUse, useMex, numThreads)
    totalTic = tic();
    
    unusedFeatures = find(~min(featuresUsed,[],2));
    
    numFeaturesUnused = length(unusedFeatures);
    
    curLabels = labels(remainingImages);
    uniqueLabels = mexUnique(curLabels);
    maxLabel = max(uniqueLabels);
    
    numWorkItems = length(remainingImages) * numFeaturesUnused;
    
    fprintf('Testing %d features on %d images ', numFeaturesUnused, length(remainingImages));
    
    [bestEntropy, bestU8Threshold, bestFeatureIndex] = computeInfoGain_outerLoop(featureValues, featuresUsed, unusedFeatures, numFeaturesUnused, remainingImages, curLabels, maxLabel, u8ThresholdsToUse, numWorkItems, useMex, numThreads);
    
    u8ThresholdsToMask = max(1, min(256, (1 + ((bestU8Threshold-u8MinDistanceForSplits):(bestU8Threshold+u8MinDistanceForSplits)))));
    
    if bestFeatureIndex > 0
        featuresUsed(bestFeatureIndex, u8ThresholdsToMask) = true;
    end
    
    fprintf(' Best entropy is %f in %f seconds\n', bestEntropy, toc(totalTic))
    
end % computeInfoGain()
