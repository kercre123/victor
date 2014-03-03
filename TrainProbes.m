function root = TrainProbes(varargin)

%% Parameters
markerImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/letters';
workingResolution = 50;
probeRadius = 5;
%maxSamples = 100;
minInfoGain = 0;
maxDepth = 20;
addRotations = true;

DEBUG_DISPLAY = true;

parseVarargin(varargin{:});
    

%% Load marker images
fnames = getfnames(markerImageDir, 'images');
%fnames = {'angryFace.png', 'ankiLogo.png', 'batteries3.png', ...
%    'bullseye2.png', 'fire.png', 'squarePlusCorners.png'};

numImages = length(fnames);
labelNames = cell(1,numImages);
img = cell(1, numImages);
for i = 1:numImages
    img{i} = imresize(mean(im2double(imread(fullfile(markerImageDir, fnames{i}))),3), workingResolution*[1 1]);
    img{i} = separable_filter(img{i}, gaussian_kernel(probeRadius));
    [~,labelNames{i}] = fileparts(fnames{i});
end
img = cat(3, img{:});
labels = 1:numImages;

%% Add all four rotations
if addRotations
    img = cat(3, img, imrotate(img,90), imrotate(img,180), imrotate(img, 270));
    labels = repmat(labels, [1 4]);
    
    numImages = 4*numImages;
end

%% Create gradient weight map
% Ix = (image_right(img) - image_left(img))/2;
% Iy = (image_down(img) - image_up(img))/2;
% gradMag = sqrt(Ix.^2 + Iy.^2);


%% Create samples 
[xgrid,ygrid] = meshgrid(1:workingResolution);
probeValues = reshape(img, [], numImages);

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
    

namedFigure('DecisionTree')
DrawTree(root, 0);

probes = GetProbeList(root);

namedFigure('ProbeLocations'), colormap(gray), clf

for i = 1:min(32,numImages)
    subplot(4,min(32,numImages),i), hold off, imagesc(img(:,:,i)), axis image, hold on
    
    fieldName = ['label_' labelNames{labels(i)}];
    if isfield(probes, fieldName)
        usedProbes = probes.(fieldName);
        plot(xgrid(usedProbes), ygrid(usedProbes), 'ro');
    else
        warning('No field for label "%s"', labelNames{labels(i)});
    end
end

correct = false(1,numImages);
for i = 1:numImages
    result = TestTree(root, img(:,:,i));
    if ischar(result)
        correct(i) = strcmp(result, labelNames{labels(i)});
    else
        warning('Reached node with multiple remaining labels: ')
        for j = 1:length(result)
            fprintf('%s, ', labelNames{labels(result(j))});
        end
        fprintf('\b\b\n');
    end
    
end
fprintf('Got %d of %d training images right (%.1f%%)\n', ...
    sum(correct), length(correct), sum(correct)/length(correct)*100);
if sum(correct) ~= length(correct)
    warning('We should have ZERO training error!');
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
            markerProb = max(eps,hist(labels(node.remaining), 1:numImages));
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
            p_on = sum(probeIsOn,2)/size(probeIsOn,2);
            p_off = 1 - p_on;
            
            markerProb_on  = zeros(numProbes,numImages);
            markerProb_off = zeros(numProbes,numImages);
            
            L = labels(node.remaining); 
            for i_probe = 1:numProbes
                %markerProb_on(i_probe,:) = max(eps,hist(L(probeIsOn(i_probe,:)), 1:numImages));
                %markerProb_off(i_probe,:) = max(eps,hist(L(~probeIsOn(i_probe,:)), 1:numImages));
                markerProb_on(i_probe,:) = max(eps, accumarray(L(:), probeIsOn(i_probe,:)', [numImages 1])');
                markerProb_off(i_probe,:) = max(eps, accumarray(L(:), 1-probeIsOn(i_probe,:)', [numImages 1])');
            end
            markerProb_on = markerProb_on ./ (sum(markerProb_on,2)*ones(1,numImages));
            %markerProb_off = 1 - markerProb_on;
            markerProb_off = markerProb_off ./ (sum(markerProb_off,2)*ones(1,numImages));
            
            conditionalEntropy = - (p_on.*sum(markerProb_on.*log2(max(eps,markerProb_on)), 2) + ...
                p_off.*sum(markerProb_off.*log2(max(eps,markerProb_off)), 2));
            
            infoGain = currentEntropy - conditionalEntropy;
            
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
            node.x = (xgrid(node.whichProbe)-1)/workingResolution;
            node.y = (ygrid(node.whichProbe)-1)/workingResolution;
            node.infoGain = infoGain(whichUnusedProbe);
            node.remainingLabels = sprintf('%s ', labelNames{labels(node.remaining)});
            
            % Recurse left and right from this node
            leftRemaining = node.remaining(probeValues(node.whichProbe,node.remaining) < .5);
            if ~isempty(leftRemaining) 
                if length(leftRemaining) == length(node.remaining)
                    error('BuildTree:UselessSplit', ...
                            'Reached split that makes no progress with [%s] remaining.', ...
                            sprintf('%s ', labelNames{labels(node.remaining)}));
                else
                    usedLeft = used;
                    usedLeft(node.whichProbe) = true;
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
            
            rightRemaining = node.remaining(probeValues(node.whichProbe,node.remaining) >= .5);
            if ~isempty(rightRemaining)
                if length(rightRemaining) == length(node.remaining)
                    error('BuildTree:UselessSplit', ...
                        'Reached split that makes no progress with [%s\b] remaining.', ...
                        sprintf('%s ', labelNames{labels(node.remaining)}));
                else
                    usedRight = used;
                    usedRight(node.whichProbe) = true;
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
        
    end % buildTree()







end % TrainProbes()