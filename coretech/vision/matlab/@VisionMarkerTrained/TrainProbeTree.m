function probeTree = TrainProbeTree(varargin)

% TODO: Use parameters from a derived class below, insetad of VisionMarkerTrained.*

%% Parameters
loadSavedProbeValues = false;
markerImageDir = VisionMarkerTrained.TrainingImageDir;
workingResolution = VisionMarkerTrained.ProbeParameters.GridSize;
%maxSamples = 100;
minInfoGain = 0;
maxDepth = 50;
addRotations = false;
numPerturbations = 100;
perturbSigma = 1;
saveTree = true;
leafNodeFraction = 1;

probeRegion = VisionMarkerTrained.ProbeRegion; 

% Now using unpadded images to train
%imageCoords = [-VisionMarkerTrained.FiducialPaddingFraction ...
%    1+VisionMarkerTrained.FiducialPaddingFraction];

DEBUG_DISPLAY = true;
DrawTrees = false;

parseVarargin(varargin{:});

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

pBar = ProgressBar('VisionMarkerTrained ProbeTree', 'CancelButton', true);
pBar.showTimingInfo = true;

if loadSavedProbeValues
    load trainingState.mat
else
    
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
    fnames = [fnames; 'ALLWHITE'; 'ALLBLACK'];
    
    %fnames = {'angryFace.png', 'ankiLogo.png', 'batteries3.png', ...
    %    'bullseye2.png', 'fire.png', 'squarePlusCorners.png'};
    
    numImages = length(fnames);
    labelNames = cell(1,numImages);
    img = cell(1, numImages);
    
    corners = [0 0; 0 1; 1 0; 1 1];
    sigma = perturbSigma/workingResolution;
    
    [xgrid,ygrid] = meshgrid(linspace(probeRegion(1),probeRegion(2),workingResolution)); %1:workingResolution);
    %probeValues = zeros(workingResolution^2, numImages);
    probeValues = cell(1,numImages);
    labels      = cell(1,numImages);
    
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
    pBar.set_message(sprintf('Interpolating perturbed probe locations from %d images', numImages));
    pBar.set_increment(1/numImages);
    pBar.set(0);
    for iImg = 1:numImages
        
        if strcmp(fnames{iImg}, 'ALLWHITE')
            img{iImg} = ones(workingResolution);
        elseif strcmp(fnames{iImg}, 'ALLBLACK')
            img{iImg} = zeros(workingResolution);
        else
            [img{iImg}, ~, alpha] = imread(fnames{iImg});
            img{iImg} = mean(im2double(img{iImg}),3);
            img{iImg}(alpha < .5) = 1;
        end
        
        imageCoordsX = linspace(0, 1, size(img{iImg},2));
        imageCoordsY = linspace(0, 1, size(img{iImg},1));
        
        [~,labelNames{iImg}] = fileparts(fnames{iImg});
        probeValues{iImg} = zeros(workingResolution^2, numPerturbations);
        for iPerturb = 1:numPerturbations
            probeValues{iImg}(:,iPerturb) = mean(interp2(imageCoordsX, imageCoordsY, img{iImg}, ...
                xPerturb{iPerturb}, yPerturb{iPerturb}, 'linear', 1), 2);            
        end
        labels{iImg} = iImg*ones(1,numPerturbations);
        
        pBar.increment();
    end
    img = cat(3, img{:});
    numImages = size(img,3);
    
    probeValues = [probeValues{:}];
    labels = [labels{:}];
    numLabels = numImages;
        
    if DEBUG_DISPLAY
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

    save trainingState.mat
    
end % if loadSavedProbeValues



%% Train the decision tree

probeTree = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
probeTree.labels = labelNames;

try
    probeTree = buildTree(probeTree, false(workingResolution), labels, labelNames);
catch E
    switch(E.identifier)
        case {'BuildTree:MaxDepth', 'BuildTree:UselessSplit'}
            fprintf('Unable to build tree. Likely ambiguities: "%s".\n', ...
                E.message);
            %probeTree = [];
            return
            
        otherwise
            rethrow(E);
    end
end

if DEBUG_DISPLAY && DrawTrees
    namedFigure('Multiclass DecisionTree'), clf
    fprintf('Drawing multi-class tree...');
    DrawTree(probeTree, 0);
    fprintf('Done.\n');
end

%% Train one-vs-all trees

assert(length(labelNames) == numLabels);

pBar.set_message('Building one-vs-all verification trees');
pBar.set_increment(1/numLabels);
pBar.set(0);

verifier = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
for i_label = 1:numLabels
    
    fprintf('\n\nTraining one-vs-all tree for "%s"\n', labelNames{i_label});
    
    currentLabels = double(labels == i_label) + 1;
    currentLabelNames = {'UNKNOWN', labelNames{i_label}};
    
    probeTree.verifiers(i_label) = buildTree(verifier, ...
        false(workingResolution), currentLabels, currentLabelNames);
    
    if DEBUG_DISPLAY && DrawTrees
        namedFigure(sprintf('%s DecisionTree', labelNames{i_label})), clf
        fprintf('Drawing %s tree...', labelNames{i_label});
        DrawTree(probeTree.verifiers(i_label), 0);
        fprintf('Done.\n');
    end

    pBar.increment();
end

if isempty(probeTree)
    error('Training failed!');
end


%% Test on Training Data

fprintf('Testing on %d training images...', numImages);
pBar.set_message(sprintf('Testing on %d training images...', numImages));
pBar.set_increment(1/numImages);
pBar.set(0);

if DEBUG_DISPLAY
    namedFigure('Sampled Test Results'); clf
    colormap(gray)
    maxDisplay = 48;
else
    maxDisplay = 0; %#ok<UNRCH>
end
correct = false(1,numImages);
verified = false(1,numImages);
randIndex = row(randperm(numImages));
for i_rand = 1:numImages
    iImg = randIndex(i_rand);
   
    testImg = img(:,:,iImg);
    
    Corners = VisionMarkerTrained.GetFiducialCorners(size(testImg,1), false);
    %Corners = VisionMarkerTrained.GetMarkerCorners(size(testImg,1), false);
    if i_rand <= maxDisplay
        subplot(6, maxDisplay/6, i_rand), hold off
        imagesc(testImg), axis image, hold on
        plot(Corners(:,1), Corners(:,2), 'y+');
        doDisplay = true;
    else 
        doDisplay = false;
    end
    
    [result, labelID] = TestTree(probeTree, testImg, tform, 0.5, probePattern, doDisplay);
    
    assert(labelID>0 && labelID<=length(probeTree.verifiers));
    
    [verificationResult, verifiedID] = TestTree(probeTree.verifiers(labelID), testImg, tform, 0.5, probePattern);
    verified(iImg) = verifiedID == 2;
    if verified(iImg)
        assert(strcmp(verificationResult, result));
    end
    
    if ischar(result)
        correct(iImg) = strcmp(result, labelNames{labels(iImg)});
        if ~verified(iImg)
            color = 'b';
        else
            if ~correct(iImg)
                color = 'r';
            else
                color = 'g';
            end
        end
        
        if i_rand <= maxDisplay
            set(gca, 'XColor', color, 'YColor', color);
            title(result)
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
       fprintf('User cancelled testing on training images.\n');
       break; 
    end
end
fprintf(' Got %d of %d training images right (%.1f%%)\n', ...
    sum(correct), length(correct), sum(correct)/length(correct)*100);
if sum(correct) ~= length(correct)
    warning('We should have ZERO training error!');
end

fprintf(' %d of %d training images verified.\n', ...
    sum(verified), length(verified));
if sum(verified) ~= length(verified)
    warning('All training images should verify!');
end

%% Test on original images

fprintf('Testing on %d original images...', length(fnames));
pBar.set_message(sprintf('Testing on %d original images...', length(fnames)));
pBar.set_increment(1/length(fnames));
pBar.set(0);

correct = false(1,length(fnames));
verified = false(1,length(fnames));
if DEBUG_DISPLAY
    namedFigure('Original Image Errors'), clf
    numDisplayRows = floor(sqrt(length(fnames)));
    numDisplayCols = ceil(length(fnames)/numDisplayRows);
end
for iImg = 1:length(fnames)   
    [testImg,~,alpha] = imread(fnames{iImg});
    testImg = mean(im2double(testImg),3);
    testImg(alpha < .5) = 1;
    
    % radius = probeRadius/workingResolution * size(testImg,1);
        
    %testImg = separable_filter(testImg, gaussian_kernel(radius/2), [], 'replicate');
    Corners = VisionMarkerTrained.GetFiducialCorners(size(testImg,1), false);
    %Corners = VisionMarkerTrained.GetMarkerCorners(size(testImg,1), false);
    tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
    
    [result, labelID] = TestTree(probeTree, testImg, tform, 0.5, probePattern);
    
    [verificationResult, verifiedID] = TestTree(probeTree.verifiers(labelID), testImg, tform, 0.5, probePattern);
    verified(iImg) = verifiedID == 2;
    if verified(iImg)
        assert(strcmp(verificationResult, result));
    end
    
    if ischar(result)
        correct(iImg) = strcmp(result, labelNames{labels(iImg)});
        
        if DEBUG_DISPLAY % && ~correct(i)
            h_axes = subplot(numDisplayRows,numDisplayCols,iImg);
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


%% Save Tree
if saveTree
    savePath = fileparts(mfilename('fullpath'));
    save(fullfile(savePath, 'probeTree.mat'), 'probeTree');
    
    fprintf('Probe tree re-trained and saved.  You will need to clear any existing VisionMarkers.\n');
end


%% buildTree() Nested Function

    function node = buildTree(node, used, labels, labelNames)
        
        if all(used(:))
            error('All probes used.');
        end
        
        %if all(labels(node.remaining)==labels(node.remaining(1)))
        remainingNodes = labels(node.remaining);
        counts = hist(remainingNodes, length(remainingNodes));
        [maxCount, maxIndex] = max(counts);
        if maxCount >= leafNodeFraction*length(counts)
            node.labelID = labels(remainingNodes(maxIndex));
            node.labelName = labelNames{node.labelID};
            fprintf('LeafNode for label = %d, or "%s"\n', node.labelID, node.labelName);
           
        else            
            unusedProbes = find(~used);
            
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
            
            if maxInfoGain < minInfoGain
                warning('Making too little progress differentiating [%s], giving up on this branch.', ...
                    sprintf('%s ', labelNames{labels(node.remaining)}));
                %node = struct('x', -1, 'y', -1, 'whichProbe', -1);
                return
            end
            
            whichUnusedProbe = find(infoGain == maxInfoGain);
            if length(whichUnusedProbe)>1
                index = randperm(length(whichUnusedProbe));
                whichUnusedProbe = whichUnusedProbe(index(1));
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
                % letting us choose this node again
                used(node.whichProbe) = true;
                node = buildTree(node, used, labels, labelNames);
                
            elseif node.depth+1 > maxDepth
                error('BuildTree:MaxDepth', ...
                        'Reached max depth with [%s\b] remaining.', ...
                        sprintf('%s ', labelNames{labels(leftRemaining)}));
                    
            else
                % Recurse left
                usedLeft = used;
                usedLeft(node.whichProbe) = true;
                
                leftChild.remaining = leftRemaining;
                leftChild.depth = node.depth+1;
                node.leftChild = buildTree(leftChild, usedLeft, labels, labelNames);
                
                % Recurse right
                rightRemaining = node.remaining(~goLeft);
                assert(~isempty(rightRemaining) && length(rightRemaining) < length(node.remaining));
                
                usedRight = used;
                usedRight(node.whichProbe) = true;
                
                rightChild.remaining = rightRemaining;
                rightChild.depth = node.depth+1;
                node.rightChild = buildTree(rightChild, usedRight, labels, labelNames);
                
            end
        end
        
    end % buildTree()


    




end % TrainProbes()


function infoGain = computeInfoGain(labels, numLabels, probeValues)

markerProb = max(eps,hist(labels, 1:numLabels));
markerProb = markerProb/max(eps,sum(markerProb));
currentEntropy = -sum(markerProb.*log2(markerProb));

numProbes = size(probeValues,1);

% Use the (blurred) image values directly as indications of the
% probability a probe is "on" or "off" instead of thresholding.
% The hope is that this discourages selecting probes near
% edges, which will be gray ("uncertain"), instead of those far
% from any edge in any image, since those will still be closer
% to black or white even after blurring.
probeIsOn = 1 - probeValues;

markerProb_on  = zeros(numProbes,numLabels);
markerProb_off = zeros(numProbes,numLabels);

for i_probe = 1:numProbes
    %markerProb_on(i_probe,:) = max(eps,hist(L(probeIsOn(i_probe,:)), 1:numImages));
    %markerProb_off(i_probe,:) = max(eps,hist(L(~probeIsOn(i_probe,:)), 1:numImages));
    markerProb_on(i_probe,:) = max(eps, accumarray(labels(:), probeIsOn(i_probe,:)', [numLabels 1])');
    markerProb_off(i_probe,:) = max(eps, accumarray(labels(:), 1-probeIsOn(i_probe,:)', [numLabels 1])');
end
markerProb_on  = markerProb_on  ./ (sum(markerProb_on,2)*ones(1,numLabels));
markerProb_off = markerProb_off ./ (sum(markerProb_off,2)*ones(1,numLabels));

p_on = sum(probeIsOn,2)/size(probeIsOn,2);
p_off = 1 - p_on;

conditionalEntropy = - (p_on.*sum(markerProb_on.*log2(max(eps,markerProb_on)), 2) + ...
    p_off.*sum(markerProb_off.*log2(max(eps,markerProb_off)), 2));

infoGain = currentEntropy - conditionalEntropy;

end % computeInfoGain()

