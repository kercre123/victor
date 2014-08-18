% function probeTree = probeTree2_train(varargin)
%
% Based off of VisionMarkerTrained.TrainProbeTree, but split into a
% separate function, to prevent too much messing around with the
% VisionMarkerTrained class hierarchy

% Example:
% probeTree = probeTree2_train(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid);

function probeTree = probeTree2_train(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, varargin)
    global trainingFailures;
    global nodeId;
    
    nodeId = 1;
    
    t_start = tic();
    
    for iProbe = 1:length(probeValues)
        assert(min(size(labels) == size(probeValues{iProbe})) == 1);
    end
    
    leafNodeFraction = 0.999;
    
    parseVarargin(varargin{:});
    
    % Train the decision tree
    
    probeTree = struct('depth', 0, 'infoGain', 0, 'remaining', int32(1:length(labels)));
    probeTree.labels = labelNames;
    
    probeTree = buildTree(probeTree, false(length(probeValues), 1), labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction);
    
    %     drawTree(probeTree)
    
    if isempty(probeTree)
        error('Training failed!');
    end
    
    t_train = toc(t_start);
    
    numNodes = countNumNodes(probeTree);
    
    disp(sprintf('Training completed in %f seconds. Tree has %d nodes.', t_train, numNodes));
    
    t_test = tic();
    
    [numCorrect, numTotal] = testOnTrainingData(probeTree, probeLocationsXGrid, probeLocationsYGrid, probeValues, labels);
    
    t_test = toc(t_test);
    
    disp(sprintf('Tested tree on training set in %f seconds. Training accuracy: %d/%d = %0.2f%%', t_test, numCorrect, numTotal, 100*numCorrect/numTotal));
    
    %     keyboard
end % probeTree2_train()

function numNodes = countNumNodes(probeTree)
    if isfield(probeTree, 'labelName')
        numNodes = 1;
    else
        numNodes = countNumNodes(probeTree.leftChild) + countNumNodes(probeTree.rightChild);
    end
end % countNumNodes()

function [numCorrect, numTotal] = testOnTrainingData(probeTree, probeLocationsXGrid, probeLocationsYGrid, probeValues, labels)
    numImages = length(probeValues{1});
    numProbes = length(probeLocationsXGrid);
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
        
        [labelName, labelID] = probeTree2_query(probeTree, probeLocationsXGrid, probeLocationsYGrid, curImage, tform);
        
        if labelID == labels(iImage);
            numCorrect = numCorrect + 1;
        end
        if mod(iImage, 100) == 0
            pBar.increment();
        end
    end
end % testOnTrainingData()

function node = buildTree(node, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction)
    global trainingFailures;
    global nodeId;
    
    maxDepth = inf;
    
    %if all(labels(node.remaining)==labels(node.remaining(1)))
    counts = hist(labels(node.remaining), 1:length(labelNames));
    [maxCount, maxIndex] = max(counts);
    
    node.nodeId = nodeId;
    nodeId = nodeId + 1;
    
    if maxCount >= leafNodeFraction*length(node.remaining)
        % Are more than leafNodeFraction percent of the remaining labels the
        % same? If so, we're done.
        
        node.labelID = maxIndex;
        node.labelName = labelNames{node.labelID};
        fprintf('LeafNode for label = %d, or "%s"\n', node.labelID, node.labelName);
    elseif node.depth == maxDepth || all(probesUsed(:))
        % Have we reached the max depth, or have we used all probes? If so,
        % we're done.
        
        node.labelID = unique(labels(node.remaining));
        node.labelName = labelNames(node.labelID);
        fprintf('MaxDepth LeafNode for labels = {%s\b}\n', sprintf('%s,', node.labelName{:}));
    else
        % We have unused probe location. So find the best one to split on
        
        useMex = true;
        
        [node.infoGain, node.whichProbe, node.grayvalueThreshold, probesUsed] = computeInfoGain(labels, probeValues, probesUsed, node.remaining, useMex);
        
        node.x = probeLocationsXGrid(node.whichProbe);
        node.y = probeLocationsYGrid(node.whichProbe);
        node.remainingLabels = sprintf('%s ', labelNames{unique(labels(node.remaining))});
        
        % If the entropy is incredibly large, there was no valid split
        if node.infoGain > 100000.0 
            trainingFailures(end+1) = node;
            disp('Training failed for node %d with labels %s', node.nodeId, node.remainingLabels);
            keyboard
        end
        
        % Recurse left and right from this node
        goLeft = probeValues{node.whichProbe}(node.remaining) < node.grayvalueThreshold;
        leftRemaining = node.remaining(goLeft);
        
        if isempty(leftRemaining) || length(leftRemaining) == length(node.remaining)
            % PB: Is this still a reasonable thing to happen?
            
            % That split makes no progress.  Try again without
            % letting us choose this node again on this branch of the
            % tree.  I believe this can happen when the numerically
            % best info gain corresponds to a position where all the
            % remaining labels are above or below the fixed 0.5 threshold.
            % We don't also select the threshold for each node, but the
            % info gain computation doesn't really take that into
            % account.  Is there a better way to fix this?
            
            keyboard
            
            probesUsed(node.whichProbe) = true;
            node.depth = node.depth + 1;
            node = buildTree(node, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction);
        else
            % Recurse left
            leftChild.remaining = leftRemaining;
            leftChild.depth = node.depth+1;
            node.leftChild = buildTree(leftChild, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction);
            
            % Recurse right
            rightRemaining = node.remaining(~goLeft);
            
            % TODO: is this ever possible?
            assert(~isempty(rightRemaining) && length(rightRemaining) < length(node.remaining));
            
            rightChild.remaining = rightRemaining;
            rightChild.depth = node.depth + 1;
            node.rightChild = buildTree(rightChild, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction);
            
        end
    end % end We have unused probe location. So find the best one to split on
end % buildTree()

function [bestEntropy, bestGrayvalueThreshold] = computeInfoGain_innerLoop(curLabels, curProbeValues, grayvalueThresholds, maxLabel)
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
        
%         allValuesLessThan = zeros(maxLabel, 1);
%         allValuesGreaterThan = zeros(maxLabel, 1);
        
%         allValuesLessThan(valuesLessThan) = countsLessThan;
%         allValuesGreaterThan(valuesGreaterThan) = countsGreaterThan;

        probabilitiesLessThan = countsLessThan / sum(countsLessThan);
        probabilitiesGreaterThan = countsGreaterThan / sum(countsGreaterThan);
        
        entropyLessThan = -sum(probabilitiesLessThan .* log2(max(eps, probabilitiesLessThan)));
        entropyGreaterThan = -sum(probabilitiesGreaterThan .* log2(max(eps, probabilitiesGreaterThan)));
        
        weightedAverageEntropy = ...
            numLessThan    / (numLessThan + numGreaterThan) * entropyLessThan +...
            numGreaterThan / (numLessThan + numGreaterThan) * entropyGreaterThan;
        
        if weightedAverageEntropy < bestEntropy
%             disp(sprintf('%d %f %f', iGrayvalueThreshold, weightedAverageEntropy, bestEntropy));
            bestEntropy = weightedAverageEntropy;            
            bestGrayvalueThreshold = grayvalueThresholds(iGrayvalueThreshold);
        end
    end % for iGrayvalueThreshold = 1:length(grayvalueThresholds)
end % computeInfoGain_innerLoop()

function [bestEntropy, bestProbeIndex, bestGrayvalueThreshold, probesUsed] = computeInfoGain(labels, probeValues, probesUsed, remainingImages, useMex)
    totalTic = tic();
    
    unusedProbeIndexes = find(~probesUsed);
    
    %     numProbesTotal = length(probeValues);
    numProbesUnused = length(unusedProbeIndexes);
    
    bestEntropy = Inf;
    bestProbeIndex = -1;
    bestGrayvalueThreshold = -1;
    
    curLabels = labels(remainingImages);
    uniqueLabels = unique(curLabels);
    maxLabel = max(uniqueLabels);
    
    fprintf('Testing %d probes on %d images ', numProbesUnused, length(remainingImages));
    for iUnusedProbe = 1:numProbesUnused
        curProbeIndex = unusedProbeIndexes(iUnusedProbe);
        curProbeValues = probeValues{curProbeIndex}(remainingImages);

        uniqueGrayvalues = unique(curProbeValues);
        
        if mod(iUnusedProbe,100) == 0
            fprintf('%d',iUnusedProbe);
            pause(.001);
        elseif mod(iUnusedProbe,25) == 0
            fprintf('.');
            pause(.001);
        end
               
        if length(uniqueGrayvalues) == 1
            %                 disp(sprintf('%d/%d skipped in %f seconds', iUnusedProbe, numProbesUnused, toc()));
            %                 pause(.001);
            probesUsed(curProbeIndex) = true;
            continue;
        end

        grayvalueThresholds = (uniqueGrayvalues(2:end) + uniqueGrayvalues(1:(end-1))) / 2;
        
        if useMex
            [curBestEntropy, curBestGrayvalueThreshold] = mexComputeInfoGain2_innerLoop(curLabels, curProbeValues, grayvalueThresholds, maxLabel);
        else
            [curBestEntropy, curBestGrayvalueThreshold] = computeInfoGain_innerLoop(curLabels, curProbeValues, grayvalueThresholds, maxLabel);
        end
        
        if curBestEntropy < bestEntropy
            bestEntropy = curBestEntropy;
            bestProbeIndex = curProbeIndex;
            bestGrayvalueThreshold = curBestGrayvalueThreshold;
        end
        
        %             disp(sprintf('%d/%d in %f seconds', iUnusedProbe, numProbesUnused, toc()));
        %             pause(.001);
    end % for iUnusedProbe = 1:numProbesUnused
    
    fprintf(' Best entropy is %f in %f seconds\n', bestEntropy, toc(totalTic))
    
end % computeInfoGain()
