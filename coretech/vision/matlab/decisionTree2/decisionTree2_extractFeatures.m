% function [labels, featureValues] = decisionTree2_extractFeatures(classesList, varargin)

% Load the images specified by the classesList, and generate the
% probe images

% Simple Example:
% clear classesList; classesList(1).labelName = '0_000'; classesList(1).filenames = {'/Users/pbarnum/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/rotated/0_000.png'}; classesList(2).labelName = '0_090'; classesList(2).filenames = {'/Users/pbarnum/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/rotated/0_090.png'};
% [labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'numPerturbations', 1, 'maxPerturbPercent', 0, 'blurSigmas', [0]);

% [labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'blurSigmas', [0, .01], 'numPerturbations', 10, 'probeResolutions', [512,32]);

% Example:
% [labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList);

function [labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, varargin)
    %#ok<*CCAT>
    
    blurSigmas = [0, .005, .01, .02]; % as a fraction of the image diagonal
    maxPerturbPercent = 0.05;
    numPerturbations = 100;
    probeLocationsX = ((1:30) - .5) / 30; % Probe location assume the left edge of the image is 0 and the right edge is 1
    probeLocationsY = ((1:30) - .5) / 30;
    probeResolutions = [128,32];
    numPadPixels = 100;
    showProbePermutations = false;
    
    parseVarargin(varargin{:});
    
    pBar = ProgressBar('decisionTree2_extractFeatures', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    corners = [0 0; 0 1; 1 0; 1 1];
    
    numBlurs = length(blurSigmas);
    numResolutions = length(probeResolutions);
    
    numImages = 0;
    for iClass = 1:length(classesList)
        numImages = numImages + length(classesList(iClass).filenames);
    end
    
    % TODO: add non-marker images
    
    [probeLocationsXGrid,probeLocationsYGrid] = meshgrid(probeLocationsX, probeLocationsY);
    probeLocationsXGrid = probeLocationsXGrid(:);
    probeLocationsYGrid = probeLocationsYGrid(:);
    
    numLabels = numBlurs*numImages*numPerturbations*numResolutions;
    labelNames  = cell(length(classesList), 1);
    
    featureValues = zeros(length(probeLocationsXGrid), numLabels, 'uint8');
    labels      = zeros(numLabels, 1, 'int32');
    
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
    
    pBar.set_message(sprintf('Computing %d perturbs, %d blurs, and %d resolutions', numPerturbations, length(blurSigmas), length(probeResolutions)));
    
    pBar.set_increment(1/numImages);
    pBar.set(0);
    
    cLabel = 1;
    for iClass = 1:length(classesList)
        labelNames{iClass} = classesList(iClass).labelName;
        for iFile = 1:length(classesList(iClass).filenames)
            img = imreadAlphaHelper(classesList(iClass).filenames{iFile}); 

            imgPadded = padarray(img, [numPadPixels,numPadPixels], 255);
            
            [nrows,ncols,~] = size(img);
            
            for iBlur = 1:numBlurs
                imgPaddedAndBlurred = imgPadded;
                
                if blurSigmas(iBlur) > 0
                    blurSigma = blurSigmas(iBlur)*sqrt(nrows^2 + ncols^2);
                    imgPaddedAndBlurred = separable_filter(imgPadded, gaussian_kernel(blurSigma));
                end
                
                for iResolution = 1:numResolutions
                    imgPaddedAndBlurredResized = imresize(imresize(imgPaddedAndBlurred, [probeResolutions(iResolution),probeResolutions(iResolution)]), size(imgPaddedAndBlurred), 'nearest');
                    
                    for iPerturb = 1:numPerturbations
                        % Probe location assume the left edge of the image is 0 and the right edge is 1
                        imageCoordsX = round(xPerturb{iPerturb} * ncols + 0.5 + numPadPixels);
                        imageCoordsY = round(yPerturb{iPerturb} * nrows + 0.5 + numPadPixels);
                        
                        inds = find(imageCoordsX >= 1 & imageCoordsX <= (ncols+2*numPadPixels) & imageCoordsY >= 1 & imageCoordsY <= (nrows+2*numPadPixels));
                        
                        % If the perturbations are too large relative to
                        % the padding, some indexes will be out of bounds.
                        % The solution is to increase the padding.
                        assert(length(inds) == length(imageCoordsX));
                        
                        % Stripe the data per-probe location
                        for iFeature = 1:length(imageCoordsY)
                            featureValues(iFeature, cLabel) = imgPaddedAndBlurredResized(imageCoordsY(iFeature), imageCoordsX(iFeature));
                        end
                        
                        if showProbePermutations
                            imshows({imgPaddedAndBlurredResized((1+numPadPixels):(end-numPadPixels), (1+numPadPixels):(end-numPadPixels)), imresize(reshape(featureValuesArray(cLabel, :), [length(probeLocationsY), length(probeLocationsX)]), size(img), 'nearest')});
                            pause(0.05);
                        end
                        
                        labels(cLabel) = iClass;
                        cLabel = cLabel + 1;
                    end % for iPerturb = 1:numPerturbations
                end % for iResolution = 1:length(numResolutions)
            end % for iBlur = 1:numBlurs
            
            pBar.increment();
        end % for iFile = 1:length(classesList(iClass).filenames)
    end % for iClass = 1:length(classesList)
    %     keyboard
end % decisionTree2_extractFeatures()

function img = imreadAlphaHelper(fname)
    
    [img, ~, alpha] = imread(fname);
    img = mean(im2double(img),3);
    img(alpha < .5) = 1;
    
    threshold = (max(img(:)) + min(img(:)))/2;
    %     img = bwpack(img > threshold);
    img = 255*uint8(img > threshold);
    % img = single(img > threshold);
    
end % imreadAlphaHelper()
