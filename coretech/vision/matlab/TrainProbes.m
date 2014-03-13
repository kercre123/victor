function root = TrainProbes(varargin)

%% Parameters
markerImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/letters';
workingResolution = 50;
%maxSamples = 100;
minInfoGain = 0;
maxDepth = 20;
addRotations = true;
numPerturbations = 100;
perturbSigma = 2;

probeRegion = [0 1];
imageCoords = [0 1];

DEBUG_DISPLAY = true;
DrawTrees = false;

parseVarargin(varargin{:});
    
%% Create Probe Pattern
%N_angles = 8;
%angles = linspace(0, 2*pi, N_angles +1);
%probePattern = struct('x', [0 probeRadius/workingResolution*cos(angles)], ...
%    'y', [0 probeRadius/workingResolution*sin(angles)]);
probePattern = VisionMarkerTrained.ProbePattern;

%% Load marker images
fnames = getfnames(markerImageDir, 'images');
%fnames = {'angryFace.png', 'ankiLogo.png', 'batteries3.png', ...
%    'bullseye2.png', 'fire.png', 'squarePlusCorners.png'};

numImages = length(fnames);
labelNames = cell(1,numImages);
img = cell(1, numImages);
distMap = cell(1, numImages);

resamplingResolution = ceil(1/(probeRegion(2)-probeRegion(1))*workingResolution);
%resamplingResolution = 4*workingResolution;

for i = 1:numImages
    img{i} = imread(fullfile(markerImageDir, fnames{i}));
    img{i} = imresize(mean(im2double(img{i}),3), ...
        resamplingResolution*[1 1], 'bilinear');
    
    
    [~,labelNames{i}] = fileparts(fnames{i});
    
end

if ~numPerturbations
    distMap = cat(3, distMap{:});
end
labels = 1:numImages;
numLabels = numImages;
 
%[xgrid,ygrid] = meshgrid(linspace(0,1,workingResolution));
imageCoords = linspace(imageCoords(1), imageCoords(2), resamplingResolution);

%% Perturb corners
[xgrid,ygrid] = meshgrid(imageCoords);
if numPerturbations > 0
    fprintf('Creating perturbed images...');
    
    img_perturb = cell(numPerturbations, numImages);
    %corners = [0 0; 0 workingResolution; workingResolution 0; workingResolution workingResolution] + 0.5;
    corners = [0 0; 0 1; 1 0; 1 1];
    sigma = perturbSigma/workingResolution;
    for i = 1:numPerturbations
        perturbation = max(-3*sigma, min(3*sigma, sigma*randn(4,2)));
        corners_i = corners + perturbation;
        T = cp2tform(corners_i, corners, 'projective');
        [xi,yi] = tforminv(T, xgrid, ygrid);
        
        for i_img = 1:numImages
            %img_perturb{i,i_img} = separable_filter(interp2(img{i_img}, xi, yi, 'linear', 1), probeKernel, [], 'replicate');
            img_perturb{i,i_img} = interp2(imageCoords, imageCoords, img{i_img}, xi, yi, 'linear', 1);
        end
    end
    
    if DEBUG_DISPLAY
        namedFigure('Average Perturbed Images'); clf
        for j = 1:numImages
            subplot(2,ceil(numImages/2),j), hold off
            imagesc(imageCoords([1 end]), imageCoords([1 end]), ...
                mean(cat(3, img_perturb{:,j}),3)), axis image, hold on
            plot(corners(:,1), corners(:,2), 'y+');
        end
        colormap(gray);
    end
    
    labels = [labels row(repmat(labels, [numPerturbations 1]))];
    img = [img(:); img_perturb(:)];
    fprintf('Done.\n');
end
img = cat(3, img{:});
numImages = size(img,3);

%% Add all four rotations
if addRotations
    img = cat(3, img, imrotate(img,90), imrotate(img,180), imrotate(img, 270));
    if ~numPerturbations
        distMap = cat(3, distMap, imrotate(distMap,90), imrotate(distMap,180), imrotate(distMap, 270));
    end
    
    labels = repmat(labels, [1 4]);
    
    numImages = 4*numImages;
end

%% Add all-white & all-black images
% Force the tree to use at least one black and one white probe from each
% image we actually care about.

img = cat(3, img, ones(resamplingResolution), zeros(resamplingResolution));
labelNames{end+1} = 'ALL_WHITE';
labelNames{end+1} = 'ALL_BLACK';
labels = [labels numLabels+1 numLabels+2];
numLabels = numLabels + 2;
numImages = numImages + 2;

%% Create gradient weight map
% Ix = (image_right(img) - image_left(img))/2;
% Iy = (image_down(img) - image_up(img))/2;
% gradMag = sqrt(Ix.^2 + Iy.^2);


%% Create samples 
%probeValues = reshape(img, [], numImages);
fprintf('Computing probe values...');
[xgrid,ygrid] = meshgrid(linspace(probeRegion(1),probeRegion(2),workingResolution)); %1:workingResolution);
probeValues = zeros(workingResolution^2, numImages);
X = probePattern.x(ones(workingResolution^2,1),:) + xgrid(:)*ones(1,length(probePattern.x));
Y = probePattern.y(ones(workingResolution^2,1),:) + ygrid(:)*ones(1,length(probePattern.y));

assert(isequal(size(img(:,:,1)), resamplingResolution*[1 1]))
% Note working resolution is the resoultion of the code image, without the
% fiducial, which is what the GetFiducialPixelSize function expects
Corners = VisionMarkerTrained.GetFiducialCorners(resamplingResolution);
tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
[xi,yi] = tformfwd(tform, X, Y);
for i = 1:numImages
   probeValues(:,i) = mean(interp2(img(:,:,i), xi, yi, 'linear', 1),2);
end
fprintf('Done.\n');



%% Train the decision tree

root = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
root.labels = labelNames;

try
    root = buildTree(root, false(workingResolution), labels, labelNames);
catch E
    switch(E.identifier)
        case {'BuildTree:MaxDepth', 'BuildTree:UselessSplit'}
            fprintf('Unable to build tree. Likely ambiguities: "%s".\n', ...
                E.message);
            root = [];
            return
            
        otherwise
            rethrow(E);
    end
end

if DEBUG_DISPLAY && DrawTrees
    namedFigure('Multiclass DecisionTree'), clf
    fprintf('Drawing multi-class tree...');
    DrawTree(root, 0);
    fprintf('Done.\n');
end

%% Train one-vs-all trees

assert(length(labelNames) == numLabels);

verifier = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
for i_label = 1:numLabels
    
    fprintf('\n\nTraining one-vs-all tree for "%s"\n', labelNames{i_label});
    
    currentLabels = double(labels == i_label) + 1;
    currentLabelNames = {'UNKNOWN', labelNames{i_label}};
    
    root.verifiers(i_label) = buildTree(verifier, ...
        false(workingResolution), currentLabels, currentLabelNames);
    
    if DEBUG_DISPLAY && DrawTrees
        namedFigure(sprintf('%s DecisionTree', labelNames{i_label})), clf
        fprintf('Drawing %s tree...', labelNames{i_label});
        DrawTree(root.verifiers(i_label), 0);
        fprintf('Done.\n');
    end

end


%% Create Verifiers
% numVerificationProbes = 20;
% verifiers = struct();
% for i_label = 1:numLabels
%     
%     used = false(workingResolution);
%     [mask_x,mask_y] = meshgrid(-ceil(probeRadius):ceil(probeRadius));
%     
%     probeIsOn = 1 - probeValues(:,labels==i_label);
%     p_on = sum(probeIsOn,2)/size(probeIsOn,2);
%     p_off = 1 - p_on;
%     
%     entropy = -(p_on.*log2(max(eps,p_on)) + p_off.*log2(max(eps,p_off)));
% 
%     % Choose the probes with the highest entropy, that are not all right on
%     % top of each other
%     [~,sortIndex]  = sort(entropy, 'ascend');
%     i_probe = 1;
%     verifiers(i_label).x = zeros(1,numVerificationProbes);
%     verifiers(i_label).y = zeros(1,numVerificationProbes);
%     verifiers(i_label).isOn = false(1, numVerificationProbes);
%     for k = 1:numVerificationProbes
%         
%         while used(sortIndex(i_probe))
%             i_probe = i_probe + 1;
%             
%             if i_probe > workingResolution^2
%                 error('Ran out of verification probe options!');
%             end
%         end
%         
%         selectedProbe = sortIndex(i_probe);
%         verifiers(i_label).x(k)    = xgrid(selectedProbe);
%         verifiers(i_label).y(k)    = ygrid(selectedProbe);
%         verifiers(i_label).isOn(k) = p_on(selectedProbe) > 0.5;
%         
%         [usedRow,usedCol] = ind2sub(workingResolution*[1 1], selectedProbe);
%         used(max(1,min(workingResolution, usedRow + mask_y)), ...
%             max(1,min(workingResolution, usedCol + mask_x))) = true;
%         assert(used(selectedProbe))
%     end
%     
%     for i =1:4
%         subplot(1,4,i), hold off
%         imagesc([-.1 1.1], [-.1 1.1], imrotate(img(:,:,find(labels==i_label,1)), (i-1)*90))
%         axis image, hold on
%         isOn = verifiers(i_label).isOn; 
%         plot(verifiers(i_label).x(isOn),  verifiers(i_label).y(isOn), 'go'); 
%         plot(verifiers(i_label).x(~isOn), verifiers(i_label).y(~isOn), 'ro'); 
%     end
%     
%     
% end
% 
% root.verifiers = verifiers;

%% Test on Training Data

fprintf('Testing on training images...');
if DEBUG_DISPLAY
    namedFigure('Sampled Test Results'); clf
    colormap(gray)
    maxDisplay = 48;
else
    maxDisplay = 0;
end
correct = false(1,numImages);
verified = false(1,numImages);
randIndex = row(randperm(numImages));
for i_rand = 1:numImages
    i = randIndex(i_rand);
   
    testImg = img(:,:,i);
    
    Corners = VisionMarkerTrained.GetFiducialCorners(size(testImg,1));
    tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
    
    if i_rand <= maxDisplay
        subplot(6, maxDisplay/6, i_rand), hold off
        imagesc(testImg), axis image, hold on
        plot(Corners(:,1), Corners(:,2), 'y+');
        doDisplay = true;
    else 
        doDisplay = false;
    end
    
    [result, labelID] = TestTree(root, testImg, tform, 0.5, probePattern, doDisplay);
    
    assert(labelID>0 && labelID<=length(root.verifiers));
    
    [verificationResult, verifiedID] = TestTree(root.verifiers(labelID), testImg, tform, 0.5, probePattern);
    verified(i) = verifiedID == 2;
    if verified(i)
        assert(strcmp(verificationResult, result));
    end
    
    if ischar(result)
        correct(i) = strcmp(result, labelNames{labels(i)});
        if ~verified(i)
            color = 'b';
        else
            if ~correct(i)
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

% Also test on original images
correct = false(1,length(fnames));
verified = false(1,length(fnames));
if DEBUG_DISPLAY
    namedFigure('Original Image Errors'), clf
end
for i = 1:length(fnames)    
    testImg = mean(im2double(imread(fullfile(markerImageDir, fnames{i}))),3);
    % radius = probeRadius/workingResolution * size(testImg,1);
        
    %testImg = separable_filter(testImg, gaussian_kernel(radius/2), [], 'replicate');
    Corners = VisionMarkerTrained.GetFiducialCorners(size(testImg,1));
    tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
    
    [result, labelID] = TestTree(root, testImg, tform, 0.5, probePattern);
    
    [verificationResult, verifiedID] = TestTree(root.verifiers(labelID), testImg, tform, 0.5, probePattern);
    verified(i) = verifiedID == 2;
    if verified(i)
        assert(strcmp(verificationResult, result));
    end
    
    if ischar(result)
        correct(i) = strcmp(result, labelNames{labels(i)});
        
        if DEBUG_DISPLAY % && ~correct(i)
            h_axes = subplot(2,ceil(length(fnames)/2),i);
            imagesc(testImg, 'Parent', h_axes); hold on
            axis(h_axes, 'image');
            TestTree(root, testImg, tform, 0.5, probePattern, true);
            plot(Corners(:,1), Corners(:,2), 'y+');
            title(h_axes, result);
            if ~verified(i)
                color = 'b';
            else
                if correct(i)
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

    function node = buildTree(node, used, labels, labelNames)
        
        if all(used(:))
            error('All probes used.');
        end
        
        if all(labels(node.remaining)==labels(node.remaining(1)))
            node.labelID = labels(node.remaining(1));
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
            % are more than one, choose the one furthest from the current
            % set of probes (?)
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

end