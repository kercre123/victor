% function probeTree = probeTree2_trainTree(varargin)
%
% Based off of VisionMarkerTrained.TrainProbeTree, but split into a
% separate function, to prevent too much messing around with the
% VisionMarkerTrained class hierarchy

% Example:
% probeTree = probeTree2_trainTree(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid);

function probeTree = probeTree2_trainTree(labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, varargin)
    t_start = tic();
    
    for iProbe = 1:length(probeValues)
        assert(min(size(labels) == size(probeValues{iProbe})) == 1);
    end
    
    leafNodeFraction = 0.99;
    
    parseVarargin(varargin{:});
    
    % Train the decision tree
    
    probeTree = struct('depth', 0, 'infoGain', 0, 'remaining', int32(1:length(labels)));
    probeTree.labels = labelNames;
        
    probeTree = buildTree(probeTree, false(length(probeValues), 1), labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction);
    
%     drawTree(probeTree)
    
%     keyboard
    
    if isempty(probeTree)
        error('Training failed!');
    end
    
    t_train = toc(t_start);
    
    %[numMain, numVerify] = VisionMarkerTrained.GetNumTreeNodes(); % Not valid until we clear VisionMarkerTrained and force a tree re-load.
%     fprintf('Training completed in %.2f seconds (%.1f minutes), plus %.2f seconds (%.2f minutes) for testing on original images.\n', ...
%         t_train, t_train/60, t_test, t_test/60);
    
end % probeTree2_trainTree()

function node = buildTree(node, probesUsed, labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid, leafNodeFraction)
    maxDepth = inf;
    
    %if all(labels(node.remaining)==labels(node.remaining(1)))
    counts = hist(labels(node.remaining), 1:length(labelNames));
    [maxCount, maxIndex] = max(counts);
    
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
        
        [node.infoGain, node.whichProbe, node.grayvalueThreshold, probesUsed] = computeInfoGain(labels, probeValues, probesUsed, node.remaining);
        
        node.x = probeLocationsXGrid(node.whichProbe);
        node.y = probeLocationsYGrid(node.whichProbe);         
        node.remainingLabels = sprintf('%s ', labelNames{unique(labels(node.remaining))});
        
        % Recurse left and right from this node
        goLeft = probeValues{node.whichProbe}(node.remaining) < node.grayvalueThreshold;
        leftRemaining = node.remaining(goLeft);
        
        if isempty(leftRemaining) || length(leftRemaining) == length(node.remaining)
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

function [bestEntropy, bestProbeIndex, bestGrayvalueThreshold, probesUsed] = computeInfoGain(labels, probeValues, probesUsed, remainingImages)
    
    recompute = true; % TODO: remove
    
    if recompute
        unusedProbeIndexes = find(~probesUsed);

        %     numProbesTotal = length(probeValues);
        numProbesUnused = length(unusedProbeIndexes);

        bestEntropy = Inf;
        bestProbeIndex = -1;
        bestGrayvalueThreshold = -1;

        curLabels = labels(remainingImages);
        uniqueLabels = unique(curLabels);
        maxLabel = max(uniqueLabels);

        for iUnusedProbe = 1:numProbesUnused
            tic

            curProbeIndex = unusedProbeIndexes(iUnusedProbe);
            curProbeValues = probeValues{curProbeIndex}(remainingImages);            

            uniqueGrayvalues = unique(curProbeValues);

            if length(uniqueGrayvalues) == 1
%                 disp(sprintf('%d/%d skipped in %f seconds', iUnusedProbe, numProbesUnused, toc()));
%                 pause(.001);
                probesUsed(curProbeIndex) = true;
                continue;
            end

            grayvalueThresholds = (uniqueGrayvalues(2:end) + uniqueGrayvalues(1:(end-1))) / 2;

            for iGrayvalueThreshold = 1:length(grayvalueThresholds)
                labelsLessThan = curLabels(curProbeValues < grayvalueThresholds(iGrayvalueThreshold));
                labelsGreaterThan = curLabels(curProbeValues >= grayvalueThresholds(iGrayvalueThreshold));

                if isempty(labelsLessThan) || isempty(labelsGreaterThan)                    
                    continue
                end

                [valuesLessThan, countsLessThan] = count_unique(labelsLessThan);
                [valuesGreaterThan, countsGreaterThan] = count_unique(labelsGreaterThan);

                numLessThan = sum(countsLessThan);
                numGreaterThan = sum(countsGreaterThan);

                allValuesLessThan = zeros(maxLabel, 1);
                allValuesGreaterThan = zeros(maxLabel, 1);

                allValuesLessThan(valuesLessThan) = countsLessThan;
                allValuesGreaterThan(valuesGreaterThan) = countsGreaterThan;

                probabilitiesLessThan = allValuesLessThan / sum(allValuesLessThan);
                probabilitiesGreaterThan = allValuesGreaterThan / sum(allValuesGreaterThan);

                entropyLessThan = -sum(probabilitiesLessThan .* log2(max(eps, probabilitiesLessThan)));
                entropyGreaterThan = -sum(probabilitiesGreaterThan .* log2(max(eps, probabilitiesGreaterThan)));

                weightedAverageEntropy = ...
                    numLessThan    / (numLessThan + numGreaterThan) * entropyLessThan +...
                    numGreaterThan / (numLessThan + numGreaterThan) * entropyGreaterThan;

                if weightedAverageEntropy < bestEntropy
                    bestEntropy = weightedAverageEntropy;
                    bestProbeIndex = curProbeIndex;
                    bestGrayvalueThreshold = grayvalueThresholds(iGrayvalueThreshold);
                end
            end % for iGrayvalueThreshold = 1:length(grayvalueThresholds)
%             disp(sprintf('%d/%d in %f seconds', iUnusedProbe, numProbesUnused, toc()));
%             pause(.001);
        end % for iUnusedProbe = 1:numProbesUnused
        
        save tmp.mat *
    else    
        load tmp.mat bestEntropy bestProbeIndex bestGrayvalueThreshold probesUsed
    end
end % computeInfoGain()
