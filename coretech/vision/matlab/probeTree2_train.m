% function probeTree = probeTree2_train()
%
% Based off of VisionMarkerTrained.TrainProbeTree, but split into a
% separate function, to prevent too much messing around with the
% VisionMarkerTrained class hierarchy

% Example:
% [probeTree, minimalProbeTree, trainingFailures] = probeTree2_train(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, false);

% Example using only 128 as the grayvalue threshold
% [probeTree, minimalProbeTree, trainingFailures] = probeTree2_train(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, false, 'minGrayvalueDistance', 1000, 'grayvalueThresholdsToUse', 128);

% To train with the completely C version of this method, use useCVersion = true, and it will save the files to disk and launch the C training
% [probeTree, minimalProbeTree, trainingFailures] = probeTree2_train(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, true);

function [probeTree, minimalProbeTree, trainingFailures] = probeTree2_train(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, useCVersion, varargin)
    global g_trainingFailures;
    global nodeId;
    global pBar;
    
    pBar = ProgressBar('probeTree2_train', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    nodeId = 1;
    
    t_start = tic();
    
    for iProbe = 1:length(probeValues)
        assert(min(size(labels) == size(probeValues{iProbe})) == 1);
    end
    
    leafNodeFraction = 1.0; % What percent of the labels must be the same for it to be considered a leaf node?
    leafNodeNumItems = 1; % If a node has less-than-or-equal-to this number of items, it is a leaf no matter what
    minGrayvalueDistance = 20; % How close can the grayvalue of two probes in the same location be? Set to 1000 to disallow probes in the same location.
    grayvalueThresholdsToUse = []; % If set, only use these grayvalues to threshold (For example, set to [128] to only split on 128);
    probesUsed = zeros(length(probeValues), 256, 'uint8'); % If you don't want to train on some of the probes or grayvalue thresholds, set some of the probesUsed to true
    cFilenamePrefix = 'c:/tmp/treeTraining_'; % Temporarly location for when useCVersion = true
    cTrainingExecutable = 'C:/Anki/products-cozmo/build/Visual Studio 11/bin/Debug/run_trainDecisionTree.exe';
    
    parseVarargin(varargin{:});
    
    grayvalueThresholdsToUse = uint8(grayvalueThresholdsToUse);
    
    if ~ispc()
        baseCFilename = strrep(cFilenamePrefix, 'c:', '~');
    end
    
    % Train the decision tree
    if useCVersion
        %         probeTree2_saveInputs(cFilenamePrefix, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, probesUsed, grayvalueThresholdsToUse);
        probeTree = probeTree2_runCVersion(cTrainingExecutable, cFilenamePrefix, length(probeValues), leafNodeFraction, leafNodeNumItems, minGrayvalueDistance, labelNames);
    else
        probeTree = struct('depth', 0, 'infoGain', 0, 'remaining', int32(1:length(labels)));
        probeTree.labels = labelNames;
        
        probeTree = buildTree(probeTree, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance, grayvalueThresholdsToUse);
    end
    
    if isempty(probeTree)
        error('Training failed!');
    end
    
    t_train = toc(t_start);
    
    numNodes = countNumNodes(probeTree);
    
    disp(sprintf('Training completed in %f seconds. Tree has %d nodes.', t_train, numNodes));
    
    minimalProbeTree = probeTree2_stripProbeTree(probeTree);
    
    t_test = tic();
    
    [numCorrect, numTotal] = testOnTrainingData(minimalProbeTree, probeValues, labels);
    
    t_test = toc(t_test);
    
    disp(sprintf('Tested tree on training set in %f seconds. Training accuracy: %d/%d = %0.2f%%', t_test, numCorrect, numTotal, 100*numCorrect/numTotal));
    
    trainingFailures = g_trainingFailures;
    
    %     keyboard
end % probeTree2_train()

% Save the inputs to file, for running the complete C version of the training
% cFilenamePrefix should be the prefix for all the files to save, such as '~/tmp/trainingFiles_'
function probeTree2_saveInputs(cFilenamePrefix, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, probesUsed, grayvalueThresholdsToUse)
    global pBar;
    
    pBar.set_message('Saving inputs for C training');
    pBar.set_increment(1/(length(probeValues)+6));
    pBar.set(0);
    
    mexSaveEmbeddedArray(uint8(probesUsed), [cFilenamePrefix, 'probesUsed.array']); % const std::vector<GrayvalueBool> &probesUsed, //< numProbes x 256
    pBar.increment();
    
    mexSaveEmbeddedArray(labelNames, [cFilenamePrefix, 'labelNames.array']); % const FixedLengthList<const char *> &labelNames, //< Lookup table between index and string name
    pBar.increment();
    
    mexSaveEmbeddedArray(int32(labels) - 1, [cFilenamePrefix, 'labels.array']); % const FixedLengthList<s32> &labels, //< The label for every item to train on (very large)
    pBar.increment();
    
    mexSaveEmbeddedArray(single(probeLocationsXGrid), [cFilenamePrefix, 'probeLocationsXGrid.array']);
    pBar.increment();
    
    mexSaveEmbeddedArray(single(probeLocationsYGrid), [cFilenamePrefix, 'probeLocationsYGrid.array']);
    pBar.increment();
    
    mexSaveEmbeddedArray(uint8(grayvalueThresholdsToUse), [cFilenamePrefix, 'grayvalueThresholdsToUse.array']);
    pBar.increment();
    
    for iProbe = 1:length(probeValues)
        mexSaveEmbeddedArray(uint8(probeValues{iProbe}), [cFilenamePrefix, sprintf('probeValues%d.array', iProbe-1)]);     % const FixedLengthList<const FixedLengthList<u8> > &probeValues, //< For every probe, the value for the probe every item to train on (outer is small, inner is very large)
        pBar.increment();
    end
end % probeTree2_saveInputs()

function probeTree = probeTree2_runCVersion(cTrainingExecutable, cFilenamePrefix, numProbes, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance, labelNames)
    % First, run the training
    trainingTic = tic();
    command = sprintf('"%s" "%s" %d %f %d %d', cTrainingExecutable, cFilenamePrefix, numProbes, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance);
    disp(['Starting C training: ', command]);
    result = system(command);
    disp(sprintf('C training finished in %f seconds', toc(trainingTic)));
    
    if result ~= 0
        probeTree = [];
        return;
    end
    
    % Next, load and convert the tree into the matlab format
    convertingTic = tic();
    
    disp(['Loading and converting c tree to matlab format'])
    
    depths = mexLoadEmbeddedArray([cFilenamePrefix, 'out_depths.array']);
    infoGains = mexLoadEmbeddedArray([cFilenamePrefix, 'out_infoGains.array']);
    whichProbes = mexLoadEmbeddedArray([cFilenamePrefix, 'out_whichProbes.array']);
    grayvalueThresholds = mexLoadEmbeddedArray([cFilenamePrefix, 'out_grayvalueThresholds.array']);
    xs = mexLoadEmbeddedArray([cFilenamePrefix, 'out_xs.array']);
    ys = mexLoadEmbeddedArray([cFilenamePrefix, 'out_ys.array']);
    leftChildIndexs = mexLoadEmbeddedArray([cFilenamePrefix, 'out_leftChildIndexs.array']);
    
    disp(sprintf('Conversion done in %f seconds', toc(convertingTic)));
    
    probeTree = convertCTree(1, depths, infoGains, whichProbes, grayvalueThresholds, xs, ys, leftChildIndexs, labelNames);
end

function probeTree = convertCTree(curIndex, depths, infoGains, whichProbes, grayvalueThresholds, xs, ys, leftChildIndexs, labelNames)
    
    if leftChildIndexs(curIndex) < 0
        labelId = (-leftChildIndexs(curIndex)) - 1000000 + 1;
        labelName = labelNames(labelId);
        
        probeTree = struct(...
            'depth', depths(curIndex),...
            'labelName', labelName,...
            'labelID', labelId);
    else
        probeTree = struct(...
            'depth', depths(curIndex),...
            'infoGain', infoGains(curIndex),...
            'whichProbe', whichProbes(curIndex) + 1,...
            'grayvalueThreshold', grayvalueThresholds(curIndex),...
            'x', double(xs(curIndex)),...
            'y', double(ys(curIndex)));
        
        probeTree.leftChild = convertCTree(leftChildIndexs(curIndex) + 1, depths, infoGains, whichProbes, grayvalueThresholds, xs, ys, leftChildIndexs, labelNames);
        probeTree.rightChild = convertCTree(leftChildIndexs(curIndex) + 2, depths, infoGains, whichProbes, grayvalueThresholds, xs, ys, leftChildIndexs, labelNames);
    end
end

function numNodes = countNumNodes(probeTree)
    if isfield(probeTree, 'labelName')
        numNodes = 1;
    else
        numNodes = countNumNodes(probeTree.leftChild) + countNumNodes(probeTree.rightChild);
    end
end % countNumNodes()

function [numCorrect, numTotal] = testOnTrainingData(probeTree, probeValues, labels)
    numImages = length(probeValues{1});
    numProbes = length(probeValues);
    probeImageWidth = sqrt(numProbes);
    
    assert(length(labels) == numImages);
    
    tform = cp2tform(probeImageWidth*[0 0; 0 1; 1 0; 1 1], [0 0; 0 1; 1 0; 1 1], 'projective');
    
    numTotal = length(labels);
    
    pBar = ProgressBar('probeTree2_train testOnTrainingData', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    pBar.set_message(sprintf('Testing %d inputs', numImages));
    pBar.set_increment(100/numImages);
    pBar.set(0);
    
    numCorrect = 0;
    for iImage = 1:numImages
        curImage = zeros(numProbes, 1, 'uint8');
        
        % reshape the probeValues
        for iProbe = 1:numProbes
            curImage(iProbe) = probeValues{iProbe}(iImage);
        end
        
        curImage = reshape(curImage, [probeImageWidth,probeImageWidth]);
        
        [~, labelID] = probeTree2_query(probeTree, curImage, tform, 0, 255);
        
        if labelID == labels(iImage);
            numCorrect = numCorrect + 1;
        else
            keyboard
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

function node = buildTree(node, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance, grayvalueThresholdsToUse)
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
    elseif node.depth == maxDepth || all(probesUsed(:))
        % Have we reached the max depth, or have we used all probes? If so, we're done.
        
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
        % We have unused probe location. So find the best one to split on.
        
        useMex = true;
        numThreads = 16;
        
        [node.infoGain, node.whichProbe, node.grayvalueThreshold, updatedProbesUsed] = computeInfoGain(labels, probeValues, probesUsed, node.remaining, minGrayvalueDistance, grayvalueThresholdsToUse, useMex, numThreads);
        
        node.x = probeLocationsXGrid(node.whichProbe);
        node.y = probeLocationsYGrid(node.whichProbe);
        node.remainingLabels = sprintf('%s ', labelNames{mexUnique(labels(node.remaining))});
        
        % If the entropy is incredibly large, there was no valid split
        if node.infoGain > 100000.0
            % PB: Is this still a reasonable thing to happen? I think no.
            
            node.labelID = mexUnique(labels(node.remaining));
            node.labelName = labelNames(node.labelID);
            
            if isempty(g_trainingFailures)
                g_trainingFailures = node;
            else
                g_trainingFailures(end+1) = node;
            end
            
            fprintf('Could not split LeafNode for labels = {%s\b} {%s\b} at depth %d\n', sprintf('%s,', node.labelName{:}), sprintf('%d,', node.remaining), node.depth);
            pBar.add(length(node.remaining) / length(labels));
            
            %debugging stuff
            images = probeTree2_probeValuesToImage(probeValues, node.remaining);
            imshows(images);
            title(sprintf('Could not split:\n\n{%s\b}', sprintf('%s\n', node.labelName{:})));
            pause(.01);
            %             keyboard
            
            return;
        end
        
        probesUsed = updatedProbesUsed;
        
        % Recurse left and right from this node
        goLeft = probeValues{node.whichProbe}(node.remaining) < node.grayvalueThreshold;
        leftRemaining = node.remaining(goLeft);
        
        if isempty(leftRemaining) || length(leftRemaining) == length(node.remaining)
            % PB: Is this still a reasonable thing to happen? I think this is a bug now.
            
            keyboard
            
            % That split makes no progress.  Try again without
            % letting us choose this node again on this branch of the
            % tree.  I believe this can happen when the numerically
            % best info gain corresponds to a position where all the
            % remaining labels are above or below the fixed 0.5 threshold.
            % We don't also select the threshold for each node, but the
            % info gain computation doesn't really take that into
            % account.  Is there a better way to fix this?
            
            %             probesUsed(node.whichProbe) = true;
            %             node.depth = node.depth + 1;
            %             node = buildTree(node, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction);
        else
            % Recurse left
            leftChild.remaining = leftRemaining;
            leftChild.depth = node.depth+1;
            node.leftChild = buildTree(leftChild, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance, grayvalueThresholdsToUse);
            
            % Recurse right
            rightRemaining = node.remaining(~goLeft);
            
            % TODO: is this ever possible?
            assert(~isempty(rightRemaining) && length(rightRemaining) < length(node.remaining));
            
            rightChild.remaining = rightRemaining;
            rightChild.depth = node.depth + 1;
            node.rightChild = buildTree(rightChild, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance, grayvalueThresholdsToUse);
        end
    end % end We have unused probe location. So find the best one to split on
end % buildTree()

function [bestEntropy, bestGrayvalueThreshold] = computeInfoGain_innerLoop(curLabels, curProbeValues, grayvalueThresholds)
    bestEntropy = Inf;
    bestGrayvalueThreshold = -1;
    
    for iGrayvalueThreshold = 1:length(grayvalueThresholds)
        labelsLessThan = curLabels(curProbeValues < grayvalueThresholds(iGrayvalueThreshold));
        labelsGreaterThan = curLabels(curProbeValues >= grayvalueThresholds(iGrayvalueThreshold));
        
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
            bestGrayvalueThreshold = grayvalueThresholds(iGrayvalueThreshold);
        end
    end % for iGrayvalueThreshold = 1:length(grayvalueThresholds)
end % computeInfoGain_innerLoop()

function [bestEntropy, bestGrayvalueThreshold, bestProbeIndex] = computeInfoGain_outerLoop(probeValues, probesUsed, unusedProbeIndexes, numProbesUnused, remainingImages, curLabels, maxLabel, grayvalueThresholdsToUse, numWorkItems, useMex, numThreads)
    bestEntropy = Inf;
    bestProbeIndex = -1;
    bestGrayvalueThreshold = -1;
    
    for iUnusedProbe = 1:numProbesUnused
        curProbeIndex = unusedProbeIndexes(iUnusedProbe);
        curProbeValues = probeValues{curProbeIndex}(remainingImages);
        
        if numWorkItems > 10000000 % 10,000,000 takes about 3 seconds
            if mod(iUnusedProbe,100) == 0
                fprintf('%d',iUnusedProbe);
                pause(.001);
            elseif mod(iUnusedProbe,25) == 0
                fprintf('.');
                pause(.001);
            end
        end
        
        if isempty(grayvalueThresholdsToUse)
            uniqueGrayvalues = mexUnique(curProbeValues);
            
            if length(uniqueGrayvalues) <= 1
                continue;
            end
            
            grayvalueThresholds = (int32(uniqueGrayvalues(2:end)) + int32(uniqueGrayvalues(1:(end-1)))) / 2;
            
            unusedGrayvaluesThresholds = find(~probesUsed(iUnusedProbe, :)) - 1;
            
            % Compute the intersection between the grayvalue thresholds and the thresholds not used
            grayvalueThresholdCounts = zeros([256,1], 'int32');
            grayvalueThresholdCounts(grayvalueThresholds + 1) = int32(1);
            grayvalueThresholdCounts(unusedGrayvaluesThresholds + 1) = grayvalueThresholdCounts(unusedGrayvaluesThresholds + 1) + int32(1);
            grayvalueThresholds = uint8(find(grayvalueThresholdCounts == int32(2)) - 1);
            
        else
            grayvalueThresholds = zeros(0,1,'uint8');
            
            for i = 1:length(grayvalueThresholdsToUse)
                if ~probesUsed(iUnusedProbe, grayvalueThresholdsToUse(i)+1)
                    grayvalueThresholds(end+1) = uint8(grayvalueThresholdsToUse(i)); %#ok<AGROW>
                end
            end
        end
        
        if isempty(grayvalueThresholds)
            continue;
        end
        
        if useMex
            [curBestEntropy, curBestGrayvalueThreshold] = mexComputeInfoGain2_innerLoop(curLabels, curProbeValues, grayvalueThresholds, maxLabel, numThreads);
        else
            [curBestEntropy, curBestGrayvalueThreshold] = computeInfoGain_innerLoop(curLabels, curProbeValues, grayvalueThresholds);
        end
        
        %         disp(sprintf('entropy is %f for probe %d and grayvalue %d', curBestEntropy, curProbeIndex, curBestGrayvalueThreshold));
        
        % The extra tiny amount is to make the result more consistent between C and Matlab, and methods with different amounts of precision
        if curBestEntropy < (bestEntropy - 1e-5)
            bestEntropy = curBestEntropy;
            bestProbeIndex = curProbeIndex;
            bestGrayvalueThreshold = curBestGrayvalueThreshold;
        end
    end % for iUnusedProbe = 1:numProbesUnused
end % computeInfoGain_outerLoop()

function [bestEntropy, bestProbeIndex, bestGrayvalueThreshold, probesUsed] = computeInfoGain(labels, probeValues, probesUsed, remainingImages, minGrayvalueDistance, grayvalueThresholdsToUse, useMex, numThreads)
    totalTic = tic();
    
    unusedProbeIndexes = find(~min(probesUsed,[],2));
    
    numProbesUnused = length(unusedProbeIndexes);
    
    curLabels = labels(remainingImages);
    uniqueLabels = mexUnique(curLabels);
    maxLabel = max(uniqueLabels);
    
    numWorkItems = length(remainingImages) * numProbesUnused;
    
    fprintf('Testing %d probes on %d images ', numProbesUnused, length(remainingImages));
    
    [bestEntropy, bestGrayvalueThreshold, bestProbeIndex] = computeInfoGain_outerLoop(probeValues, probesUsed, unusedProbeIndexes, numProbesUnused, remainingImages, curLabels, maxLabel, grayvalueThresholdsToUse, numWorkItems, useMex, numThreads);
    
    grayvaluesToMask = max(1, min(256, (1 + ((bestGrayvalueThreshold-minGrayvalueDistance):(bestGrayvalueThreshold+minGrayvalueDistance)))));
    
    probesUsed(bestProbeIndex, grayvaluesToMask) = true;
    
    fprintf(' Best entropy is %f in %f seconds\n', bestEntropy, toc(totalTic))
    
end % computeInfoGain()
