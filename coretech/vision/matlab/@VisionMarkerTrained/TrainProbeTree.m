function probeTree = TrainProbeTree(varargin)

% TODO: Use parameters from a derived class below, instead of VisionMarkerTrained.*

%% Parameters
baggingIterations = 1;
trainingState = [];
preThreshold = true;
%maxSamples = 100;
%minInfoGain = 0;
redBlackVerifyDepth = 8;
redBlackSampleFraction = 0.75;
maxDepth = inf;
addInversions = true;
saveTree = true;
baggingSampleFraction = []; % set to 0 to automatically sample sqrt(N) points
probeSampleMethod = 'gradMag'; % 'gradMag', 'alternative', 'none'
probeSampleFraction = 1;
maxInfoGainFraction = [];
leafNodeFraction = 0.9; % fraction of remaining examples that must have same label to consider node a leaf
testTree = true;
weights = [];
ignoreLabelNamePattern = {};

% Now using unpadded images to train
%imageCoords = [-VisionMarkerTrained.FiducialPaddingFraction ...
%    1+VisionMarkerTrained.FiducialPaddingFraction];

DEBUG_DISPLAY = false;
DrawTrees = false;

parseVarargin(varargin{:});

if ~iscell(ignoreLabelNamePattern)
  ignoreLabelNamePattern = {ignoreLabelNamePattern};
end

if baggingIterations > 1
  probeTree = cell(1, baggingIterations);
  
  pBar = ProgressBar('VisionMarkerTrained ProbeTree Ensemble', 'CancelButton', true);

  pBarCleanup = onCleanup(@()delete(pBar));
  
  pBar.set_message('Building bagged probe tree ensemble');
  pBar.set(0);
  pBar.set_increment(1/baggingIterations);
  pBar.showTimingInfo = true;

  for iBag = 1:baggingIterations
    fprintf('\n\n\n=== Training Bag %d of %d ===\n\n', iBag, baggingIterations);
    probeTree{iBag} = VisionMarkerTrained.TrainProbeTree(varargin{:}, 'baggingIterations', 1, 'saveTree', false, 'testTree', false);
    pBar.increment();
    if pBar.cancelled
      fprintf('User cancelled bagging.\n');
      saveTree = false;
      break;
    end
  end
  
  if saveTree
    SaveTreeHelper();
  end
  return
end


t_start = tic;

if isempty(trainingState)
    % If none provided, load last saved training state from file
    trainingState = fullfile(fileparts(mfilename('fullpath')), 'trainingState.mat');
end

if ischar(trainingState)
    trainingState = load(trainingState);
end

assert(isstruct(trainingState), 'Expecting trainingState struct.');
try
    probeValues   = trainingState.probeValues;
    if ~strcmp(probeSampleMethod, 'none') || ~isempty(maxInfoGainFraction)
      gradMagValues = trainingState.gradMagValues;
    end
    fnames        = trainingState.fnames;
    labels        = trainingState.labels;
    labelNames    = trainingState.labelNames;
    numImages     = trainingState.numImages;
    xgrid         = trainingState.xgrid;
    ygrid         = trainingState.ygrid;
    %resolution    = trainingState.resolution;
catch E
    error('Failed to get all required fields from training state: %s.', E.message);
end

assert(isa(probeValues, 'uint8'), 'Assuming probeValues are UINT8 now.');

probePattern = VisionMarkerTrained.ProbePattern;

if ~isempty(ignoreLabelNamePattern)
  
  toKeep = true(1,size(probeValues,2));
  for iPattern = 1:length(ignoreLabelNamePattern)
    index = find(~cellfun(@isempty, regexp(trainingState.labelNames, ignoreLabelNamePattern{iPattern})));
    
    for i = 1:length(index)
      toKeep = toKeep & labels ~= index(i);
    end
  end

  probeValues = probeValues(:,toKeep);
  if exist('gradMagValues', 'var')
    gradMagValues = gradMagValues(:,toKeep);
  end
  labels = labels(toKeep);
  
end

if isempty(weights)
    weights = ones(1,size(probeValues,2)) / size(probeValues,2);
end

if ~isempty(baggingSampleFraction)
  
  N = size(probeValues,2);
  
    if baggingSampleFraction == 0 %#ok<BDSCI>
      baggingSampleFraction = sqrt(N)/N;
    end
    
    t_bagSample = tic;
    fprintf('Sampling %.1f%% of training examples with replacement...', baggingSampleFraction*100);
    
    sampleIndex = randi(N, 1, ceil(baggingSampleFraction*N));
    
    probeValues = probeValues(:,sampleIndex);
    if exist('gradMagValues', 'var')
      gradMagValues = gradMagValues(:,sampleIndex);
    end
    labels = labels(sampleIndex);
    weights = weights(sampleIndex);
    numImages = length(sampleIndex);
    
    fprintf('Done. (%.1f seconds)\n', toc(t_bagSample));
end

if probeSampleFraction < 1
    t_probeSample = tic;
    numProbes = size(probeValues,1);
    if probeSampleFraction == 0
      probeSampleFraction = sqrt(numProbes)/numProbes;
    end
    
    fprintf('Sampling %.1f%% of probes...', probeSampleFraction*100);
    sampleMask = rand(numProbes,1) < probeSampleFraction;
    probeValues   = probeValues(sampleMask,:);
    if exist('gradMagValues', 'var')
      gradMagValues = gradMagValues(sampleMask,:);
    end
    xgrid = xgrid(sampleMask,:);
    ygrid = ygrid(sampleMask,:);
    %resolution = resolution(sampleMask,:);
    fprintf('Done. (%.1f seconds)\n', toc(t_probeSample));
end

% 
% % TODO: move these parameters to derived classes
% sqFrac = VisionMarkerTrained.SquareWidthFraction;
% padFrac = VisionMarkerTrained.FiducialPaddingFraction;
% 
% probeTree = TrainProbes('maxDepth', 50, 'addRotations', false, ...
%     'markerImageDir', VisionMarkerTrained.TrainingImageDir, ...
%     'imageCoords', [-padFrac 1+padFrac], ...
%     'probeRegion', [sqFrac+padFrac 1-sqFrac-padFrac], ...
%     'numPerturbations', 100, 'perturbSigma', 1, 'workingResolution', 30, 'DEBUG_DISPLAY', true); %#ok<NASGU>

if ~saveTree && nargout==0
    answer = questdlg('SaveTree==false and no output requested for resulting tree. Continue?', 'Continue?', 'Yes', 'No', 'No');
    if strcmp(answer, 'No')
        fprintf('Aborted.\n');
        return
    end
end


%% Train the decision tree

%resHist = zeros(1, length(probePattern));

pBar = ProgressBar('VisionMarkerTrained ProbeTree', 'CancelButton', true);
pBar.showTimingInfo = true;
pBarCleanup = onCleanup(@()delete(pBar));

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

if preThreshold
  probeValues = im2uint8(probeValues > 128);
end

% try
    probeTree = buildTree(probeTree, false(size(probeValues,1),1), labels, labelNames, maxDepth);
    
%     subplot 131
%     bar(resHist)
%     title('Resolution Histogram after Main Tree');
    
    
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

%% Train Red/Black Verify trees

if redBlackVerifyDepth > 0
    pBar.set_message('Training red/black verification trees');
    pBar.set(0);
    totalExamplesToClassify = 2*totalExamplesToClassify;
    
    % redMask = false(workingResolution);
    % %redMask(1:2:end,1:2:end) = true;
    % %redMask(2:2:end,2:2:end) = true;
    % redMask(1:round(workingResolution/2),:) = true;
    redMask = rand(size(probeValues,1),1) > redBlackSampleFraction;
    %resHist = zeros(1,length(probePattern));
    
    probeTree.verifyTreeRed = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
    probeTree.verifyTreeRed.labels = labelNames;
    probeTree.verifyTreeRed = buildTree(probeTree.verifyTreeRed, redMask, labels, labelNames, redBlackVerifyDepth);
    
    %subplot 132
    %bar(resHist)
    %title('Resolution Histogram after Red Tree');
    
    blackMask = rand(size(probeValues,1),1) > redBlackSampleFraction;
    %resHist = zeros(1,length(probePattern));
    
    probeTree.verifyTreeBlack = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
    probeTree.verifyTreeBlack.labels = labelNames;
    probeTree.verifyTreeBlack = buildTree(probeTree.verifyTreeBlack, blackMask, labels, labelNames, redBlackVerifyDepth);
    
    %subplot 133
    %bar(resHist)
    %title('Resolution Histogram after Black Tree');
    
end

t_train = toc(t_start);
t_start = tic;

%% Train one-vs-all trees

% assert(length(labelNames) == numLabels);
% 
% pBar.set_message('Building one-vs-all verification trees');
% pBar.set_increment(1/numLabels);
% pBar.set(0);
% 
% verifier = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
% for i_label = 1:numLabels
%     
%     fprintf('\n\nTraining one-vs-all tree for "%s"\n', labelNames{i_label});
%     
%     currentLabels = double(labels == i_label) + 1;
%     currentLabelNames = {'UNKNOWN', labelNames{i_label}};
%     
%     probeTree.verifiers(i_label) = buildTree(verifier, ...
%         false(workingResolution), currentLabels, currentLabelNames);
%     
%     if DEBUG_DISPLAY && DrawTrees
%         namedFigure(sprintf('%s DecisionTree', labelNames{i_label})), clf
%         fprintf('Drawing %s tree...', labelNames{i_label});
%         DrawTree(probeTree.verifiers(i_label), 0);
%         fprintf('Done.\n');
%     end
% 
%     pBar.increment();
% end




%% Test on Training Data
% 
% fprintf('Testing on %d training images...', numImages);
% pBar.set_message(sprintf('Testing on %d training images...', numImages));
% pBar.set_increment(1/numImages);
% pBar.set(0);
% 
% if DEBUG_DISPLAY
%     namedFigure('Sampled Test Results'); clf
%     colormap(gray)
%     maxDisplay = 48;
% else
%     maxDisplay = 0; %#ok<UNRCH>
% end
% correct = false(1,numImages);
% verified = false(1,numImages);
% randIndex = row(randperm(numImages));
% for i_rand = 1:numImages
%     iImg = randIndex(i_rand);
%    
%     testImg = reshape(probeValues(:,iImg), workingResolution*[1 1]);
%     
%     Corners = VisionMarkerTrained.GetFiducialCorners(size(testImg,1), false);
%     %Corners = VisionMarkerTrained.GetMarkerCorners(size(testImg,1), false);
%     if i_rand <= maxDisplay
%         subplot(6, maxDisplay/6, i_rand), hold off
%         imagesc(testImg), axis image, hold on
%         plot(Corners(:,1), Corners(:,2), 'y+');
%         doDisplay = true;
%     else 
%         doDisplay = false;
%     end
%     
%     tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
%     
%     [result, labelID] = TestTree(probeTree, testImg, tform, 0.5, probePattern, doDisplay);
%     
%     assert(labelID>0 && labelID<=length(probeTree.verifiers));
%     
%     [verificationResult, verifiedID] = TestTree(probeTree.verifiers(labelID), testImg, tform, 0.5, probePattern);
%     verified(iImg) = verifiedID == 2;
%     if verified(iImg)
%         assert(strcmp(verificationResult, result));
%     end
%     
%     if ischar(result)
%         correct(iImg) = strcmp(result, labelNames{labels(iImg)});
%         if ~verified(iImg)
%             color = 'b';
%         else
%             if ~correct(iImg)
%                 color = 'r';
%             else
%                 color = 'g';
%             end
%         end
%         
%         if i_rand <= maxDisplay
%             set(gca, 'XColor', color, 'YColor', color);
%             title(result)
%         end
%                 
%     else
%         warning('Reached node with multiple remaining labels: ')
%         for j = 1:length(result)
%             fprintf('%s, ', labelNames{labels(result(j))});
%         end
%         fprintf('\b\b\n');
%     end
%     
%     pBar.increment();
%     if pBar.cancelled
%        fprintf('User cancelled testing on training images.\n');
%        break; 
%     end
% end
% fprintf(' Got %d of %d training images right (%.1f%%)\n', ...
%     sum(correct), length(correct), sum(correct)/length(correct)*100);
% if sum(correct) ~= length(correct)
%     warning('We should have ZERO training error!');
% end
% 
% fprintf(' %d of %d training images verified.\n', ...
%     sum(verified), length(verified));
% if sum(verified) ~= length(verified)
%     warning('All training images should verify!');
% end

%% Test on original images
if testTree
    if addInversions
        invert = [false(size(fnames(:))); true(size(fnames(:)))];
        fnames = [fnames(:); fnames(:)];
    else
        invert = false(size(fnames)); %#ok<UNRCH>
    end
    
    fprintf('Testing on %d original images...', length(fnames));
    pBar.set_message(sprintf('Testing on %d original images...', length(fnames)));
    pBar.set_increment(1/length(fnames));
    pBar.set(0);
    
    correct = false(1,length(fnames));
    verified = false(1,length(fnames));
    
    if DEBUG_DISPLAY
        namedFigure('Original Image Errors'), clf %#ok<UNRCH>
        numDisplayRows = floor(sqrt(length(fnames)));
        numDisplayCols = ceil(length(fnames)/numDisplayRows);
    end
    for iImg = 1:length(fnames)
        if any(strcmp(fnames{iImg}, {'ALLWHITE', 'ALLBLACK'}))
            correct(iImg)  = true;
            verified(iImg) = true;
            continue;
        end
        
        [testImg,~,alpha] = imread(fnames{iImg});
        testImg = mean(im2double(testImg),3);
        testImg(alpha < .5) = 1;
        
        if invert(iImg)
            testImg = 1 - testImg;
        end
        
        % radius = probeRadius/workingResolution * size(testImg,1);
        
        %testImg = separable_filter(testImg, gaussian_kernel(radius/2), [], 'replicate');
        Corners = VisionMarkerTrained.GetFiducialCorners(size(testImg,1), false);
        %Corners = VisionMarkerTrained.GetMarkerCorners(size(testImg,1), false);
        tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
        
        [result, labelID] = TestTree(probeTree, testImg, tform, 0.5, probePattern);
        
        if length(labelID) > 1
          %warning('Multiple labels in multiclass tree.');
          
          labelID = mode(labelID);
          result = labelNames{labelID};
        end
        
        if redBlackVerifyDepth > 0
            [redResult, redLabelID] = TestTree(probeTree.verifyTreeRed, testImg, tform, 0.5, probePattern);
            [blackResult, blackLabelID] = TestTree(probeTree.verifyTreeBlack, testImg, tform, 0.5, probePattern);
            
            if any(labelID == redLabelID) && any(labelID == blackLabelID)
                assert(any(strcmp(result, redResult)) && ...
                    any(strcmp(result, blackResult)));
                verified(iImg) = true;
            end
        end
        
        
        %     [verificationResult, verifiedID] = TestTree(probeTree.verifiers(labelID), testImg, tform, 0.5, probePattern);
        %     verified(iImg) = verifiedID == 2;
        %     if verified(iImg)
        %         assert(strcmp(verificationResult, result));
        %     end
        
        if ischar(result)
            correct(iImg) = strcmp(result, labelNames{labelID});
            
            if ~correct(iImg)
                figure, imagesc(testImg), axis image, title(result)
            end
            
            if DEBUG_DISPLAY % && ~correct(i)
                h_axes = subplot(numDisplayRows,numDisplayCols,iImg); %#ok<UNRCH>
                imagesc(testImg, 'Parent', h_axes); hold on
                axis(h_axes, 'image');
                TestTree(probeTree, testImg, tform, 0.5, probePattern, true);
                plot(Corners(:,1), Corners(:,2), 'y+');
                title(h_axes, result);
                if ~verified(iImg)
                    color = 'b';
                else
                    if correct(iImg)
                        color = 'g';
                    else
                        color = 'r';
                    end
                end
                set(h_axes, 'XColor', color, 'YColor', color);
            end
            
        else
            fprintf('Reached node with multiple remaining labels: ')
            fprintf('%s, ', result{:});
            fprintf('\b\b\n');
        end
        
        pBar.increment();
        if pBar.cancelled
            fprintf('User cancelled testing on original images.\n');
            break;
        end
    end
    fprintf('Got %d of %d original images right (%.1f%%)\n', ...
        sum(correct), length(correct), sum(correct)/length(correct)*100);
    if sum(correct) ~= length(correct)
        warning('We should have ZERO errors on original images!');
    end
    fprintf(' %d of %d original images verified.\n', ...
        sum(verified), length(verified));
    if sum(verified) ~= length(verified)
        warning('All original images should verify!');
    end
    
end % if testTree

t_test = toc(t_start);

%[numMain, numVerify] = VisionMarkerTrained.GetNumTreeNodes(); % Not valid until we clear VisionMarkerTrained and force a tree re-load.
fprintf('Training completed in %.2f seconds (%.1f minutes), plus %.2f seconds (%.2f minutes) for testing on original images.\n', ...
    t_train, t_train/60, t_test, t_test/60);
% Used %d main tree nodes + %d verification nodes.\n', ...
%    numMain, numVerify);


SaveTreeHelper();

%% Save Tree
  function SaveTreeHelper
    
    if saveTree
      savePath = VisionMarkerTrained.SavedTreePath;
      saveFile = fullfile(savePath, 'probeTree.mat');
      
      % Preserve previous probeTree, for archiving
      archiveFile = '';
      if exist(saveFile, 'file')
        archiveFile = fullfile(savePath, sprintf('probeTree_%s.mat',datestr(now, 30)));
        movefile(saveFile, archiveFile);
      end
      
      % Save the new tree
      save(saveFile, 'probeTree');
      
      fprintf('Probe tree re-trained and saved to %s.\n', saveFile);
      if ~isempty(archiveFile)
        fprintf('Previous tree archived to %s.\n', archiveFile);
      end
      fprintf('You may need to clear any existing VisionMarkers.\n\n');
      clear(mfilename('class')); % Force a re-load of the saved tree from disk
    end % if saveTree
    
  end % SaveTreeHelper()


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
            node.labelID = labels(node.remaining);
            node.labelName = labelNames(unique(node.labelID));
            fprintf('MaxDepth LeafNode for labels = {%s\b}\n', sprintf('%s,', node.labelName{:}));
            
            pBar.add(length(node.remaining)/totalExamplesToClassify);
%             for iRemain = 1:length(node.remaining)
%                 h = hCounts(labels(node.remaining(iRemain)));
%                 set(h, 'YData', get(h, 'YData')+1);
%             end
    
        else            
            unusedProbes = find(~masked);
            
            if ~strcmp(probeSampleMethod, 'none')
              switch(probeSampleMethod)
                
                case 'gradMag'
                  % Sample those probes which have lower gradient magnitude
                  pSample = mean(gradMagValues(unusedProbes,node.remaining),2);
                  pSample = max(pSample) - pSample;
                  pSample = pSample / sum(pSample);
                  sampleIndex = discretesample(pSample, ceil(sqrt(length(unusedProbes))));
                  %sampleIndex = mexRandP(pSample, sqrt(length(unusedProbes)));
                  %temp = 0; temp2 = 0; x = 0; y = 0; % for use during debugging since this is a static workspace
                  
                case 'alternative'
                  % Alternative sampling method
                  pSample = mean(gradMagValues(unusedProbes,node.remaining),2);
                  pSample = max(pSample) - pSample;
                  pSample = pSample / max(pSample);
                  sampleIndex = find(pSample > .5*rand(size(pSample))+.5);
                  temp = 0; temp2 = 0; x = 0; y = 0; % for use during debugging since this is a static workspace
                  
                  %             temp = zeros(32);
                  %             temp(unusedProbes) = pSample;
                  %             [y,x] = ind2sub([32 32], unusedProbes(sampleIndex));
                  %             hold off, imagesc(temp), axis image, hold on, plot(x + .1*randn(size(x)),y+.1*randn(size(y)),'ro')
                  %             pause
                  
                  unusedProbes = unusedProbes(sampleIndex);
                  
                otherwise
                  error('Unrecognized probeSampleMethod "%s"', probeSampleMethod);
              end % switch(probeSampleMethod)
            end % if ~strcmp(probeSampleMethod, 'none')
              
            infoGain = computeInfoGain(labels(node.remaining), length(labelNames), ...
              probeValues(unusedProbes,node.remaining), weights(node.remaining));
            
            % % Testing categorical info gain computation
            % infoGain2 = computeInfoGain_nonBinary(labels(node.remaining), length(labelNames), ...
            % double(probeValues(unusedProbes,node.remaining) > .5)+1, 2, weights(node.remaining));
                        
            %{
            meanInfoGain = zeros(1,length(VisionMarkerTrained.ProbeParameters.GridSize));
            %startIndex = 1;
            for iRes = 1:length(VisionMarkerTrained.ProbeParameters.GridSize)
                %res = VisionMarkerTrained.ProbeParameters.GridSize(iRes);
                meanInfoGain(iRes) = mean(infoGain(resolution(unusedProbes) == iRes));
                %subplot(1,length(VisionMarkerTrained.ProbeParameters.GridSize),iRes)
                %imagesc(meanInfoGain(1)/meanInfoGain(iRes)*reshape(infoGain(startIndex:(startIndex+res^2-1)), res*[1 1])), axis image, colorbar
                %startIndex = startIndex + res^2;
            end
            
            infoGainScale = meanInfoGain / meanInfoGain(1);
            infoGain = infoGain ./ column(infoGainScale(resolution(unusedProbes))); % note we divide b/c info gains are negative and we want to scale up, which is towards zero
            %infoGainShift = meanInfoGain - meanInfoGain(1);
            %infoGain = infoGain - column(infoGainShift(resolution(unusedProbes)));
            %}
            
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
            
            % Randomly choose from the top X fraction of the info gain
            % (to avoid greedily always picking the max -- i.e. take a step
            %  in the direction of random trees)
            if ~isempty(maxInfoGainFraction)
                [~,sortIndex] = sort(infoGain(:), 'descend');
                
                whichUnusedProbe = sortIndex(randi(ceil(maxInfoGainFraction*length(sortIndex)), 1));
                
            else
            
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
            end
            
            % Choose the probe location with the highest score
            %[~,node.whichProbe] = max(score(:));
            node.whichProbe = unusedProbes(whichUnusedProbe);
            node.x = xgrid(node.whichProbe);%-1)/workingResolution;
            node.y = ygrid(node.whichProbe);%-1)/workingResolution;
            %node.resolution = resolution(node.whichProbe);
            node.infoGain = infoGain(whichUnusedProbe);
            node.remainingLabels = sprintf('%s ', labelNames{unique(labels(node.remaining))});
            
            %resHist(node.resolution) = resHist(node.resolution) + 1;
            
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


    




end % TrainProbeTree()


function infoGain = computeInfoGain(labels, numLabels, probeValues, weights)

% Use private mex implementation for speed
infoGain = mexComputeInfoGain(labels, numLabels, probeValues', single(weights)); % Note the transpose!

if any(isnan(infoGain(:)))
    warning('mexComputeInfoGain returned NaN values!');
    keyboard
end

return
                        
% Since we are just looking for max info gain, we don't actually need to
% compute the currentEntropy, since it's the same for all probes.  We just
% need the probe with the lowest conditional entropy, since that will be
% the one with the highest infoGain
% %markerProb = max(eps,hist(labels, 1:numLabels));
% markerProb = max(eps, accumarray(labels(:), weights(:), [numLabels 1]));
% markerProb = markerProb/max(eps,sum(markerProb));
% currentEntropy = -sum(markerProb.*log2(markerProb));
currentEntropy = 0;

numProbes = size(probeValues,1);

% Use the (blurred) image values directly as indications of the
% probability a probe is "on" or "off" instead of thresholding.
% The hope is that this discourages selecting probes near
% edges, which will be gray ("uncertain"), instead of those far
% from any edge in any image, since those will still be closer
% to black or white even after blurring.

W = weights(ones(numProbes,1),:);
probeIsOff = W.*double(probeValues > 0.5);
probeIsOn  = W.*double(~probeIsOff);
% probeIsOn  = max(0, 1 - 2*probeValues);
% probeIsOff = max(0, 2*probeValues - 1);

markerProb_on  = zeros(numProbes,numLabels);
markerProb_off = zeros(numProbes,numLabels);

for i_probe = 1:numProbes
    markerProb_on(i_probe,:)  = max(eps, accumarray(labels(:), probeIsOn(i_probe,:)',  [numLabels 1])');
    markerProb_off(i_probe,:) = max(eps, accumarray(labels(:), probeIsOff(i_probe,:)', [numLabels 1])');
end

markerProb_on  = markerProb_on  ./ (sum(markerProb_on,2)*ones(1,numLabels));
markerProb_off = markerProb_off ./ (sum(markerProb_off,2)*ones(1,numLabels));

%p_on = sum(probeIsOn,2)/size(probeIsOn,2);
%p_off = 1 - p_on;
p_off = sum(W.*probeValues,2) / sum(weights);
p_on  = 1 - p_off;

conditionalEntropy = - (p_on.*sum(markerProb_on.*log2(max(eps,markerProb_on)), 2) + ...
    p_off.*sum(markerProb_off.*log2(max(eps,markerProb_off)), 2));

infoGain = currentEntropy - conditionalEntropy;

end % computeInfoGain()



function infoGain = computeInfoGain_nonBinary(labels, numLabels, probeValues, numValues, weights)
                        
% Since we are just looking for max info gain, we don't actually need to
% compute the currentEntropy, since it's the same for all probes.  We just
% need the probe with the lowest conditional entropy, since that will be
% the one with the highest infoGain
% %markerProb = max(eps,hist(labels, 1:numLabels));
% markerProb = max(eps, accumarray(labels(:), weights(:), [numLabels 1]));
% markerProb = markerProb/max(eps,sum(markerProb));
% currentEntropy = -sum(markerProb.*log2(markerProb));
currentEntropy = 0;

[numProbes, numExamples] = size(probeValues);

% Use the (blurred) image values directly as indications of the
% probability a probe is "on" or "off" instead of thresholding.
% The hope is that this discourages selecting probes near
% edges, which will be gray ("uncertain"), instead of those far
% from any edge in any image, since those will still be closer
% to black or white even after blurring.

%W = weights(ones(numProbes,1),:);

% probeIsOn  = max(0, 1 - 2*probeValues);
% probeIsOff = max(0, 2*probeValues - 1);

markerProb  = zeros(numProbes,numLabels,numValues);

% Probability of each value at each probe
pValue = zeros(numProbes,numValues);
for i_value = 1:numValues
  pValue(:,i_value) = sum(probeValues==i_value,2);
end
pValue = max(eps, pValue/numExamples);

% Conditional probability of values at each probe, given each label
for i_probe = 1:numProbes
  for i_value = 1:numValues
    markerProb(i_probe,:,i_value) = max(eps, accumarray(labels(:), (probeValues(i_probe,:)==i_value)', [numLabels 1])');
  end
end

conditionalEntropy = zeros(numProbes,1);
for i_value = 1:numValues
  markerProb(:,:,i_value) = markerProb(:,:,i_value) ./ (sum(markerProb(:,:,i_value),2)*ones(1,numLabels));

  conditionalEntropy = conditionalEntropy - pValue(:,i_value).*sum(markerProb(:,:,i_value).*log2(max(eps,markerProb(:,:,i_value))),2);
end
  
infoGain = currentEntropy - conditionalEntropy;

end % computeInfoGain()



