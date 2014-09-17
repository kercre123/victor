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
    
    entireTic = tic();
    
    blurSigmas = [0, .005, .01, .02]; % as a fraction of the image diagonal
    maxPerturbPercent = 0.05;
    numPerturbations = 100;
    probeLocationsX = ((1:30) - .5) / 30; % Probe location assume the left edge of the image is 0 and the right edge is 1
    probeLocationsY = ((1:30) - .5) / 30;
    probeResolutions = [128,32];
    numPadPixels = 100;
    showProbePermutations = false;
    numThreads = 4;
    
    parseVarargin(varargin{:});
    
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
    
    disp(sprintf('Computing %d perturbs, %d blurs, and %d resolutions.', numPerturbations, length(blurSigmas), length(probeResolutions)))
    
    % Compute the perturbed probe values
    cQueue = 1;
    workQueue = cell(length(classesList) * length(classesList(iClass).filenames), 1);
    for iClass = 1:length(classesList)
        labelNames{iClass} = classesList(iClass).labelName;
        for iFile = 1:length(classesList(iClass).filenames)
            curWorkItem.iClass = iClass;
            curWorkItem.iFile = iFile;
            workQueue{cQueue} = curWorkItem;
            cQueue = cQueue + 1;
        end % for iFile = 1:length(classesList(iClass).filenames)
    end % for iClass = 1:length(classesList)
    
    parameters.xPerturb = xPerturb;
    parameters.yPerturb = yPerturb;
    parameters.classesList = classesList;
    parameters.numBlurs = numBlurs;
    parameters.numResolutions = numResolutions;
    parameters.numPerturbations = numPerturbations;
    parameters.numPadPixels = numPadPixels;
    parameters.blurSigmas = blurSigmas;
    parameters.probeResolutions = probeResolutions;
    parameters.numFeatures = length(probeLocationsXGrid);

    if numThreads == 1
        [labels, featureValues] = decisionTree2_extractFeatures_worker(workQueue, parameters);
    else % if numThreads == 1
        
        if ispc()
            temporaryDirectory = 'c:/tmp/';
        elseif ismac()
            temporaryDirectory = '~/tmp/';
            
%             % Create a ramdisk
%             
%             numMegabytes = round( 10 + 1.1 * ceil((numel(featureValues) + numel(labels) * 4) / (1024*1024)) );
%             
%             featureValues = zeros(length(probeLocationsXGrid), numLabels, 'uint8');
%             
%             system('diskutil unmount /Volumes/MatlabRamDisk');
%             
%             command = ['diskutil erasevolume HFS+ ''MatlabRamDisk'' `hdiutil attach -nomount ram://', sprintf('%d', numMegabytes*2048), '`'];
%             
%             system(command);
%             
%             temporaryDirectory = '/Volumes/MatlabRamDisk/';
        end
        
        allInputFilename = [temporaryDirectory, '/extractFeaturesAllInput.mat'];
        outputFilenamePattern = [temporaryDirectory, '/extractFeaturesAllOutput%d.mat'];
        
        savefast(allInputFilename, 'parameters');
        
        matlabCommandString = ['disp(''Loading input...''); load(''', allInputFilename, '''); disp(''Input loaded''); ' , '[localLabels, localFeatureValues] = decisionTree2_extractFeatures_worker(localWorkQueue, parameters); savefast(sprintf(''', outputFilenamePattern,''', iThread), ''localLabels'', ''localFeatureValues'');'];
        
        runParallelProcesses(numThreads, workQueue, temporaryDirectory, matlabCommandString, false);
        
        cLabel = 1;
        for iThread = 0:(numThreads-1)
            outputFilename = sprintf(outputFilenamePattern, iThread);
            
            load(outputFilename);
            delete(outputFilename);
            
            numLocalLabels = length(localLabels);
            
            labels(cLabel:(cLabel+numLocalLabels-1)) = localLabels;
            featureValues(:, cLabel:(cLabel+numLocalLabels-1)) = localFeatureValues;
            
            cLabel = cLabel + numLocalLabels;
        end
        
        if ismac()
%             pause(2);
%             system('diskutil unmount /Volumes/MatlabRamDisk');
            delete(allInputFilename);
        else
            delete(allInputFilename);
        end
        
    end % if numThreads == 1 ... else
    
    disp(sprintf('Completed %d perturbs, %d blurs, and %d resolutions in %f seconds.', numPerturbations, length(blurSigmas), length(probeResolutions), toc(entireTic)))
    
    %     keyboard
end % decisionTree2_extractFeatures()
