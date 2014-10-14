function [trainingState] = ExtractProbeValues(varargin)

% TODO: Use parameters from a derived class below, instead of VisionMarkerTrained.*

%% Parameters
markerImageDir = VisionMarkerTrained.TrainingImageDir;
negativeImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/negativeExamplePatches_old';
maxNegativeExamples = inf;
workingResolution = VisionMarkerTrained.ProbeParameters.GridSize;
%maxSamples = 100;
%minInfoGain = 0;
%addRotations = false;
blurSigmas = [0 .01];
imageSizes = [20 80];
exposures = [0.8 1 1.2];
blackPoint = 15; % assuming [0,255] range, what is unadjusted "black" at normal exposure
addInversions = true;
numPerturbations = 100;
perturbationType = 'normal'; % 'normal' or 'uniform'
perturbSigma = 1; % 'sigma' in 'normal' mode, half-width in 'uniform' mode
computeGradMag = false;
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

numImages    = length(fnames);
numBlurs     = length(blurSigmas);
numSizes     = length(imageSizes);
numExposures = length(exposures);

fprintf('Will create %d samples for each of %d images = %d total samples.\n', ...
  numBlurs*numSizes*numExposures*numPerturbations, numImages, ...
  numBlurs*numSizes*numExposures*numPerturbations*numImages);

labelNames = cell(1, numImages);
%img = cell(1, numImages);

corners = [0 0; 0 1; 1 0; 1 1];

labels        = cell(numImages, numBlurs, numSizes, numExposures);

probeValues = cell(numImages, numBlurs, numSizes, numExposures, numPerturbations);
if computeGradMag
  gradMagValues = cell(numImages, numBlurs, numSizes, numExposures, numPerturbations);
end

perturbSigma = perturbSigma/workingResolution;

if strcmp(perturbationType, 'shift')
  % Note that i would normally use numPerturbations+1 here
  % but I'm removing one for iPerturb==1, so it's really
  % doing numPerturbations+1-1
  angle = linspace(0, 2*pi, numPerturbations);
  xShift = [0 perturbSigma*cos(angle(1:end-1))];
  yShift = [0 perturbSigma*sin(angle(1:end-1))];
end

[xgrid,ygrid] = meshgrid(linspace(probeRegion(1),probeRegion(2),workingResolution)); %1:workingResolution);
xgrid = xgrid(:);
ygrid = ygrid(:);

probePattern = VisionMarkerTrained.ProbePattern;
X = probePattern.x(ones(workingResolution^2,1),:) + xgrid*ones(1,length(probePattern.x));
Y = probePattern.y(ones(workingResolution^2,1),:) + ygrid*ones(1,length(probePattern.y));
   
    
% Compute the perturbed probe values
pBar.set_message(sprintf(['Interpolating %d perturbed probe locations ' ...
    'from %d images'], numPerturbations, numImages));
pBar.set_increment(1/numImages);
pBar.set(0);
for iImg = 1:numImages
    
    %         if strcmp(fnames{iImg}, 'ALLWHITE')
    %             img{iImg} = ones(workingResolution);
    %         elseif strcmp(fnames{iImg}, 'ALLBLACK')
    %             img{iImg} = zeros(workingResolution);
    %         else
    imgOrig = imreadAlphaHelper(fnames{iImg});
    %         end
    
    [~,labelNames{iImg}] = fileparts(fnames{iImg});
    
    [nrowsOrig,ncolsOrig,~] = size(imgOrig);
    
    % numBlurs, numSizes, numExposures, numPerturbations
    for iBlur = 1:numBlurs
      imgBlur = single(imgOrig);
      
      blurSigma = blurSigmas(iBlur);
      if blurSigma > 0
        imgBlur = separable_filter(imgBlur, gaussian_kernel(blurSigma*sqrt(nrowsOrig^2 + ncolsOrig^2)));
      end
      
      for iSize = 1:numSizes
        
        imgResized = imresize(imgBlur, imageSizes(iSize)*[1 1], 'bilinear'); 
      
        for iExp = 1:numExposures
          img = min(1, (imgResized+blackPoint/255)*exposures(iExp));
          
          % Same for all perturbations in following loop
          img = single(img);
          [nrows,ncols,~] = size(img);
          imageCoordsX = linspace(0, 1, ncols);
          imageCoordsY = linspace(0, 1, nrows);
            
          for iPerturb = 1:numPerturbations
            
            if strcmp(perturbationType, 'shift')
              xPerturb = X + xShift(iPerturb);
              yPerturb = Y + yShift(iPerturb);
            else
              % Sample a perturbation of the corners
              switch(perturbationType)
                case 'normal'
                  perturbation = max(-3*perturbSigma, min(3*perturbSigma, perturbSigma*randn(4,2)));
                case 'uniform'
                  perturbation = 2*perturbSigma*rand(4,2) - perturbSigma;
                otherwise
                  error('Unrecognized perturbationType "%s".', perturbationType);
              end
              
              corners_i = corners + perturbation;
              T = cp2tform(corners_i, corners, 'projective');
              [xPerturb, yPerturb] = tforminv(T, X, Y);
            end
            
            probeValues{iImg, iBlur, iSize, iExp, iPerturb} = mean(interp2(imageCoordsX, imageCoordsY, img, ...
              xPerturb, yPerturb, 'linear', 1), 2);
            
            if computeGradMag
              imgGradMag = single(smoothgradient(img));
              gradMagValues{iImg, iBlur, iSize, iExp, iPerturb} = mean(interp2(imageCoordsX, imageCoordsY, img, ...
                xPerturb, yPerturb, 'linear', 0), 2);
            end
           
            labels{iImg, iBlur, iSize, iExp, iPerturb} = uint32(iImg);
            
          end % FOR each perturbation
          
          %labels{iImg, iBlur, iSize, iExp} = iImg*ones(1,numPerturbations, 'uint32');
          
        end % FOR each exposure
        
      end % FOR each size
      
    end % FOR each blur
        
    pBar.increment();
    
end % FOR each image
    
% Stack all perturbations horizontally
probeValues   = [probeValues{:}];
if computeGradMag
  gradMagValues = [gradMagValues{:}];
end

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

probeValuesNeg   = zeros(workingResolution^2, numNeg, 'single');
if computeGradMag
  gradMagValuesNeg = zeros(workingResolution^2, numNeg, 'single');
end

for iImg = 1:numNeg
    if strcmp(fnamesNeg{iImg}, 'ALLWHITE')
        imgNeg = ones(workingResolution);
    elseif strcmp(fnamesNeg{iImg}, 'ALLBLACK')
        imgNeg = zeros(workingResolution);
    else
        imgNeg = imreadAlphaHelper(fnamesNeg{iImg});
    end
    
    
    imgNeg = single(imgNeg);
    
    [nrows,ncols,~] = size(imgNeg);
    imageCoordsX = linspace(0, 1, ncols);
    imageCoordsY = linspace(0, 1, nrows);
    
    probeValuesNeg(:,iImg) = mean(interp2(imageCoordsX, imageCoordsY, imgNeg, ...
      X, Y, 'linear', 1), 2);
    
    if computeGradMag
      imgGradMag = single(smoothgradient(imgNeg));
      gradMagValuesNeg(:,iImg) = mean(interp2(imageCoordsX, imageCoordsY, imgGradMag, ...
        X, Y, 'linear', 0), 2);
    end
    
    pBar.increment();
end % for each negative example

noInfo = all(probeValuesNeg == 0 | probeValuesNeg == 1, 1);
if any(noInfo)
    fprintf('Ignoring %d negative patches with no valid info.\n', sum(noInfo));
    numNeg = numNeg - sum(noInfo);
    probeValuesNeg(:,noInfo) = [];
    if computeGradMag
      gradMagValuesNeg(:,noInfo) = [];
    end
    %fnamesNeg(noInfo) = [];
end

labelNames{end+1} = 'INVALID';
labels = [labels length(labelNames)*ones(1,numNeg,'uint32')];
%fnames = [fnames; fnamesNeg];
probeValues = [probeValues probeValuesNeg];
if computeGradMag
  gradMagValues = [gradMagValues gradMagValuesNeg];
end

if addInversions
    % Add white-on-black versions of everything
    labels        = [labels labels+length(labelNames)];
    labelNames    = [labelNames cellfun(@(name)['INVERTED_' name], labelNames, 'UniformOutput', false)];
    probeValues   = [probeValues 1-probeValues];
    if computeGradMag
      gradMagValues = [gradMagValues gradMagValues];
    end
    
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
if computeGradMag
  trainingState.gradMagValues = gradMagValues;
end
trainingState.fnames        = fnames;
trainingState.labels        = labels;
trainingState.labelNames    = labelNames;
trainingState.numImages     = numImages;
trainingState.xgrid         = xgrid;
trainingState.ygrid         = ygrid;

fprintf('Probe extraction took %.2f seconds (%.1f minutes)\n', toc(t_start), toc(t_start)/60);

% Only save what we need to re-run (not all the settings!)
fprintf('Saving state...');
save(savedStateFile, '-struct', 'trainingState');
fprintf('Done.\n');

end % FUNCTION ExtractProbeValues

