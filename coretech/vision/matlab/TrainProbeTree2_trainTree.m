% function probeTree = TrainProbeTree2(varargin)
%
% Based off of VisionMarkerTrained.TrainProbeTree, but split into a
% separate function, to prevent too much messing around with the
% VisionMarkerTrained class hierarchy

% Example:

% fiducialClassesWithImages = TrainProbeTree2_loadImages();
% probeTree = TrainProbeTree2_trainTree(fiducialClassesWithImages);

function probeTree = TrainProbeTree2_trainTree(fiducialClassesWithProbes, varargin)
    %% Parameters
    maxDepth = inf;
    
    numGrayvalueSeparations = 100;
    
    pBar = ProgressBar('VisionMarkerTrained ProbeTree', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    
    probeValues = [probeValues{:}];
    gradMagValues = [gradMagValues{:}];
    
    labels = [labels{:}];
    
    % Add negative examples
    fnamesNeg = {'ALLWHITE'; 'ALLBLACK'};
    if ~isempty(negativeImageDir)
        fnamesNeg = [fnamesNeg; getfnames(negativeImageDir, 'images', 'useFullPath', true)];
    end
    
    numNeg = min(length(fnamesNeg), maxNegativeExamples);
    
    pBar.set_message(sprintf(['Interpolating probe locations ' ...
        'from %d negative images'], numNeg));
    pBar.set_increment(1/numNeg);
    pBar.set(0);
    
    probeValuesNeg   = zeros(workingResolution^2, numNeg, 'single');
    gradMagValuesNeg = zeros(workingResolution^2, numNeg, 'single');
    
    for iImg = 1:numNeg
        if strcmp(fnamesNeg{iImg}, 'ALLWHITE')
            imgNeg = ones(workingResolution);
        elseif strcmp(fnamesNeg{iImg}, 'ALLBLACK')
            imgNeg = zeros(workingResolution);
        else
            imgNeg = imreadAlphaHelper(fnamesNeg{iImg});
        end
        
        imgGradMag = single(smoothgradient(imgNeg));
        imgNeg = single(imgNeg);
        
        [nrows,ncols,~] = size(imgNeg);
        imageCoordsX = linspace(0, 1, ncols);
        imageCoordsY = linspace(0, 1, nrows);
        
        probeValuesNeg(:,iImg) = mean(interp2(imageCoordsX, imageCoordsY, imgNeg, ...
            X, Y, 'linear', 1), 2);
        
        gradMagValuesNeg(:,iImg) = mean(interp2(imageCoordsX, imageCoordsY, imgGradMag, ...
            X, Y, 'linear', 0), 2);
        
        pBar.increment();
    end % for each negative example
    
    noInfo = all(probeValuesNeg == 0 | probeValuesNeg == 1, 1);
    if any(noInfo)
        fprintf('Ignoring %d negative patches with no valid info.\n', sum(noInfo));
        numNeg = numNeg - sum(noInfo);
        probeValuesNeg(:,noInfo) = [];
        gradMagValuesNeg(:,noInfo) = [];
        %fnamesNeg(noInfo) = [];
    end
    
    labelNames{end+1} = 'INVALID';
    labels = [labels length(labelNames)*ones(1,numNeg,'uint32')];
    %fnames = [fnames; fnamesNeg];
    probeValues = [probeValues probeValuesNeg];
    gradMagValues = [gradMagValues gradMagValuesNeg];
    
    assert(size(probeValues,2) == length(labels));
    assert(max(labels) == length(labelNames));
    
    
    %% Train the decision tree
    
    pBar.set_message('Building multiclass tree');
    pBar.set(0);
    totalExamplesToClassify = size(probeValues, 2);
    
    % numLabels = length(labelNames);
    % hCounts = zeros(numLabels,1);
    % namedFigure('Leaves per Label'), hold off
    % for iBar = 1:numLabels
    %     hCounts(iBar) = barh(iBar, 0); hold on
    % end
    % set(get(hCounts(1), 'Parent'), 'YTick', 1:numLabels, 'YTickLabel', labelNames);
    
    probeTree = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
    probeTree.labels = labelNames;
    
    % try
    probeTree = buildTree(probeTree, false(workingResolution), labels, labelNames, maxDepth);
    % catch E
    %     switch(E.identifier)
    %         case {'BuildTree:MaxDepth', 'BuildTree:UselessSplit'}
    %             fprintf('Unable to build tree. Likely ambiguities: "%s".\n', ...
    %                 E.message);
    %             %probeTree = [];
    %             return
    %
    %         otherwise
    %             rethrow(E);
    %     end
    % end
    
    if isempty(probeTree)
        error('Training failed!');
    end
    
    if DEBUG_DISPLAY && DrawTrees
        namedFigure('Multiclass DecisionTree'), clf
        fprintf('Drawing multi-class tree...');
        DrawTree(probeTree, 0);
        fprintf('Done.\n');
    end
    
    %[numMain, numVerify] = VisionMarkerTrained.GetNumTreeNodes(); % Not valid until we clear VisionMarkerTrained and force a tree re-load.
    fprintf('Training completed in %.2f seconds (%.1f minutes), plus %.2f seconds (%.2f minutes) for testing on original images.\n', ...
        t_train, t_train/60, t_test, t_test/60);
    
    %% buildTree() Nested Function
    
    function node = buildTree(node, masked, labels, labelNames, maxDepth)
        
        if nargin < 5
            maxDepth = inf;
        end
        
        %if all(labels(node.remaining)==labels(node.remaining(1)))
        counts = hist(labels(node.remaining), 1:length(labelNames));
        [maxCount, maxIndex] = max(counts);
        if maxCount >= leafNodeFraction*length(node.remaining)
            node.labelID = maxIndex;
            node.labelName = labelNames{node.labelID};
            fprintf('LeafNode for label = %d, or "%s"\n', node.labelID, node.labelName);
            
            pBar.add(length(node.remaining)/totalExamplesToClassify);
            %             for iRemain = 1:length(node.remaining)
            %                 h = hCounts(labels(node.remaining(iRemain)));
            %                 set(h, 'YData', get(h, 'YData')+1);
            %             end
            
        elseif node.depth == maxDepth || all(masked(:))
            node.labelID = unique(labels(node.remaining));
            node.labelName = labelNames(node.labelID);
            fprintf('MaxDepth LeafNode for labels = {%s\b}\n', sprintf('%s,', node.labelName{:}));
            
            pBar.add(length(node.remaining)/totalExamplesToClassify);
            %             for iRemain = 1:length(node.remaining)
            %                 h = hCounts(labels(node.remaining(iRemain)));
            %                 set(h, 'YData', get(h, 'YData')+1);
            %             end
            
        else
            unusedProbes = find(~masked);
            
            infoGain = computeInfoGain(labels(node.remaining), length(labelNames), ...
                probeValues(unusedProbes,node.remaining));
            
            %{
            if DEBUG_DISPLAY
                temp = zeros(workingResolution);
                temp(unusedProbes) = infoGain;
                if isempty(findobj(gca, 'Type', 'image'))
                    hold off, imagesc(temp), axis image, hold on
                    colormap(gray)
                else
                    replace_image(temp); hold on
                end
                pause
            end
                %}
                
                % Find all probes with the max information gain and if there
                % are more than one, choose a random one of them
                maxInfoGain = max(infoGain);
                
                %             if maxInfoGain < minInfoGain
                %                 warning('Making too little progress differentiating [%s], giving up on this branch.', ...
                %                     sprintf('%s ', labelNames{labels(node.remaining)}));
                %                 %node = struct('x', -1, 'y', -1, 'whichProbe', -1);
                %                 return
                %             end
                
                whichUnusedProbe = find(infoGain == maxInfoGain);
                if length(whichUnusedProbe)>1
                    % % Randomly select from those with the max score
                    % index = randperm(length(whichUnusedProbe));
                    % whichUnusedProbe = whichUnusedProbe(index(1));
                    
                    % Choose the one with the lowest edge energy since we don't
                    % want to select probes near edges if we can avoid it
                    gradMag = mean(gradMagValues(unusedProbes(whichUnusedProbe),node.remaining),2);
                    minGradMag = min(gradMag);
                    
                    % If there are more than one with the minimum edge energy,
                    % randomly select one of them
                    minGradMagIndex = find(gradMag == minGradMag);
                    if length(minGradMagIndex) > 1
                        index = randperm(length(minGradMagIndex));
                        minGradMagIndex = minGradMagIndex(index(1));
                    end
                    
                    whichUnusedProbe = whichUnusedProbe(minGradMagIndex);
                    
                end
                
                % Choose the probe location with the highest score
                %[~,node.whichProbe] = max(score(:));
                node.whichProbe = unusedProbes(whichUnusedProbe);
                node.x = xgrid(node.whichProbe);%-1)/workingResolution;
                node.y = ygrid(node.whichProbe);%-1)/workingResolution;
                node.infoGain = infoGain(whichUnusedProbe);
                node.remainingLabels = sprintf('%s ', labelNames{unique(labels(node.remaining))});
                
                % Recurse left and right from this node
                goLeft = probeValues(node.whichProbe,node.remaining) < .5;
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
                    masked(node.whichProbe) = true;
                    node.depth = node.depth + 1;
                    node = buildTree(node, masked, labels, labelNames, maxDepth);
                    
                else
                    % Recurse left
                    %usedLeft = masked;
                    %usedLeft(node.whichProbe) = true;
                    
                    leftChild.remaining = leftRemaining;
                    leftChild.depth = node.depth+1;
                    node.leftChild = buildTree(leftChild, masked, labels, labelNames, maxDepth);
                    
                    % Recurse right
                    rightRemaining = node.remaining(~goLeft);
                    assert(~isempty(rightRemaining) && length(rightRemaining) < length(node.remaining));
                    
                    %usedRight = masked;
                    %usedRight(node.whichProbe) = true;
                    
                    rightChild.remaining = rightRemaining;
                    rightChild.depth = node.depth+1;
                    node.rightChild = buildTree(rightChild, masked, labels, labelNames, maxDepth);
                    
                end
        end
        
    end % buildTree()
end % TrainProbes()


function infoGain = computeInfoGain(labels, numLabels, probeValues)
    
    % Use private mex implementation for speed
    infoGain = mexComputeInfoGain(labels, numLabels, probeValues'); % Note the transpose!
    
    if any(isnan(infoGain(:)))
        warning('mexComputeInfoGain returned NaN values!');
        keyboard
    end
    
    return
    
    % Since we are just looking for max info gain, we don't actually need to
    % compute the currentEntropy, since it's the same for all probes.  We just
    % need the probe with the lowest conditional entropy, since that will be
    % the one with the highest infoGain
    %markerProb = max(eps,hist(labels, 1:numLabels));
    %markerProb = markerProb/max(eps,sum(markerProb));
    %currentEntropy = -sum(markerProb.*log2(markerProb));
    currentEntropy = 0;
    
    numProbes = size(probeValues,1);
    
    % Use the (blurred) image values directly as indications of the
    % probability a probe is "on" or "off" instead of thresholding.
    % The hope is that this discourages selecting probes near
    % edges, which will be gray ("uncertain"), instead of those far
    % from any edge in any image, since those will still be closer
    % to black or white even after blurring.
    
    probeIsOff = probeValues;
    probeIsOn  = 1 - probeValues;
    % probeIsOn  = max(0, 1 - 2*probeValues);
    % probeIsOff = max(0, 2*probeValues - 1);
    
    markerProb_on  = zeros(numProbes,numLabels);
    markerProb_off = zeros(numProbes,numLabels);
    
    for i_probe = 1:numProbes
        markerProb_on(i_probe,:)  = max(eps, accumarray(labels(:), probeIsOn(i_probe,:)', [numLabels 1])');
        markerProb_off(i_probe,:) = max(eps, accumarray(labels(:), probeIsOff(i_probe,:)', [numLabels 1])');
    end
    
    markerProb_on  = markerProb_on  ./ (sum(markerProb_on,2)*ones(1,numLabels));
    markerProb_off = markerProb_off ./ (sum(markerProb_off,2)*ones(1,numLabels));
    
    p_on = sum(probeIsOn,2)/size(probeIsOn,2);
    p_off = 1 - p_on;
    
    conditionalEntropy = - (p_on.*sum(markerProb_on.*log2(max(eps,markerProb_on)), 2) + ...
        p_off.*sum(markerProb_off.*log2(max(eps,markerProb_off)), 2));
    
    infoGain = currentEntropy - conditionalEntropy;
    
end % computeInfoGain()
