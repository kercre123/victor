% function tree = decisionTree2_train()
%
% Based off of VisionMarkerTrained.TrainProbeTree, but split into a
% separate function, to prevent too much messing around with the
% VisionMarkerTrained class hierarchy

% As input, this function needs a list of labels and featureValues. Start with the following two lines for any example:
% fiducialClassesList = decisionTree2_createClassesList();
% [labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(fiducialClassesList);

% Example:
% [tree, minimalTree, trainingFailures] = decisionTree2_train(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, false);

% Example using only 128 as the grayvalue threshold
% [tree, minimalTree, trainingFailures] = decisionTree2_train(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, false, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);

% To train with the completely C version of this method, use useCVersion = true, and it will save the files to disk and launch the C training
% [tree, minimalTree, trainingFailures] = decisionTree2_train(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, true);

function [tree, minimalTree, trainingFailures, testOnTrain_numCorrect, testOnTrain_numTotal] = decisionTree2_train(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, useCVersion, varargin)
    global g_trainingFailures;
    global nodeId;
    global pBar;
    
    pBar = ProgressBar('decisionTree2_train', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    nodeId = 1;
    
    t_start = tic();
    
    for iFeature = 1:length(featureValues)
        assert(min(size(labels) == size(featureValues{iFeature})) == 1);
    end
    
    leafNodeFraction = 1.0; % What percent of the labels must be the same for it to be considered a leaf node?
    leafNodeNumItems = 1; % If a node has less-than-or-equal-to this number of items, it is a leaf no matter what
    u8MinDistanceForSplits = 20; % How close can the value for splitting on two values of one feature? Set to 255 to disallow splitting more than once per feature (e.g. multiple splits for one probe in the same location).
    u8MinDistanceFromThreshold = 20; % If a feature value is closer than this to the threshold, pass it to both subpaths
    u8ThresholdsToUse = []; % If set, only use these grayvalues to threshold (For example, set to [128] to only split on 128);
    featuresUsed = zeros(length(featureValues), 256, 'uint8'); % If you don't want to train on some of the features or u8 thresholds, set some of the featuresUsed to true
    cInFilenamePrefix = 'c:/tmp/treeTraining_'; % Temporary location for when useCVersion = true
    cOutFilenamePrefix = 'c:/tmp/treeTraining_out_';
    %cTrainingExecutable = 'C:/Anki/products-cozmo/build/Visual Studio 11/bin/RelWithDebInfo/run_trainDecisionTree.exe';
    cTrainingExecutable = 'C:/Anki/products-cozmo/build/Visual Studio 11/bin/Debug/run_trainDecisionTree.exe';
    maxThreads = 1;
    maxSavingThreads = 8;
    
    parseVarargin(varargin{:});
    
    u8ThresholdsToUse = uint8(u8ThresholdsToUse);
    
    if ~ispc()
        cInFilenamePrefix = strrep(cInFilenamePrefix, 'c:', tildeToPath());
        cOutFilenamePrefix = strrep(cOutFilenamePrefix, 'c:', tildeToPath());
        
        if ~isempty(strfind(cTrainingExecutable, 'C:/Anki/products-cozmo'))
            cTrainingExecutable = [tildeToPath(), '/Documents/Anki/products-cozmo/build/Xcode/bin/Debug/run_trainDecisionTree'];
        end
    end
    
    % Train the decision tree
    if useCVersion
        decisionTree2_saveInputs(cInFilenamePrefix, labelNames, labels, featureValues, featuresUsed, u8ThresholdsToUse, maxSavingThreads);
        tree = decisionTree2_runCVersion(cTrainingExecutable, cInFilenamePrefix, cOutFilenamePrefix, length(featureValues), leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8MinDistanceFromThreshold, labelNames, probeLocationsXGrid, probeLocationsYGrid, maxThreads);
    else
        tree = struct('depth', 0, 'infoGain', 0, 'remaining', int32(1:length(labels)));
        tree.remainingLabels = labelNames;
        
        tree = buildTree(tree, featuresUsed, labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8ThresholdsToUse);
    end
    
    if isempty(tree)
        error('Training failed!');
    end
    
    t_train = toc(t_start);
    
    [numInternalNodes, numLeaves] = countNumNodes(tree);
    
    disp(sprintf('Training completed in %f seconds. Tree has %d nodes.', t_train, numInternalNodes + numLeaves));
    
    minimalTree = decisionTree2_stripTree(tree);
    
    t_test = tic();
    
    [testOnTrain_numCorrect, testOnTrain_numTotal] = testOnTrainingData(tree, featureValues, labels);
    
    t_test = toc(t_test);
    
    disp(sprintf('Tested tree on training set in %f seconds. Training accuracy: %d/%d = %0.2f%%', t_test, testOnTrain_numCorrect, testOnTrain_numTotal, 100*testOnTrain_numCorrect/testOnTrain_numTotal));
    
    trainingFailures = g_trainingFailures;
    
    %     keyboard
end % decisionTree2_train()

% Save the inputs to file, for running the complete C version of the training
% cInFilenamePrefix should be the prefix for all the files to save, such as '~/tmp/trainingFiles_'
function decisionTree2_saveInputs(cInFilenamePrefix, labelNames, labels, featureValues, featuresUsed, u8ThresholdsToUse, maxSavingThreads)
    global pBar;
    
    totalTic = tic();
    
    pBar.set_message('Saving inputs for C training');
    pBar.set_increment(1/(length(featureValues)+4));
    pBar.set(0);
    
    if size(labelNames, 1) ~= 1
        labelNames = labelNames';
    end
    
    if size(labels, 1) ~= 1
        labels = labels';
    end
    
    if size(u8ThresholdsToUse,1) ~= 1
        u8ThresholdsToUse = u8ThresholdsToUse';
    end
        
    mexSaveEmbeddedArray(...
        {uint8(featuresUsed), labelNames, int32(labels) - 1, uint8(u8ThresholdsToUse)},...
        {[cInFilenamePrefix, 'featuresUsed.array'], [cInFilenamePrefix, 'labelNames.array'], [cInFilenamePrefix, 'labels.array'], [cInFilenamePrefix, 'u8ThresholdsToUse.array']});
    
    pBar.increment();
    pBar.increment();
    pBar.increment();
    pBar.increment();
    
    for iFeature = 1:maxSavingThreads:length(featureValues)
        allArrays = {};
        allFilenames = {};
        
        pBar.set_message(sprintf('Saving inputs for C training %d/%d', iFeature, length(featureValues)));
        
        for iThread = 1:maxSavingThreads
            index = iFeature + iThread - 1;
            if index > length(featureValues)
                break;
            end
            
            curFeatureValues = featureValues{index};
            
            if size(curFeatureValues, 1) ~= 1
                curFeatureValues = curFeatureValues';
            end
            
            allArrays{end+1} = uint8(curFeatureValues); %#ok<AGROW>
            allFilenames{end+1} = [cInFilenamePrefix, sprintf('featureValues%d.array', index-1)]; %#ok<AGROW>
        end
        
        mexSaveEmbeddedArray(allArrays, allFilenames, 6, maxSavingThreads);
        
        for i = 1:maxSavingThreads
            pBar.increment();
        end
    end
    
    disp(sprintf('Total time to save: %f seconds', toc(totalTic)));
    
end % decisionTree2_saveInputs()

function tree = decisionTree2_runCVersion(cTrainingExecutable, cInFilenamePrefix, cOutFilenamePrefix, numFeatures, leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8MinDistanceFromThreshold, labelNames, probeLocationsXGrid, probeLocationsYGrid, maxThreads)
    % First, run the training
    trainingTic = tic();
    command = sprintf('"%s" "%s" %d %f %d %d %d %d "%s"', cTrainingExecutable, cInFilenamePrefix, numFeatures, leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8MinDistanceFromThreshold, maxThreads, cOutFilenamePrefix);
    disp(['Starting C training: ', command]);
    result = system(command);
    disp(sprintf('C training finished in %f seconds', toc(trainingTic)));
    
    if result ~= 0
        tree = [];
        return;
    end
    
    % Next, load and convert the tree into the matlab format
    convertingTic = tic();
    
    disp('Loading and converting c tree to matlab format')
    
    depths = mexLoadEmbeddedArray([cOutFilenamePrefix, 'depths.array']);
    infoGains = mexLoadEmbeddedArray([cOutFilenamePrefix, 'bestEntropys.array']);
    whichFeatures = mexLoadEmbeddedArray([cOutFilenamePrefix, 'whichFeatures.array']);
    u8Thresholds = mexLoadEmbeddedArray([cOutFilenamePrefix, 'u8Thresholds.array']);
    leftChildIndexs = mexLoadEmbeddedArray([cOutFilenamePrefix, 'leftChildIndexs.array']);
    cpuUsageSamples = mexLoadEmbeddedArray([cOutFilenamePrefix, 'cpuUsageSamples.array']);
    
    tree = decisionTree2_convertCTree(depths, infoGains, whichFeatures, u8Thresholds, leftChildIndexs, labelNames, probeLocationsXGrid, probeLocationsYGrid);
    
    figure(50);
    plot(cpuUsageSamples, 'b+');
    title('CPU usage over time');
    limits = axis();
    limits(3:4) = [0,100];
    axis(limits)
    
    disp(sprintf('Conversion done in %f seconds (average CPU usage %f%%)', toc(convertingTic), mean(cpuUsageSamples)));
end

function [numInternalNodes, numLeaves] = countNumNodes(tree)
    if isfield(tree, 'labelName')
        numInternalNodes = 0;
        numLeaves = 1;
    else
        [numInternalNodesLeft, numLeavesLeft] = countNumNodes(tree.leftChild);
        [numInternalNodesRight, numLeavesRight] = countNumNodes(tree.rightChild);
        numLeaves = numLeavesLeft + numLeavesRight;
        numInternalNodes = 1 + numInternalNodesLeft + numInternalNodesRight;
    end
end % countNumNodes()

function [numCorrect, numTotal] = testOnTrainingData(tree, featureValues, labels)
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
    
    numCorrect = 0;
    for iImage = 1:numImages
        curImage = zeros(numFeatures, 1, 'uint8');
        
        % reshape the featureValues
        for iFeature = 1:numFeatures
            curImage(iFeature) = featureValues{iFeature}(iImage);
        end
        
        curImage = reshape(curImage, [featureImageWidth,featureImageWidth]);
        
        [~, labelID] = decisionTree2_query(tree, curImage, tform, 0, 255);
        
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
end % testOnTrainingData()

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
        goLeft = featureValues{node.whichFeature}(node.remaining) < node.u8Threshold;
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
        curFeatureValues = featureValues{curFeatureIndex}(remainingImages);
        
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
