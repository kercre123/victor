function [trainingState] = ExtractProbeValues(varargin)

% TODO: Use parameters from a derived class below, instead of VisionMarkerTrained.*

%% Parameters
markerImageDir = VisionMarkerTrained.TrainingImageDir;
negativeImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/negativeExamplePatches';
maxNegativeExamples = inf;
workingResolutions = VisionMarkerTrained.ProbeParameters.GridSize;
%maxSamples = 100;
%minInfoGain = 0;
%addRotations = false;
addInversions = true;
numPerturbations = 100;
perturbationType = 'normal'; % 'normal' or 'uniform'
blurSigmas = [0 .005 .01]; % as a fraction of the image diagonal
imageSizes = [32 64 128];
perturbSigma = 1; % 'sigma' in 'normal' mode, half-width in 'uniform' mode
saveTree = true;

probeRegion = VisionMarkerTrained.ProbeRegion;

% Now using unpadded images to train
%imageCoords = [-VisionMarkerTrained.FiducialPaddingFraction ...
%    1+VisionMarkerTrained.FiducialPaddingFraction];

DEBUG_DISPLAY = false;

parseVarargin(varargin{:});

t_start = tic;

if ~iscell(markerImageDir)
    markerImageDir = {markerImageDir};
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

savedStateFile = fullfile(fileparts(mfilename('fullpath')), 'trainingState.mat');


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
numBlurs = length(blurSigmas);
numSizes = length(imageSizes);

numResolutions = length(workingResolutions);
probeValues   = cell(numResolutions,1);%numBlurs,numImages);
gradMagValues = cell(numResolutions,1);%numBlurs,numImages);
labels        = cell(numSizes,numBlurs,numImages);
xgrid         = cell(numResolutions,1);
ygrid         = cell(numResolutions,1);
resolutions   = cell(numResolutions,1);

pBar.set_message(sprintf('Computing %d perturbed probe locations at %d resolutions', numPerturbations, numResolutions));
pBar.set_increment(1/(numPerturbations*numResolutions));
pBar.set(0);
    
X = cell(numResolutions,1);
Y = cell(numResolutions,1);
xPerturb = cell(numResolutions, numPerturbations);
yPerturb = cell(numResolutions, numPerturbations);
    
for iRes = 1:numResolutions
    probeValues{iRes} = cell(numSizes,numBlurs,numImages);
    gradMagValues{iRes} = cell(numSizes,numBlurs,numImages);
    
    workingResolution = workingResolutions(iRes);
    resolutions{iRes} = iRes*ones(workingResolution^2,1);
    
    sigma = perturbSigma/workingResolution;
    
    [xgrid{iRes},ygrid{iRes}] = meshgrid(linspace(probeRegion(1),probeRegion(2),workingResolution)); %1:workingResolution);
    xgrid{iRes} = xgrid{iRes}(:);
    ygrid{iRes} = ygrid{iRes}(:);
    
    probePattern = VisionMarkerTrained.ProbePattern(iRes);
    X{iRes} = probePattern.x(ones(workingResolution^2,1),:) + xgrid{iRes}*ones(1,length(probePattern.x));
    Y{iRes} = probePattern.y(ones(workingResolution^2,1),:) + ygrid{iRes}*ones(1,length(probePattern.y));
    
    for iPerturb = 1:numPerturbations
      switch(perturbationType)
        case 'normal'
          perturbation = max(-3*sigma, min(3*sigma, sigma*randn(4,2)));
        case 'uniform'
          perturbation = 2*sigma*rand(4,2) - sigma;
        otherwise
          error('Unrecognized perturbationType "%s".', perturbationType);
      end
      
      corners_i = corners + perturbation;
      T = cp2tform(corners_i, corners, 'projective');
      [xPerturb{iRes, iPerturb}, yPerturb{iRes, iPerturb}] = tforminv(T, X{iRes}, Y{iRes});
      
      pBar.increment();
    end
end
    
% Compute the perturbed probe values
pBar.set_message(sprintf(['Interpolating perturbed probe locations ' ...
    'from %d images at %d sizes and %d blurs'], numImages, numSizes, numBlurs));
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
    
    [~,labelNames{iImg}] = fileparts(fnames{iImg});
    
    [nrows,ncols,~] = size(img);
    
    for iBlur = 1:numBlurs
      imgBlur = img;
      if blurSigmas(iBlur) > 0
        blurSigma = blurSigmas(iBlur)*sqrt(nrows^2 + ncols^2);
        imgBlur = separable_filter(imgBlur, gaussian_kernel(blurSigma));
      end
      
      imgBlur = single(imgBlur);
      
      for iSize = 1:numSizes
        imgResized = imresize(imgBlur, imageSizes(iSize)*[1 1], 'bilinear');
        imgGradMag = single(smoothgradient(imgResized));
        
        [nrows,ncols,~] = size(imgResized);
        imageCoordsX = linspace(0, 1, ncols);
        imageCoordsY = linspace(0, 1, nrows);
        
        for iRes = 1:numResolutions
          workingResolution = workingResolutions(iRes);
          probeValues{iRes}{iSize,iBlur,iImg} = zeros(workingResolution^2, numPerturbations, 'single');
          gradMagValues{iRes}{iSize,iBlur,iImg} = zeros(workingResolution^2, numPerturbations, 'single');
          for iPerturb = 1:numPerturbations
            probeValues{iRes}{iSize,iBlur,iImg}(:,iPerturb) = mean(interp2(imageCoordsX, imageCoordsY, imgResized, ...
              xPerturb{iRes,iPerturb}, yPerturb{iRes,iPerturb}, 'linear', 1), 2);
            
            gradMagValues{iRes}{iSize,iBlur,iImg}(:,iPerturb) = mean(interp2(imageCoordsX, imageCoordsY, imgGradMag, ...
              xPerturb{iRes,iPerturb}, yPerturb{iRes,iPerturb}, 'linear', 0), 2);
          end
          
        end
        
        labels{iSize,iBlur,iImg} = iImg*ones(1,numPerturbations, 'uint32');
        
      end % FOR each blurSigma
    end % FOR each imageSize
    
    pBar.increment();
end
    
% Stack all blurs/perturbations horizontally
for iRes = 1:numResolutions
    probeValues{iRes} = [probeValues{iRes}{:}];
    gradMagValues{iRes} = [gradMagValues{iRes}{:}];
end

% Stack all resolutions vertically
probeValues = vertcat(probeValues{:});
gradMagValues = vertcat(gradMagValues{:});

labels = [labels{:}];

averagePerturbations = false;
if averagePerturbations
    labelIndex = idMap2Index(labels);
    
    probeValues = cellfun(@(x)mean(probeValues(:,x),2), labelIndex, 'UniformOutput', false);
    gradMagValues = cellfun(@(x)mean(gradMagValues(:,x),2), labelIndex, 'UniformOutput', false);
    probeValues = [probeValues{:}];
    gradMagValues = [gradMagValues{:}];
    labels = 1:numImages;
    
    assert(size(probeValues,2) == numImages);
    
    % Debug:
    numCols = ceil(sqrt(numImages));
    numRows = ceil(numImages/numCols);
    for iImg = 1:numImages
        subplot(numRows,numCols,iImg)
        imagesc(reshape(probeValues(:,iImg),workingResolution*[1 1])), axis image
    end
    colormap gray
    
end

% Add negative examples
fnamesNeg = {'ALLWHITE'; 'ALLBLACK'};
if ~isempty(negativeImageDir)
    fprintf('Reading negative image data...');
    fnamesNeg = [fnamesNeg; getfnames(negativeImageDir, 'images', 'useFullPath', true)];
    subDirs = getdirnames(negativeImageDir, 'chunk*');
    numSubDirs = length(subDirs);
    if numSubDirs > 0
        fnamesSubDirs = cell(numSubDirs,1);
        for iSubDir = 1:length(subDirs)
            fnamesSubDirs{iSubDir} = getfnames(fullfile(negativeImageDir, subDirs{iSubDir}), 'images', 'useFullPath', true);
        end
        fnamesNeg = [fnamesNeg; vertcat(fnamesSubDirs{:})];
        clear fnamesSubDirs
    end
    fprintf('Done. (Found %d negative examples.)\n', length(fnamesNeg));
end

numNeg = min(length(fnamesNeg), maxNegativeExamples);

pBar.set_message(sprintf(['Interpolating probe locations ' ...
    'from %d negative images'], numNeg));
pBar.set_increment(1/numNeg);
pBar.set(0);

probeValuesNeg = cell(numResolutions,1);
gradMagValuesNeg = cell(numResolutions,1);
for iRes = 1:numResolutions
    workingResolution = workingResolutions(iRes);
    probeValuesNeg{iRes}   = zeros(workingResolution^2, numNeg, 'single');
    gradMagValuesNeg{iRes} = zeros(workingResolution^2, numNeg, 'single');
end

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
    
    for iRes = 1:numResolutions
        probeValuesNeg{iRes}(:,iImg) = mean(interp2(imageCoordsX, imageCoordsY, imgNeg, ...
            X{iRes}, Y{iRes}, 'linear', 1), 2);
        
        gradMagValuesNeg{iRes}(:,iImg) = mean(interp2(imageCoordsX, imageCoordsY, imgGradMag, ...
            X{iRes}, Y{iRes}, 'linear', 0), 2);
    end
    
    pBar.increment();
end % for each negative example

probeValuesNeg = vertcat(probeValuesNeg{:});
gradMagValuesNeg = vertcat(gradMagValuesNeg{:});

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
% assert(max(labels) == length(labelNames));

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

trainingState.probeValues   = probeValues;
trainingState.gradMagValues = gradMagValues;
trainingState.fnames        = fnames;
trainingState.labels        = labels;
trainingState.labelNames    = labelNames;
trainingState.numImages     = numImages;
trainingState.xgrid         = vertcat(xgrid{:});
trainingState.ygrid         = vertcat(ygrid{:});
trainingState.resolution    = vertcat(resolutions{:});

fprintf('Probe extraction took %.2f seconds (%.1f minutes)\n', toc(t_start), toc(t_start)/60);

% Only save what we need to re-run (not all the settings!)
fprintf('Saving state...');
save(savedStateFile, '-struct', 'trainingState');
fprintf('Done.\n');

end % FUNCTION ExtractProbeValues

