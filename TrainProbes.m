function root = TrainProbes(varargin)

%% Parameters
markerImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/letters';
workingResolution = 50;
probeRadius = 5;
%maxSamples = 100;
minInfoGain = 0;
maxDepth = 20;
addRotations = true;
numPerturbations = 100;
perturbSigma = 2;

probeRegion = [0 1];
imageCoords = [0 1];

DEBUG_DISPLAY = true;

parseVarargin(varargin{:});
    
%% Create Probe Pattern
N_angles = 8;
angles = linspace(0, 2*pi, N_angles +1);
probePattern = struct('x', [0 probeRadius/workingResolution*cos(angles)], ...
    'y', [0 probeRadius/workingResolution*sin(angles)]);

%% Load marker images
fnames = getfnames(markerImageDir, 'images');
%fnames = {'angryFace.png', 'ankiLogo.png', 'batteries3.png', ...
%    'bullseye2.png', 'fire.png', 'squarePlusCorners.png'};

numImages = length(fnames);
labelNames = cell(1,numImages);
img = cell(1, numImages);
distMap = cell(1, numImages);

resamplingResolution = round(1/(probeRegion(2)-probeRegion(1))*workingResolution);

for i = 1:numImages
    img{i} = imread(fullfile(markerImageDir, fnames{i}));
    
    if numPerturbations > 0
        img{i} = imresize(mean(im2double(img{i}),3), resamplingResolution*[1 1], 'nearest');
    else
        img{i} = imresize(mean(im2double(img{i}),3), resamplingResolution*[1 1], 'bilinear');
        distMap{i} = img{i}(:,:,1) > 0.5;
        
        img{i} = separable_filter(img{i}, gaussian_kernel(0.5*probeRadius), [], 'replicate');
    end
    
    if ~numPerturbations
        distMap{i} = bwdist( ...
            image_right(distMap{i}) ~= distMap{i} | ...
            image_left(distMap{i})  ~= distMap{i} | ...
            image_up(distMap{i})    ~= distMap{i} | ...
            image_down(distMap{i})  ~= distMap{i});
        %distMap{i} = imresize(distMap{i}, workingResolution*[1 1]);
    end
    
    [~,labelNames{i}] = fileparts(fnames{i});
end

if ~numPerturbations
    distMap = cat(3, distMap{:});
end
labels = 1:numImages;
numLabels = numImages;
 
%assert(probeRegion(1)>=0 && probeRegion(1)<1 && probeRegion(2)<=1 && probeRegion(2)>0 && probeRegion(1)<probeRegion(2), 'Invalid crop.');

%[xgrid,ygrid] = meshgrid(linspace(0,1,workingResolution));

imageCoords = linspace(imageCoords(1), imageCoords(2), resamplingResolution);

%% Perturb corners
[xgrid,ygrid] = meshgrid(imageCoords);
if numPerturbations
    fprintf('Creating perturbed images...');
    
    img_perturb = cell(numPerturbations, numImages);
    %corners = [0 0; 0 workingResolution; workingResolution 0; workingResolution workingResolution] + 0.5;
    corners = [0 0; 0 1; 1 0; 1 1];
    sigma = perturbSigma/resamplingResolution;
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
            subplot(2,ceil(numImages/2),j)
            imagesc(imageCoords([1 end]), imageCoords([1 end]), ...
                mean(cat(3, img_perturb{:,j}),3)), axis image
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

[~, padding] = VisionMarkerTrained.GetFiducialPixelSize(resamplingResolution);
Corners = [padding padding;
    padding resamplingResolution-padding;
    resamplingResolution-padding padding;
    resamplingResolution-padding resamplingResolution-padding];
tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
[xi,yi] = tformfwd(tform, X, Y);
for i = 1:numImages
   probeValues(:,i) = mean(interp2(img(:,:,i), xi, yi, 'linear', 1),2);
end
fprintf('Done.\n');
% probeValues = cell(1,numImages*4);
% for i = 1:numImages*4
%     probeValues{i}  = interp2(img(:,:,i), xgrid, ygrid, 'nearest');    
% end
% probeValues = reshape(cat(3, probeValues{:}), [], 4*numImages);

% probeGradMag = interp2(probeGradMag, xgrid, ygrid, 'nearest');
%probeGradMag = 1 - probeGradMag / max(probeGradMag(:));


%% Start with empty probe list

root = struct('depth', 0, 'infoGain', 0, 'remaining', 1:numImages);
root.labels = labelNames;

try
    root = buildTree(root, false(workingResolution));
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
    
if DEBUG_DISPLAY
    namedFigure('DecisionTree'), clf
    fprintf('Drawing tree...');
    DrawTree(root, 0);
    fprintf('Done.\n');
end

% probes = GetProbeList(root);
%
% namedFigure('ProbeLocations'), colormap(gray), clf
%
% for i = 1:min(32,numImages)
%     subplot(4,min(8,numImages),i), hold off, imagesc(img(:,:,i)), axis image, hold on
%     
%     fieldName = ['label_' labelNames{labels(i)}];
%     if isfield(probes, fieldName)
%         usedProbes = probes.(fieldName);
%         plot(xgrid(usedProbes), ygrid(usedProbes), 'ro');
%     else
%         warning('No field for label "%s"', labelNames{labels(i)});
%     end
% end

fprintf('Testing on training images...');
if DEBUG_DISPLAY
    namedFigure('Sampled Test Results'); clf
    colormap(gray)
    maxDisplay = 48;
else
    maxDisplay = 0;
end
correct = false(1,numImages);
randIndex = row(randperm(numImages));
for i_rand = 1:numImages
    i = randIndex(i_rand);
    
    %testImg = mean(im2double(imread(fullfile(markerImageDir, fnames{i}))),3);
    testImg = img(:,:,i);
    %radius = probeRadius/workingResolution * size(testImg,1);
    if i_rand <= maxDisplay
        subplot(6, maxDisplay/6, i_rand), hold off
        imagesc(testImg), axis image, hold on
        doDisplay = true;
    else 
        doDisplay = false;
    end
    
    %testImg = separable_filter(testImg, gaussian_kernel(radius/2), [], 'replicate');
    [nrows,ncols,~] = size(testImg);
    [~, padding] = VisionMarkerTrained.GetFiducialPixelSize(nrows);
    Corners = [padding padding;
        padding nrows-padding;
        ncols-padding padding;
        ncols-padding nrows-padding];
    tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
    
    result = TestTree(root, testImg, tform, 0.5, probePattern, doDisplay);
    if ischar(result)
        correct(i) = strcmp(result, labelNames{labels(i)});
        if ~correct(i)
            color = 'r';
        else 
            color = 'g';
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

% Also test on original images
correct = false(1,length(fnames));
if DEBUG_DISPLAY
    namedFigure('Original Image Errors'), clf
end
for i = 1:length(fnames)    
    testImg = mean(im2double(imread(fullfile(markerImageDir, fnames{i}))),3);
    % radius = probeRadius/workingResolution * size(testImg,1);
        
    %testImg = separable_filter(testImg, gaussian_kernel(radius/2), [], 'replicate');
    [nrows,ncols,~] = size(testImg);
    [~, padding] = VisionMarkerTrained.GetFiducialPixelSize(nrows);
    Corners = [padding padding;
        padding nrows-padding;
        ncols-padding padding;
        ncols-padding nrows-padding];
    tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
    
    result = TestTree(root, testImg, tform, 0.5, probePattern);
    if ischar(result)
        correct(i) = strcmp(result, labelNames{labels(i)});
        
        if DEBUG_DISPLAY && ~correct(i)
            h_axes = axes; %#ok<LAXES>
            imagesc(testImg, 'Parent', h_axes); hold on
            axis(h_axes, 'image');
            TestTree(root, testImg, tform, 0.5, probePattern, true);
            title(h_axes, result);
        end
                        
    else
        warning('Reached node with multiple remaining labels: ')
        for j = 1:length(result)
            fprintf('%s, ', labelNames{labels(result(j))});
        end
        fprintf('\b\b\n');
    end
    
end
if DEBUG_DISPLAY && any(~correct)
    fix_subplots(2,ceil(sum(~correct)/2));
end
fprintf('Got %d of %d original images right (%.1f%%)\n', ...
    sum(correct), length(correct), sum(correct)/length(correct)*100);
if sum(correct) ~= length(correct)
    warning('We should have ZERO errors on original images!');
end

    function node = buildTree(node, used)
        
        if all(used(:))
            error('All probes used.');
        end
        
        if all(labels(node.remaining)==labels(node.remaining(1)))
            node.labelID = labels(node.remaining(1));
            node.labelName = labelNames{node.labelID};
            fprintf('LeafNode for label = %d, or "%s"\n', node.labelID, node.labelName);
           
        else 
            markerProb = max(eps,hist(labels(node.remaining), 1:numLabels));
            markerProb = markerProb/max(eps,sum(markerProb));

%             % Compute distance map from current probes
%             if any(used(:))
%                 probeDistMap = bwdist(used);
%                 %probeDistMap = interp2(distMap, xgrid(~used), ygrid(~used), 'nearest');
%                 probeDistMap = 1 - probeDistMap/max(probeDistMap(:));
%             else
%                 probeDistMap = ones(size(used));
%             end
            
%             % Sample possible probes at this split, weighted by those far
%             % from the edges of all remaining images at this node (i.e.
%             % places that are local minima of gradient magnitude)
%             % TODO: Also take distance from previously-used locations?
%             probeGradMag = sum(gradMag(:,:,node.remaining),3);
%             localMinima = find(...
%                 probeGradMag < image_right(probeGradMag) & ...
%                 probeGradMag < image_left(probeGradMag) & ...
%                 probeGradMag < image_up(probeGradMag) & ...
%                 probeGradMag < image_down(probeGradMag));
%             if length(localMinima) > maxSamples
%                 [~, sortIndex] = sort(probeGradMag(localMinima), 'ascend');
%                 localMinima = localMinima(sortIndex(1:maxSamples));
%             end
%             %[ysamples,xsamples] = ind2sub(workingResolution*[1 1], localMinima);

%localMinima = 1:numel(xgrid);
unusedProbes = find(~used);
       

                        
            % Compute the information gain of choosing each probe location
            currentEntropy = -sum(markerProb.*log2(markerProb));
            numProbes = length(unusedProbes);

            % Use the (blurred) image values directly as indications of the
            % probability a probe is "on" or "off" instead of thresholding.
            % The hope is that this discourages selecting probes near
            % edges, which will be gray ("uncertain"), instead of those far 
            % from any edge in any image, since those will still be closer 
            % to black or white even after blurring.
            probeIsOn = 1 - probeValues(unusedProbes,node.remaining);
                        
            markerProb_on  = zeros(numProbes,numLabels);
            markerProb_off = zeros(numProbes,numLabels);
            
            L = labels(node.remaining); 
            for i_probe = 1:numProbes
                %markerProb_on(i_probe,:) = max(eps,hist(L(probeIsOn(i_probe,:)), 1:numImages));
                %markerProb_off(i_probe,:) = max(eps,hist(L(~probeIsOn(i_probe,:)), 1:numImages));
                markerProb_on(i_probe,:) = max(eps, accumarray(L(:), probeIsOn(i_probe,:)', [numLabels 1])');
                markerProb_off(i_probe,:) = max(eps, accumarray(L(:), 1-probeIsOn(i_probe,:)', [numLabels 1])');
            end
            markerProb_on = markerProb_on ./ (sum(markerProb_on,2)*ones(1,numLabels));
            %markerProb_off = 1 - markerProb_on;
            markerProb_off = markerProb_off ./ (sum(markerProb_off,2)*ones(1,numLabels));
            
            p_on = sum(probeIsOn,2)/size(probeIsOn,2);
            p_off = 1 - p_on;
            
            conditionalEntropy = - (p_on.*sum(markerProb_on.*log2(max(eps,markerProb_on)), 2) + ...
                p_off.*sum(markerProb_off.*log2(max(eps,markerProb_off)), 2));
            
            infoGain = currentEntropy - conditionalEntropy;
            
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
%                 if any(used(:))
%                     distMap = bwdist(used);
%                     [~,furthestAway] = max(distMap(unusedProbes(whichUnusedProbe)));
%                     whichUnusedProbe = whichUnusedProbe(furthestAway);
%                 else
                    index = randperm(length(whichUnusedProbe));
                    whichUnusedProbe = whichUnusedProbe(index(1));
%                 end
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
            if ~numPerturbations
                D = reshape(distMap(:,:,node.remaining), [], length(node.remaining));
                uncertain = D(node.whichProbe,:) < probeRadius;
            else
                uncertain = false(size(node.remaining));
            end
            
            if all(uncertain)
               error('BuildTree:ProbeOnEdges', ...
                   'Selected probe is uncertain for all remaining images.'); 
               %used(node.whichProbe) = true;
               %node = buildTree(node, used);
            else
                leftRemaining = node.remaining(goLeft | uncertain);
                if ~isempty(leftRemaining)
                    usedLeft = used;
                    usedLeft(node.whichProbe) = true;
                    
                    if length(leftRemaining) == length(node.remaining)
                        if any(uncertain)
                            node = buildTree(node, usedLeft);
                        else
                            error('BuildTree:UselessSplit', ...
                                'Reached split that makes no progress with [%s\b] remaining.', ...
                                sprintf('%s ', labelNames{labels(node.remaining)}));
                        end
                    else
                        leftChild.remaining = leftRemaining;
                        leftChild.depth = node.depth+1;
                        if leftChild.depth <= maxDepth
                            node.leftChild = buildTree(leftChild, usedLeft);
                        else
                            error('BuildTree:MaxDepth', ...
                                'Reached max depth with [%s] remaining.', ...
                                sprintf('%s ', labelNames{labels(leftRemaining)}));
                        end
                    end
                end
                
                %rightRemaining = node.remaining(probeValues(node.whichProbe,node.remaining) >= .5);
                rightRemaining = node.remaining(~goLeft | uncertain);
                if ~isempty(rightRemaining)
                    usedRight = used;
                    usedRight(node.whichProbe) = true;
                    
                    if length(rightRemaining) == length(node.remaining)
                        if any(uncertain)
                            node = buildTree(node, usedRight);
                        else
                            error('BuildTree:UselessSplit', ...
                                'Reached split that makes no progress with [%s\b] remaining.', ...
                                sprintf('%s ', labelNames{labels(node.remaining)}));
                        end
                    else
                        
                        rightChild.remaining = rightRemaining;
                        rightChild.depth = node.depth+1;
                        if rightChild.depth <= maxDepth
                            node.rightChild = buildTree(rightChild, usedRight);
                        else
                            error('BuildTree:MaxDepth', ...
                                'Reached max depth with [%s\b] remaining.', ...
                                sprintf('%s ', labelNames{labels(rightRemaining)}));
                        end
                    end
                end
            end
        end
        
    end % buildTree()







end % TrainProbes()