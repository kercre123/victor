function probeTree = TrainProbeTree(varargin)

% TODO: Use parameters from a derived class below, instead of VisionMarkerTrained.*

%% Parameters
loadSavedProbeValues = false;
markerImageDir = VisionMarkerTrained.TrainingImageDir;
negativeImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/negativeExamplePatches';
maxNegativeExamples = inf;
workingResolution = VisionMarkerTrained.ProbeParameters.GridSize;
%maxSamples = 100;
%minInfoGain = 0;
redBlackVerifyDepth = 8;
maxDepth = inf;
%addRotations = false;
addInversions = true;
numPerturbations = 100;
blurSigmas = [0 .005 .01]; % as a fraction of the image diagonal
perturbSigma = 1;
saveTree = true;
leafNodeFraction = 0.9; % fraction of remaining examples that must have same label to consider node a leaf

probeRegion = VisionMarkerTrained.ProbeRegion; 

% Now using unpadded images to train
%imageCoords = [-VisionMarkerTrained.FiducialPaddingFraction ...
%    1+VisionMarkerTrained.FiducialPaddingFraction];

DEBUG_DISPLAY = false;
DrawTrees = false;

parseVarargin(varargin{:});

t_start = tic;

if ~iscell(markerImageDir)
    markerImageDir = {markerImageDir};
end

probePattern = VisionMarkerTrained.ProbePattern;

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

savedStateFile = fullfile(fileparts(mfilename('fullpath')), 'trainingState.mat');

if loadSavedProbeValues
    % This load will kill the current pBar, so save that
    load(savedStateFile); %#ok<UNRCH>

    pBar = ProgressBar('VisionMarkerTrained ProbeTree', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));

else
    
    pBar = ProgressBar('VisionMarkerTrained ProbeTree', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));

    
    %% Load marker images
    numDirs = length(markerImageDir);
    fnames = cell(numDirs,1);
    for i_dir = 1:numDirs
        fnames{i_dir} = getfnames(markerImageDir{i_dir}, 'images', 'useFullPath', true);
    end
    fnames = vertcat(fnames{:});
    
    if isempty(fnames)
        error('No image files found.');
    end
    
    fprintf('Found %d total image files to train on.\n', length(fnames));
    
    % Add special all-white and all-black images
    %fnames = [fnames; 'ALLWHITE'; 'ALLBLACK'];
    
    %fnames = {'angryFace.png', 'ankiLogo.png', 'batteries3.png', ...
    %    'bullseye2.png', 'fire.png', 'squarePlusCorners.png'};
    
    numImages = length(fnames);
    labelNames = cell(1,numImages);
    %img = cell(1, numImages);
    
    corners = [0 0; 0 1; 1 0; 1 1];
    sigma = perturbSigma/workingResolution;
    
    numBlurs = length(blurSigmas);
    
    [xgrid,ygrid] = meshgrid(linspace(probeRegion(1),probeRegion(2),workingResolution)); %1:workingResolution);
    %probeValues = zeros(workingResolution^2, numImages);
    probeValues   = cell(numBlurs,numImages);
    gradMagValues = cell(numBlurs,numImages);
    labels        = cell(numBlurs,numImages);
    
    X = probePattern.x(ones(workingResolution^2,1),:) + xgrid(:)*ones(1,length(probePattern.x));
    Y = probePattern.y(ones(workingResolution^2,1),:) + ygrid(:)*ones(1,length(probePattern.y));
    
    % Precompute all the perturbed probe locations once
    pBar.set_message(sprintf('Computing %d perturbed probe locations', numPerturbations));
    pBar.set_increment(1/numPerturbations);
    pBar.set(0);    
    xPerturb = cell(1, numPerturbations);
    yPerturb = cell(1, numPerturbations);
    for iPerturb = 1:numPerturbations
        perturbation = max(-3*sigma, min(3*sigma, sigma*randn(4,2)));
        corners_i = corners + perturbation;
        T = cp2tform(corners_i, corners, 'projective');
        [xPerturb{iPerturb}, yPerturb{iPerturb}] = tforminv(T, X, Y);
        
        pBar.increment();
    end
    
    % Compute the perturbed probe values
    pBar.set_message(sprintf(['Interpolating perturbed probe locations ' ...
        'from %d images at %d blurs'], numImages, numBlurs));
    pBar.set_increment(1/numImages);
    pBar.set(0);
    for iImg = 1:numImages
        
%         if strcmp(fnames{iImg}, 'ALLWHITE')
%             img{iImg} = ones(workingResolution);
%         elseif strcmp(fnames{iImg}, 'ALLBLACK')
%             img{iImg} = zeros(workingResolution);
%         else
            img = imreadAlphaHelper(fnames{iImg});
%         end
        
        [nrows,ncols,~] = size(img);
        imageCoordsX = linspace(0, 1, ncols);
        imageCoordsY = linspace(0, 1, nrows);
                
        [~,labelNames{iImg}] = fileparts(fnames{iImg});
        
        for iBlur = 1:numBlurs
            imgBlur = img;
            if blurSigmas(iBlur) > 0
                blurSigma = blurSigmas(iBlur)*sqrt(nrows^2 + ncols^2);
                imgBlur = separable_filter(imgBlur, gaussian_kernel(blurSigma));
            end
            
            imgGradMag = single(smoothgradient(imgBlur));
            imgBlur = single(imgBlur);
            
            probeValues{iBlur,iImg} = zeros(workingResolution^2, numPerturbations, 'single');
            gradMagValues{iBlur,iImg} = zeros(workingResolution^2, numPerturbations, 'single');
            for iPerturb = 1:numPerturbations
                probeValues{iBlur,iImg}(:,iPerturb) = mean(interp2(imageCoordsX, imageCoordsY, imgBlur, ...
                    xPerturb{iPerturb}, yPerturb{iPerturb}, 'linear', 1), 2);
                
                gradMagValues{iBlur,iImg}(:,iPerturb) = mean(interp2(imageCoordsX, imageCoordsY, imgGradMag, ...
                    xPerturb{iPerturb}, yPerturb{iPerturb}, 'linear', 0), 2);
            end
            
            labels{iBlur,iImg} = iImg*ones(1,numPerturbations, 'uint32');
            
        end % FOR each blurSigma
        
        pBar.increment();
    end
    
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
    
    if addInversions
        % Add white-on-black versions of everything
        labels        = [labels labels+length(labelNames)];
        labelNames    = [labelNames cellfun(@(name)['INVERTED_' name], labelNames, 'UniformOutput', false)];
        probeValues   = [probeValues 1-probeValues];
        gradMagValues = [gradMagValues gradMagValues];
        
        % Get rid of INVERTED_INVALID label -- it's just INVALID
        invertedInvalidLabel = find(strcmp(labelNames, 'INVERTED_INVALID'));
        assert(invertedInvalidLabel == length(labelNames));
        labelNames(end) = [];
        invalidLabel = find(strcmp(labelNames, 'INVALID'));
        labels(labels == invertedInvalidLabel) = invalidLabel;
    end
    
    assert(size(probeValues,2) == length(labels));
    assert(max(labels) == length(labelNames));
    
    %numLabels = numImages;
    numImages = length(labels);
        
    if false && DEBUG_DISPLAY
        namedFigure('Average Perturbed Images'); clf
        for j = 1:numImages
            subplot(2,ceil(numImages/2),j), hold off
            imagesc([0 1], [0 1], reshape(mean(probeValues(:,labels==j),2), workingResolution*[1 1]));
            axis image, hold on
            plot(corners(:,1), corners(:,2), 'y+');
        end
        colormap(gray);
    end
    
    %     %% Add all four rotations
    %     if addRotations
    %         img = cat(3, img, imrotate(img,90), imrotate(img,180), imrotate(img, 270)); %#ok<UNRCH>
    %         if ~numPerturbations
    %             distMap = cat(3, distMap, imrotate(distMap,90), imrotate(distMap,180), imrotate(distMap, 270));
    %         end
    %
    %         labels = repmat(labels, [1 4]);
    %
    %         numImages = 4*numImages;
    %     end

    % Only save what we need to re-run (not all the settings!)
    fprintf('Saving state...');
    save(savedStateFile, 'probeValues', 'gradMagValues', 'fnames', 'labels', 'labelNames', 'numImages', 'xgrid', 'ygrid');
    fprintf('Done.\n');
    
end % if loadSavedProbeValues



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

%% Train Red/Black Verify trees

pBar.set_message('Training red/black verification trees');
pBar.set(0);
totalExamplesToClassify = 2*totalExamplesToClassify;

redMask = false(workingResolution);
%redMask(1:2:end,1:2:end) = true;
%redMask(2:2:end,2:2:end) = true;
redMask(1:round(workingResolution/2),:) = true;

probeTree.verifyTreeRed = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
probeTree.verifyTreeRed.labels = labelNames;
probeTree.verifyTreeRed = buildTree(probeTree.verifyTreeRed, redMask, labels, labelNames, redBlackVerifyDepth);

blackMask = ~redMask;
probeTree.verifyTreeBlack = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
probeTree.verifyTreeBlack.labels = labelNames;
probeTree.verifyTreeBlack = buildTree(probeTree.verifyTreeBlack, blackMask, labels, labelNames, redBlackVerifyDepth);

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

if addInversions
    invert = [false(size(fnames(:))); true(size(fnames(:)))];
    fnames = [fnames(:); fnames(:)];
else
    invert = false(size(fnames));
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
   
    [redResult, redLabelID] = TestTree(probeTree.verifyTreeRed, testImg, tform, 0.5, probePattern);
    [blackResult, blackLabelID] = TestTree(probeTree.verifyTreeBlack, testImg, tform, 0.5, probePattern);
    
    if any(labelID == redLabelID) && any(labelID == blackLabelID)
        assert(any(strcmp(result, redResult)) && ...
            any(strcmp(result, blackResult)));
        verified(iImg) = true;
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
        warning('Reached node with multiple remaining labels: ')
        for j = 1:length(result)
            fprintf('%s, ', labelNames{labels(result(j))});
        end
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

t_test = toc(t_start);

%[numMain, numVerify] = VisionMarkerTrained.GetNumTreeNodes(); % Not valid until we clear VisionMarkerTrained and force a tree re-load.
fprintf('Training completed in %.2f seconds (%.1f minutes), plus %.2f seconds (%.2f minutes) for testing on original images.\n', ...
    t_train, t_train/60, t_test, t_test/60);
% Used %d main tree nodes + %d verification nodes.\n', ...
%    numMain, numVerify);

%% Save Tree
if saveTree
    savePath = fileparts(mfilename('fullpath'));
    save(fullfile(savePath, 'probeTree.mat'), 'probeTree');
    
    fprintf('Probe tree re-trained and saved.  You will need to clear any existing VisionMarkers.\n');
end


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


    




end % TrainProbeTree()


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


function img = imreadAlphaHelper(fname)

[img, ~, alpha] = imread(fname);
img = mean(im2double(img),3);
img(alpha < .5) = 1;

threshold = (max(img(:)) + min(img(:)))/2;
img = double(img > threshold);

end % imreadAlphaHelper()
