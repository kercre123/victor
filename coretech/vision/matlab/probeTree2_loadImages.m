% function [labels, probeValues] = TrainProbeTree2_loadImages(fiducialClassesList, varargin)

% Load the images specified by the fiducialClassesList, and generate the
% probe images

% Simple Example:
% clear fiducialClassesList; fiducialClassesList(1).labelName = '0_000'; fiducialClassesList(1).filenames = {'/Users/pbarnum/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/rotated/0_000.png'}; fiducialClassesList(2).labelName = '0_090'; fiducialClassesList(2).filenames = {'/Users/pbarnum/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/rotated/0_090.png'};
% [labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid] = TrainProbeTree2_loadImages(fiducialClassesList, 'numPerturbations', 1, 'maxPerturbPercent', 0, 'blurSigmas', [0]);

% Example:
% [labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid] = TrainProbeTree2_loadImages(fiducialClassesList);



function [labelNames, labels, probeValues, probeLocationsXGrid, probeLocationsYGrid] = TrainProbeTree2_loadImages(fiducialClassesList, varargin)
    %#ok<*CCAT>
    
    blurSigmas = [0 .005 .01]; % as a fraction of the image diagonal
    maxPerturbPercent = 0.05;
    numPerturbations = 100;
    probeLocationsX = linspace(0, 1, 30);
    probeLocationsY = linspace(0, 1, 30);
    
    parseVarargin(varargin{:});
    
    pBar = ProgressBar('VisionMarkerTrained ProbeTree', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    corners = [0 0; 0 1; 1 0; 1 1];
    
    numBlurs = length(blurSigmas);
    
    numImages = 0;
    for iClass = 1:length(fiducialClassesList)
        numImages = numImages + length(fiducialClassesList(iClass).filenames);
    end
    
    % TODO: add non-marker images
    
    [probeLocationsXGrid,probeLocationsYGrid] = meshgrid(probeLocationsX, probeLocationsY);
    probeLocationsXGrid = probeLocationsXGrid(:);
    probeLocationsYGrid = probeLocationsYGrid(:);
    
    %     probeValues   = zeros(numBlurs*numImages*numPerturbations, length(probeLocationsX)*length(probeLocationsY), 'uint8');
    labelNames  = cell(length(fiducialClassesList), 1);
    probeValues = cell(length(probeLocationsXGrid), 1);
    labels      = zeros(numBlurs*numImages*numPerturbations, 1, 'uint32');
    
    for iProbe = 1:length(probeValues)
        probeValues{iProbe} = zeros(numBlurs*numImages*numPerturbations, 1, 'uint8');
    end
    
    % Precompute all the perturbed probe locations once
    xPerturb = cell(1, numPerturbations);
    yPerturb = cell(1, numPerturbations);
    for iPerturb = 1:numPerturbations
        perturbation = (2*rand(4,2) - 1) * maxPerturbPercent;
        corners_i = corners + perturbation;
        T = cp2tform(corners_i, corners, 'projective');
        [xPerturb{iPerturb}, yPerturb{iPerturb}] = tforminv(T, probeLocationsXGrid, probeLocationsYGrid);
        xPerturb{iPerturb} = xPerturb{iPerturb}(:);
        yPerturb{iPerturb} = yPerturb{iPerturb}(:);
    end
    
    % Compute the perturbed probe values
    
    pBar.set_message(sprintf('Computing %d perturbed probe locations', numPerturbations));
    pBar.set_increment(1/numImages);
    pBar.set(0);
    
    cLabel = 1;
    for iClass = 1:length(fiducialClassesList)
        labelNames{iClass} = fiducialClassesList(iClass).labelName;
        for iFile = 1:length(fiducialClassesList(iClass).filenames)
            img = imreadAlphaHelper(fiducialClassesList(iClass).filenames{iFile});
            
            [nrows,ncols,~] = size(img);
            
            imageCoordsX = linspace(0, 1, ncols);
            imageCoordsY = linspace(0, 1, nrows);
            
            for iBlur = 1:numBlurs
                imgBlur = img;
                
                if blurSigmas(iBlur) > 0
                    blurSigma = blurSigmas(iBlur)*sqrt(nrows^2 + ncols^2);
                    imgBlur = separable_filter(img, gaussian_kernel(blurSigma));
                end
                
                for iPerturb = 1:numPerturbations
                    tmpValues = uint8(255*interp2(imageCoordsX, imageCoordsY, imgBlur, xPerturb{iPerturb}, yPerturb{iPerturb}, 'linear', 1));
                    
                    % Stripe the data per-probe location
                    for iProbe = 1:length(probeValues)
                        probeValues{iProbe}(cLabel) = tmpValues(iProbe);
                    end
                    
                    labels(cLabel) = iClass;
                    cLabel = cLabel + 1;
                end % for iPerturb = 1:numPerturbations
            end % for iBlur = 1:numBlurs
            
            pBar.increment();
        end % for iFile = 1:length(fiducialClassesList(iClass).filenames)
    end % for iClass = 1:length(fiducialClassesList)
    
    %     keyboard
end % TrainProbeTree2_loadImages()

function img = imreadAlphaHelper(fname)
    
    [img, ~, alpha] = imread(fname);
    img = mean(im2double(img),3);
    img(alpha < .5) = 1;
    
    threshold = (max(img(:)) + min(img(:)))/2;
    %     img = bwpack(img > threshold);
    %     img = uint8(img > threshold);
    img = single(img > threshold);
    
end % imreadAlphaHelper()
