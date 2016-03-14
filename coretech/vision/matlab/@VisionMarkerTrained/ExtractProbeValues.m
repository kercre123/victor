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
exposures  = [0.8 1 1.2];
blackPoint = 15; % assuming [0,255] range, what is unadjusted "black" at normal exposure
whitePoint = 255;
addInversions = true;
numPerturbations = 100;
perturbationType = 'normal'; % 'normal' or 'uniform'
perturbSigma = 1; % 'sigma' in 'normal' mode, half-width in 'uniform' mode
computeGradMag = false;
computeGradMagWeights = false;
saveTree = true;
boxFilterWidth = 0; % for illumination normalization for NearestNeighbor
numNoiseVersions = 1;
noiseVariance = 0;

% Now using unpadded images to train
%imageCoords = [-VisionMarkerTrained.FiducialPaddingFraction ...
%    1+VisionMarkerTrained.FiducialPaddingFraction];

DEBUG_DISPLAY = false;

parseVarargin(varargin{:});

if computeGradMagWeights 
  computeGradMag = true;
end

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
  if isdir(markerImageDir{i_dir})
    fnames{i_dir} = getfnames(markerImageDir{i_dir}, 'images', 'useFullPath', true);
  else 
    % Assume it's a file list
    assert(exist(markerImageDir{i_dir}, 'file')>0, ...
      '%s is not a directory, so assuming it is a file list.', markerImageDir{i_dir});
    fid = fopen(markerImageDir{i_dir}, 'r');
    temp = textscan(fid, '%s', 'CommentStyle', '#', 'Delimiter', '\n');
    fclose(fid)
    fnames{i_dir} = temp{1};
  end
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

numVariations = numNoiseVersions*numBlurs*numSizes*numExposures*numPerturbations*(double(addInversions)+1);
fprintf('Will create %d samples for each of %d images = %d total samples.\n', ...
  numVariations, numImages, numVariations*numImages);

labelNames = cell(1, numImages);
%img = cell(1, numImages);

corners = [0 0; 0 1; 1 0; 1 1];

labels        = cell(numImages, numBlurs, numSizes, numNoiseVersions, numExposures);

probeValues = cell(numImages, numBlurs, numSizes, numNoiseVersions, numExposures, numPerturbations, double(addInversions)+1);
if computeGradMag
  gradMagValues = cell(numImages, numBlurs, numSizes, numExposures, numPerturbations, double(addInversions)+1);
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

[xgrid,ygrid] = VisionMarkerTrained.GetProbeGrid(workingResolution);
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
    
    %[nrowsOrig,ncolsOrig,~] = size(imgOrig);
    
    % numBlurs, numSizes, numExposures, numPerturbations
    for iInvert = 1:(double(addInversions)+1)
      if iInvert == 2
        imgOrig = 1 - imgOrig;
      end
      
        for iSize = 1:numSizes
          
          imgResized = imresize(imgOrig, imageSizes(iSize)*[1 1], 'bilinear');
          
           for iBlur = 1:numBlurs
             imgBlur = single(imgResized);
             
             blurSigma = blurSigmas(iBlur);
             if blurSigma > 0
               imgBlur = separable_filter(imgBlur, gaussian_kernel(blurSigma*sqrt(2)*imageSizes(iSize)));
             end
             
             for iNoise = 1:numNoiseVersions
               
               if noiseVariance > 0
                 imgNoisy = imnoise(imgBlur, 'speckle', noiseVariance);
               else
                 imgNoisy = imgBlur;
               end
               
               for iExp = 1:numExposures
                 img = min(1, ((whitePoint-blackPoint)/255*imgNoisy+blackPoint/255)*exposures(iExp));
                 
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
                   
                   probeValues{iImg, iBlur, iSize, iNoise, iExp, iPerturb, iInvert} = im2uint8(mean(interp2(imageCoordsX, imageCoordsY, img, ...
                     xPerturb, yPerturb, 'linear', 1), 2));
                   
                   if computeGradMag
                     imgGradMag = single(smoothgradient(img));
                     gradMagValues{iImg, iBlur, iSize, iNoise, iExp, iPerturb, iInvert} = im2uint8(mean(interp2(imageCoordsX, imageCoordsY, imgGradMag, ...
                       xPerturb, yPerturb, 'linear', 0), 2));
                   end
                   
                   labels{iImg, iBlur, iSize, iNoise, iExp, iPerturb, iInvert} = uint32(iImg + (iInvert-1)*numImages);
                   
                 end % FOR each perturbation
                 
                 %labels{iImg, iBlur, iSize, iExp} = iImg*ones(1,numPerturbations, 'uint32');
                 
               end % FOR each exposure
             
             end % FOR each noise version

           end % FOR each blur
           
        end % FOR each size
        
      
    end % FOR inversion
        
    pBar.increment();
    if pBar.cancelled
      disp('User cancelled.');
      return;
    end
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
    
    numCols = ceil(sqrt(numImages));
    numRows = ceil(numImages/numCols);
    for iImg = 1:numImages
        subplot(numRows,numCols,iImg)
        imagesc(reshape(probeValues(:,iImg),workingResolution*[1 1])), axis image
    end
    colormap gray
    
end

% Add negative examples
fnamesNeg = {};
if ~isempty(negativeImageDir) && maxNegativeExamples > 0
  fprintf('Reading negative image data...');
  if ~iscell(negativeImageDir)
    negativeImageDir = {negativeImageDir};
  end
  
  numNegDirs = length(negativeImageDir);
  fnamesNeg = cell(numNegDirs,1);
  for iNegDir = 1:numNegDirs
    fnamesNeg{iNegDir} = getfnames(negativeImageDir{iNegDir}, 'images', 'useFullPath', true);
    subDirs = getdirnames(negativeImageDir{iNegDir}, 'chunk*');
    numSubDirs = length(subDirs);
    if numSubDirs > 0
      
      fnamesSubDirs = cell(numSubDirs,1);
      for iSubDir = 1:length(subDirs)
        fnamesSubDirs{iSubDir} = getfnames(fullfile(negativeImageDir, subDirs{iSubDir}), 'images', 'useFullPath', true);
        
      end
      fnamesNeg{iNegDir} = [fnamesNeg{iNegDir}; vertcat(fnamesSubDirs{:})];
      
      clear fnamesSubDirs
    end
    
    fnamesNeg = vertcat(fnamesNeg{:});
  end
  fprintf('Done. (Found %d negative examples.)\n', length(fnamesNeg));
end
fnamesNeg = [{'ALLWHITE'; 'ALLBLACK'}; fnamesNeg];
numNeg = min(length(fnamesNeg), max(2,maxNegativeExamples));

pBar.set_message(sprintf(['Interpolating probe locations ' ...
    'from %d negative images'], numNeg));
pBar.set_increment(1/numNeg);
pBar.set(0);

probeValuesNeg = zeros(workingResolution^2, numNeg*(double(addInversions)+1), 'single');
if computeGradMag
  gradMagValuesNeg = zeros(workingResolution^2, numNeg*(double(addInversions)+1), 'single');
end

if addInversions
    labelNames = [labelNames cellfun(@(name)['INVERTED_' name], labelNames, 'UniformOutput', false)];
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
    
    if computeGradMag
      imgGradMag = single(smoothgradient(imgNeg));
      gradMagValuesNeg(:,iImg) = mean(interp2(imageCoordsX, imageCoordsY, imgGradMag, ...
        X, Y, 'linear', 0), 2);
    end
    
    for iInvert = 1:(double(addInversions)+1)  
      if iInvert == 2
        imgNeg = 1 - imgNeg;
        
        probeValuesNeg(:,iImg + numNeg) = mean(interp2(imageCoordsX, imageCoordsY, 1 - imgNeg, ...
          X, Y, 'linear', 1), 2);
        
        if computeGradMag
          % No need to recompute this for negated image
          gradMagValuesNeg(:,iImg + numNeg) = gradMagValuesNeg(:,iImg);
        end
      else 
        probeValuesNeg(:,iImg) = mean(interp2(imageCoordsX, imageCoordsY, imgNeg, ...
          X, Y, 'linear', 1), 2);    
      end
    end % FOR inversion
    
    pBar.increment();
    if pBar.cancelled
      disp('User cancelled.');
      return;
    end
end % for each negative example

allWhite = all(probeValuesNeg==1,1);
allBlack = all(probeValuesNeg==0,1);
noInfo = allWhite | allBlack;

% Keep a single all-white and all-black example
noInfo(find(allWhite,1)) = false;
noInfo(find(allBlack,1)) = false;

if any(noInfo)
    fprintf('Ignoring %d negative patches with no valid info.\n', sum(noInfo));
    numNeg = numNeg - sum(noInfo);
    probeValuesNeg(:,noInfo) = [];
    if computeGradMag
      gradMagValuesNeg(:,noInfo) = [];
    end
    %fnamesNeg(noInfo) = [];
end

%fnames = [fnames; fnamesNeg];
probeValues = [probeValues probeValuesNeg];
if computeGradMag
  gradMagValues = [gradMagValues gradMagValuesNeg];
end

labelNames{end+1} = 'INVALID';
labels = [labels length(labelNames)*ones(1,size(probeValuesNeg,2),'uint32')];


assert(size(probeValues,2) == length(labels));
% assert(max(labels) == length(labelNames));

%numLabels = numImages;
numImages = length(labels);

% Normalize by removing local variation
if boxFilterWidth > 0
  kernel = -ones(boxFilterWidth);
  mid = floor(boxFilterWidth/2);
  kernel(mid,mid) = boxFilterWidth^2 - 1;
  
  probeValues = imfilter(im2double(reshape(single(probeValues),workingResolution,workingResolution,[])), kernel, 'replicate');
  probeValues = reshape(probeValues,workingResolution^2,[]);

  mins = min(probeValues,[],1);
  maxes = max(probeValues,[],1);
  probeValues = im2uint8((probeValues - mins(ones(workingResolution^2,1),:))./(ones(workingResolution^2,1)*(maxes-mins)));
end


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
  if computeGradMagWeights 
    gradMagWeights = im2single(gradMagValues);
    minVal = min(gradMagWeights(:));
    maxVal = max(gradMagWeights(:));
    trainingState.gradMagWeights = 1 - (gradMagWeights-minVal)/(maxVal-minVal);
  end
end
trainingState.fnames        = fnames;
trainingState.labels        = labels;
trainingState.labelNames    = labelNames;
trainingState.numImages     = numImages;
trainingState.xgrid         = xgrid;
trainingState.ygrid         = ygrid;

trainingState.parameters    = varargin;

fprintf('Probe extraction took %.2f seconds (%.1f minutes)\n', toc(t_start), toc(t_start)/60);

% Only save what we need to re-run (not all the settings!)
fprintf('Saving state...');
save(savedStateFile, '-struct', 'trainingState');
fprintf('Done.\n');

end % FUNCTION ExtractProbeValues

