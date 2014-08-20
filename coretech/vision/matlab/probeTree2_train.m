% function probeTree = probeTree2_train()
%
% Based off of VisionMarkerTrained.TrainProbeTree, but split into a
% separate function, to prevent too much messing around with the
% VisionMarkerTrained class hierarchy

% Example:
% [probeTree, trainingFailures] = probeTree2_train(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid);

function [probeTree, trainingFailures] = probeTree2_train(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, varargin)
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
    
    leafNodeFraction = 0.999; % What percent of the labels must be the same for it to be considered a leaf node?
    leafNodeNumItems = 1; % If a node has less-than-or-equal-to this number of items, it is a leaf no matter what
    minGrayvalueDistance = 20; % How close can the grayvalue of two probes in the same location be? Set to 1000 to disallow probes in the same location.
    
    parseVarargin(varargin{:});
    
    % Train the decision tree
    
    probeTree = struct('depth', 0, 'infoGain', 0, 'remaining', int32(1:length(labels)));
    probeTree.labels = labelNames;
    
    probeTree = buildTree(probeTree, false(length(probeValues), 256), labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance);
    
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
    
    trainingFailures = g_trainingFailures;
    
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
            pBar.set_message(sprintf('Testing %d inputs. Current accuracy %d/%d=%f', numImages, numCorrect, iImage, numCorrect/iImage));
            pBar.increment();
        end
    end
end % testOnTrainingData()

function node = buildTree(node, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance)
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
        
        [node.infoGain, node.whichProbe, node.grayvalueThreshold, updatedProbesUsed] = computeInfoGain(labels, probeValues, probesUsed, node.remaining, minGrayvalueDistance, useMex, numThreads);
        
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
            keyboard
            
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
            node.leftChild = buildTree(leftChild, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance);
            
            % Recurse right
            rightRemaining = node.remaining(~goLeft);
            
            % TODO: is this ever possible?
            assert(~isempty(rightRemaining) && length(rightRemaining) < length(node.remaining));
            
            rightChild.remaining = rightRemaining;
            rightChild.depth = node.depth + 1;
            node.rightChild = buildTree(rightChild, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction, leafNodeNumItems, minGrayvalueDistance);
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

function [bestEntropy, bestGrayvalueThreshold, bestProbeIndex] = computeInfoGain_outerLoop(probeValues, probesUsed, unusedProbeIndexes, numProbesUnused, remainingImages, curLabels, maxLabel, numWorkItems, useMex, numThreads)  
    bestEntropy = Inf;
    bestProbeIndex = -1;
    bestGrayvalueThreshold = -1;
    
    for iUnusedProbe = 1:numProbesUnused
        curProbeIndex = unusedProbeIndexes(iUnusedProbe);
        curProbeValues = probeValues{curProbeIndex}(remainingImages);
        
        uniqueGrayvalues = mexUnique(curProbeValues);
                
        if numWorkItems > 10000000 % 10,000,000 takes about 3 seconds
            if mod(iUnusedProbe,100) == 0
                fprintf('%d',iUnusedProbe);
                pause(.001);
            elseif mod(iUnusedProbe,25) == 0
                fprintf('.');
                pause(.001);
            end
        end
        
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
        
        if isempty(grayvalueThresholds)
            continue;
        end
        
        if useMex
            [curBestEntropy, curBestGrayvalueThreshold] = mexComputeInfoGain2_innerLoop(curLabels, curProbeValues, grayvalueThresholds, maxLabel, numThreads);
        else
            [curBestEntropy, curBestGrayvalueThreshold] = computeInfoGain_innerLoop(curLabels, curProbeValues, grayvalueThresholds);
        end
        
        if curBestEntropy < bestEntropy
            bestEntropy = curBestEntropy;
            bestProbeIndex = curProbeIndex;
            bestGrayvalueThreshold = curBestGrayvalueThreshold;
        end
    end % for iUnusedProbe = 1:numProbesUnused
end % computeInfoGain_outerLoop()

function [bestEntropy, bestProbeIndex, bestGrayvalueThreshold, probesUsed] = computeInfoGain(labels, probeValues, probesUsed, remainingImages, minGrayvalueDistance, useMex, numThreads)
    totalTic = tic();
    
    unusedProbeIndexes = find(~min(probesUsed,[],2));
    
    numProbesUnused = length(unusedProbeIndexes);
    
    curLabels = labels(remainingImages);
    uniqueLabels = mexUnique(curLabels);
    maxLabel = max(uniqueLabels);
    
    numWorkItems = length(remainingImages) * numProbesUnused;
    
    fprintf('Testing %d probes on %d images ', numProbesUnused, length(remainingImages));
    
    [bestEntropy, bestGrayvalueThreshold, bestProbeIndex] = computeInfoGain_outerLoop(probeValues, probesUsed, unusedProbeIndexes, numProbesUnused, remainingImages, curLabels, maxLabel, numWorkItems, useMex, numThreads);
        
    grayvaluesToMask = max(1, min(256, (1 + ((bestGrayvalueThreshold-minGrayvalueDistance):(bestGrayvalueThreshold+minGrayvalueDistance)))));
    
    probesUsed(bestProbeIndex, grayvaluesToMask) = true;
    
    fprintf(' Best entropy is %f in %f seconds\n', bestEntropy, toc(totalTic))
    
end % computeInfoGain()
