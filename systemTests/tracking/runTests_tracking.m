% function allCompiledResults = runTests_tracking()

% On PC
% allCompiledResults = runTests_tracking('C:/Anki/products-cozmo-large-files/systemTestsData/trackingScripts/tracking_*.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/results/', , 'z:/Documents/Anki/products-cozmo-large-files/VisionMarkers/');

% On Mac
% allCompiledResults = runTests_tracking('~/Documents/Anki/products-cozmo-large-files/systemTestsData/trackingScripts/tracking_*.json', '~/Documents/Anki/products-cozmo-large-files/systemTestsData/results/', '~/Documents/Anki/products-cozmo-large-files/VisionMarkers/');

function allCompiledResults = runTests_tracking(testJsonPattern, resultsDirectory, visionMarkersDirectory)
    % To be a match, all corners of a quad must be within these thresholds
    maxMatchDistance_pixels = 5;
    maxMatchDistance_percent = 0.06;
    
    numComputeThreads.basics = 1;
    numComputeThreads.perPose = 3;
    
    % If makeNewResultsDirectory is true, make a new directory if runTests_tracking.m is changed. Otherwise, use the last created directory.
%     makeNewResultsDirectory = true;
    makeNewResultsDirectory = false;
    
    runWhichAlgorithms = {...
        %         {'tracking_c'},...
        {'tracking_variableTimes','klt_withSnap'},...
        {'tracking_variableTimes','klt_withoutSnap'},...
        {'tracking_variableTimes','c_withoutSnap'},...
        {'tracking_variableTimes','c_withSnap'},...
        };
    
    assert(exist('testJsonPattern', 'var') == 1);
    assert(exist('resultsDirectory', 'var') == 1);
    assert(exist('visionMarkersDirectory', 'var') == 1);
    
    testJsonPattern = strrep(testJsonPattern, '~/', [tildeToPath(),'/']);
    resultsDirectory = strrep(resultsDirectory, '~/', [tildeToPath(),'/']);
    visionMarkersDirectory = strrep(visionMarkersDirectory, '~/', [tildeToPath(),'/']);
    
    thisFilename = [mfilename('fullpath'), '.m'];
    thisFileChangeTime = dir(thisFilename);
    
    fprintf('Loading json test data...');
    allTestData = getTestData(testJsonPattern);
    fprintf('Loaded\n');
    
    % Compute the accuracy for each test type (matlab-with-refinement, c-with-matlab-quads, etc.), and each set of parameters
    algorithmParameters.normalizeImage = false;

    algorithmParameters.whichAlgorithm = 'c_6dof';
    
    % Initialize the track
    algorithmParameters.init_scaleTemplateRegionPercent  = 0.9;
    algorithmParameters.init_numPyramidLevels            = 3;
    algorithmParameters.init_maxSamplesAtBaseLevel       = 500;
    algorithmParameters.init_numSamplingRegions          = 5;
    algorithmParameters.init_numFiducialSquareSamples    = 500;
    algorithmParameters.init_fiducialSquareWidthFraction = 0.1;
    
    % Update the track with a new image
    algorithmParameters.track_maxIterations                 = 50;
    algorithmParameters.track_convergenceTolerance_angle    = 0.05 * 180 / pi;
    algorithmParameters.track_convergenceTolerance_distance = 0.05;
    algorithmParameters.track_verify_maxPixelDifference     = 30;
    algorithmParameters.track_maxSnapToClosestQuadDistance  = 0; % If >0, snap to the closest quad, and re-initialize
    
    % Only for track_snapToClosestQuad == true
    minSideLength = round(0.01*max(240,320));
    maxSideLength = round(0.97*min(240,320));
    algorithmParameters.detection.useIntegralImageFiltering = true;
    algorithmParameters.detection.scaleImage_thresholdMultiplier = 1.0;
    algorithmParameters.detection.scaleImage_numPyramidLevels = 3;
    algorithmParameters.detection.component1d_minComponentWidth = 0;
    algorithmParameters.detection.component1d_maxSkipDistance = 0;
    algorithmParameters.detection.component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
    algorithmParameters.detection.component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
    algorithmParameters.detection.component_sparseMultiplyThreshold = 1000.0;
    algorithmParameters.detection.component_solidMultiplyThreshold = 2.0;
    algorithmParameters.detection.component_minHollowRatio = 1.0;
    algorithmParameters.detection.quads_minQuadArea = 100 / 4;
    algorithmParameters.detection.quads_minLaplacianPeakRatio = 5;
    algorithmParameters.detection.quads_quadSymmetryThreshold = 2.0;
    algorithmParameters.detection.quads_minDistanceFromImageEdge = 2;
    algorithmParameters.detection.decode_minContrastRatio = 0; % EVERYTHING has contrast >= 1, by definition
    algorithmParameters.detection.refine_quadRefinementMinCornerChange = 0.005;
    algorithmParameters.detection.refine_quadRefinementMaxCornerChange = 2;
    algorithmParameters.detection.refine_numRefinementSamples = 100;
    algorithmParameters.detection.refine_quadRefinementIterations = 25;
    algorithmParameters.detection.useMatlabForAll = false;
    algorithmParameters.detection.useMatlabForQuadExtraction = false;
    algorithmParameters.detection.matlab_embeddedConversions = EmbeddedConversionsManager();
    algorithmParameters.detection.showImageDetectionWidth = 640;
    algorithmParameters.detection.showImageDetections = false;
    algorithmParameters.detection.preprocessingFunction = []; % If non-empty, preprocessing is applied before compression
    algorithmParameters.detection.imageCompression = {'none', 0}; % Applies compression before running the algorithm
    algorithmParameters.detection.cornerMethod = 'laplacianPeaks'; % {'laplacianPeaks', 'lineFits'}
    algorithmParameters.cornerMethod = 'laplacianPeaks'; % {'laplacianPeaks', 'lineFits'}
    
    % Display and compression options
    algorithmParameters.showImageDetectionWidth = 640;
    algorithmParameters.showImageDetections = false;
    algorithmParameters.preprocessingFunction = []; % If non-empty, preprocessing is applied before compression
    algorithmParameters.imageCompression = {'none', 0}; % Applies compression before running the algorithm
    
    % Shake the image horizontally, as it's being tracked
    algorithmParameters.shaking = {'none', 0};
    
    % Change the grayvalue of all pixels + or - this percent
    algorithmParameters.grayvalueShift = {'none', 0};
    
    %     algorithmParameters.track_ = ;
    
    algorithmParametersOrig = algorithmParameters;
    
    %     if length(runWhichAlgorithms) ~= length(unique(runWhichAlgorithms))
    %         disp('There is a repeat');
    %         keyboard
    %     end
    
    resultsDirectory_curTime = makeResultsDirectory(resultsDirectory, thisFileChangeTime, makeNewResultsDirectory);
    
    clear resultsData
    
    for iAlgorithm = 1:length(runWhichAlgorithms)
        algorithmParametersN = algorithmParametersOrig;
        algorithmParametersN.extractionFunctionName = runWhichAlgorithms{iAlgorithm};
        isSimpleTest = true;
        
        if strcmp(runWhichAlgorithms{iAlgorithm}{1}, 'tracking_c')
            % Default parameters
        elseif strcmp(runWhichAlgorithms{iAlgorithm}{1}, 'tracking_variableTimes')
            isSimpleTest = false;
            
            %numShakingPixels = [0, 2, 8, 12];
            %pixelGravaluePercentDifferences = [0.0, 0.05, 0.1, 0.2, 0.3, 0.5];
            
            numShakingPixels = [0, 2];
            pixelGravaluePercentDifferences = [0.0, 0.1];
            
            temporalFrameFractions = [1, 10];
            
            percentMarkersCorrect_window = cell(length(numShakingPixels), length(pixelGravaluePercentDifferences), length(temporalFrameFractions), 1);
            
            for iShake = 1:length(numShakingPixels)
                for iGrayvaluePercent = 1:length(pixelGravaluePercentDifferences)
                    % Note: figuring out change time is tricky, so this reruns everything every time
                    for iMaxFraction = 1:length(temporalFrameFractions)
                        maxFraction = temporalFrameFractions(iMaxFraction);

                        percentMarkersCorrect_window{iShake}{iGrayvaluePercent}{iMaxFraction} = zeros(length(allTestData), maxFraction);

                        for iPiece = 1:maxFraction
                            for iTest = 1:length(allTestData)
                                numPoses = length(allTestData{iTest}.jsonData.Poses);
                                frameIndexes = round(linspace(0, numPoses, maxFraction+1));
                                startIndexes = 1 + frameIndexes(1:(end-1));
                                endIndexes = frameIndexes(2:end);

                                curAllTestData = allTestData(iTest);
                                curAllTestData{1}.jsonData.Poses = allTestData{iTest}.jsonData.Poses(startIndexes(iPiece):endIndexes(iPiece));

                                algorithmParametersN = algorithmParametersOrig;
                                algorithmParametersN.extractionFunctionName = sprintf('tracking_variableTimes%s/test%d/grayvalue%0.2f/shake%d/fraction%d/piece%d', runWhichAlgorithms{iAlgorithm}{2}, iTest, pixelGravaluePercentDifferences(iGrayvaluePercent), numShakingPixels(iShake), maxFraction, iPiece);
                                algorithmParametersN.shaking = {'horizontal', numShakingPixels(iShake)};
                                algorithmParametersN.grayvalueShift = {'uniformPercent', pixelGravaluePercentDifferences(iGrayvaluePercent)};

                                if strcmp(runWhichAlgorithms{iAlgorithm}{2}, 'c_withoutSnap')
                                    % The rest of the parameters are defaults
                                elseif strcmp(runWhichAlgorithms{iAlgorithm}{2}, 'c_withSnap')
                                    algorithmParametersN.track_maxSnapToClosestQuadDistance = 4*5;
                                elseif strcmp(runWhichAlgorithms{iAlgorithm}{2}, 'klt_withoutSnap')
                                    algorithmParametersN.whichAlgorithm = 'matlab_klt';
                                elseif strcmp(runWhichAlgorithms{iAlgorithm}{2}, 'klt_withSnap')
                                    algorithmParametersN.whichAlgorithm = 'matlab_klt';
                                    algorithmParametersN.track_maxSnapToClosestQuadDistance = 4*5;
                                else
                                    keyboard
                                    assert(false);
                                end

                                if length(curAllTestData{1}.jsonData.Poses) > 10
                                    localNumComputeThreads.basics = numComputeThreads.basics;
                                    localNumComputeThreads.perPose = numComputeThreads.perPose;
                                else
                                    localNumComputeThreads.basics = 1;
                                    localNumComputeThreads.perPose = 1;
                                end

                                curResults = compileAll(algorithmParametersN, visionMarkersDirectory, resultsDirectory, curAllTestData, localNumComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, thisFileChangeTime, false);

                                percentMarkersCorrect_window{iShake}{iGrayvaluePercent}{iMaxFraction}(iTest, iPiece) = curResults.percentMarkersCorrect;
                            end % for iTest = 1:length(allTestData)
                        end % for iPiece = 1:maxFraction
                    end % for iMaxFraction = 1:length(temporalFrameFractions)
                    
                    markersCorrectImage = zeros(length(allTestData), length(temporalFrameFractions) - 1 + sum(temporalFrameFractions), 3);
                    markersCorrectImage(:,:,1) = 255;

                    cx = 1;
                    for iMaxFraction = 1:length(temporalFrameFractions)
                        curResults = percentMarkersCorrect_window{iShake}{iGrayvaluePercent}{iMaxFraction};

                        markersCorrectImage(:, (cx):(cx+size(curResults,2)-1), :) = repmat(curResults, [1,1,3]);

                        cx = cx + size(curResults,2) + 1;
                    end

                    newSize = round([120, size(markersCorrectImage,2)*120/size(markersCorrectImage,1)]);
                    markersCorrectImageToShow = imresize(markersCorrectImage, newSize, 'nearest');

                    %figureHandle = figure(10 + iShake);
                    %set(figureHandle, 'Name', sprintf('Shake%d', numShakingPixels(iShake)));

                    if strcmp(runWhichAlgorithms{iAlgorithm}{2}, 'c_withoutSnap')
                        figureHandle = figure(100);
                    elseif strcmp(runWhichAlgorithms{iAlgorithm}{2}, 'c_withSnap')
                        figureHandle = figure(110);
                    elseif strcmp(runWhichAlgorithms{iAlgorithm}{2}, 'klt_withoutSnap')
                        figureHandle = figure(120);
                    elseif strcmp(runWhichAlgorithms{iAlgorithm}{2}, 'klt_withSnap')
                        figureHandle = figure(130);
                    end
                    
                    set(figureHandle, 'Name', runWhichAlgorithms{iAlgorithm}{2});

                    %numX = ceil(sqrt(length(numShakingPixels)));
                    %numY = ceil(length(numShakingPixels) / numX);
                    
                    numX = length(pixelGravaluePercentDifferences);
                    numY = length(numShakingPixels);
                    subplot(numY, numX, (iGrayvaluePercent-1) + numX*(iShake-1) + 1);
                    imshow(markersCorrectImageToShow);
                    title(sprintf('Shake%d Gray%0.2f', numShakingPixels(iShake), pixelGravaluePercentDifferences(iGrayvaluePercent)));
                end % for iGrayvaluePercent = 1:length(pixelGravaluePercentDifferences)
            end % for iShake = 1:length(numShakingPixels)
        else
            % Unknown runWhichAlgorithms{iAlgorithm}
            keyboard
            assert(false);
        end
        
        % If it's a simple test (without loops or whatnot)
        if isSimpleTest
            curResults = compileAll(algorithmParametersN, visionMarkersDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, thisFileChangeTime, false);
            resultsData.(runWhichAlgorithms{iAlgorithm}) = curResults;
            disp(sprintf('Results for %s percentQuadsExtracted:%f percentMarkersCorrect:%f numMarkerErrors:%d fileSizePercent:%f', runWhichAlgorithms{iAlgorithm}, curResults.percentQuadsExtracted, curResults.percentMarkersCorrect, curResults.numMarkerErrors, curResults.compressedFileSizeTotal / curResults.uncompressedFileSizeTotal));
        end
    end % for iAlgorithm = 1:length(runWhichAlgorithms)
    
    if ~exist('resultsDirectory_curTime', 'var')
        keyboard;
    end
    
    allResultsFilename = strrep(resultsDirectory_curTime, 'results/dateTime', 'resultsAt');
    allResultsFilename = [allResultsFilename(1:(end-1)), '.mat'];
    save(allResultsFilename, '*');
    
    keyboard
    
    allCompiledResults = resultsData;
end % runTests_tracking()

function allTestData = getTestData(testJsonPattern)
    testJsonPattern = strrep(testJsonPattern, '\', '/');
    slashIndexes = strfind(testJsonPattern, '/');
    testPath = testJsonPattern(1:(slashIndexes(end)));
    
    allTestFilenamesRaw = dir(testJsonPattern);
    
    allTestData = cell(length(allTestFilenamesRaw), 1);
    
    for iTest = 1:length(allTestFilenamesRaw)
        allTestFilename = [testPath, allTestFilenamesRaw(iTest).name];
        
        % Load json file
        allTestData{iTest} = loadJsonTestFile(allTestFilename);
    end
end % getTestFilenames()

function resultsData_overall = compileAll(algorithmParameters, visionMarkersDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, thisFileChangeTime, showPlots)
    compileAllTic = tic();

    markerDirectoryList = {
        [visionMarkersDirectory, '/symbols/withFiducials/'],...
        [visionMarkersDirectory, '/letters/withFiducials/'],...
        [visionMarkersDirectory, '/dice/withFiducials/']};
    
    
    rotationList = getListOfSymmetricMarkers(markerDirectoryList);
    
    [workQueue_basicStats_current, workQueue_basicStats_all, workQueue_perPoseStats_current, workQueue_perPoseStats_all] = computeWorkQueues(resultsDirectory, allTestData, algorithmParameters.extractionFunctionName, thisFileChangeTime);
    
    disp(sprintf('%s: workQueue_basics has %d elements and workQueue_perPoseStats has %d elements', algorithmParameters.extractionFunctionName, length(workQueue_basicStats_current), length(workQueue_perPoseStats_current)));
    
    slashIndexes = strfind(workQueue_perPoseStats_all{1}.basicStats_filename, '/');
    lastSlashIndex = slashIndexes(end);
    temporaryDirectory = workQueue_perPoseStats_all{1}.basicStats_filename(1:lastSlashIndex);
    
    resultsData_basics = run_recompileBasics(numComputeThreads.basics, workQueue_basicStats_current, workQueue_basicStats_all, temporaryDirectory, allTestData, rotationList, algorithmParameters);
    
    resultsData_perPose = run_recompilePerPoseStats(numComputeThreads.perPose, workQueue_perPoseStats_current, workQueue_perPoseStats_all, temporaryDirectory, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, algorithmParameters.showImageDetections, algorithmParameters.showImageDetectionWidth);
    
    resultsData_overall = run_compileOverallStats(resultsData_perPose, showPlots);
    
    disp(sprintf('Compile all took %f seconds', toc(compileAllTic)));
end % compileAll()

function resultsDirectory_curTime = makeResultsDirectory(resultsDirectory, thisFileChangeTime, makeNewResultsDirectory)
    
    thisFileChangeString = ['dateTime_', datestr(thisFileChangeTime(1).datenum, 'yyyy-mm-dd_HH-MM-SS')];
    
    [~, ~, ~] = mkdir(resultsDirectory);
    
    if ~makeNewResultsDirectory
        dirs = dir([resultsDirectory, '/dateTime_*']);
        lastTime = 0;
        lastId = -1;
        for i = 1:length(dirs)
            if dirs(i).isdir
                if dirs(i).datenum > lastTime
                    lastTime = dirs(i).datenum;
                    lastId = i;
                end
            end
        end
        
        if lastId ~= -1
            thisFileChangeString = dirs(lastId).name;
        end
    end
    
    resultsDirectory_curTime = [resultsDirectory, '/', thisFileChangeString, '/'];
    resultsDirectory_curTime = strrep(resultsDirectory_curTime, '//', '/');
    
    [~, ~, ~] = mkdir(resultsDirectory_curTime);
    
end % function resultsDirectory_curTime = makeResultsDirectory()

function [workQueue_basicStats_current, workQueue_basicStats_all, workQueue_perPoseStats_current, workQueue_perPoseStats_all] = computeWorkQueues(resultsDirectory, allTestData, extractionFunctionName, thisFileChangeTime)
    
    resultsDirectory_curTime = makeResultsDirectory(resultsDirectory, thisFileChangeTime, false);
    
    extractionFunctionName = strrep(strrep(extractionFunctionName, '\', '/'), '//', '/');
    
    curExtractFunction_intermediatePrefix = [resultsDirectory_curTime, 'intermediate/', extractionFunctionName];
    curExtractFunction_dataPrefix = [resultsDirectory_curTime, 'data/', extractionFunctionName];
    curExtractFunction_imagePrefix = [resultsDirectory_curTime, 'images/', extractionFunctionName];
    
    % if extractionFunctionName has / characters for directories, don't add more
    if isempty(strfind(extractionFunctionName, '/'))
        curExtractFunction_intermediatePrefix = [curExtractFunction_intermediatePrefix, '/'];
        curExtractFunction_dataPrefix = [curExtractFunction_dataPrefix, '/'];
        curExtractFunction_imagePrefix = [curExtractFunction_imagePrefix, '/'];
    else
        curExtractFunction_intermediatePrefix = [curExtractFunction_intermediatePrefix, '_'];
        curExtractFunction_dataPrefix = [curExtractFunction_dataPrefix, '_'];
        curExtractFunction_imagePrefix = [curExtractFunction_imagePrefix, '_'];
    end
    
    slashInds = strfind(curExtractFunction_intermediatePrefix, '/');
    curExtractFunction_intermediateDirectory = curExtractFunction_intermediatePrefix(1:slashInds(end));
    
    slashInds = strfind(curExtractFunction_dataPrefix, '/');
    curExtractFunction_dataDirectory = curExtractFunction_dataPrefix(1:slashInds(end));
    
    slashInds = strfind(curExtractFunction_imagePrefix, '/');
    curExtractFunction_imageDirectory = curExtractFunction_imagePrefix(1:slashInds(end));
    
    [~, ~, ~] = mkdir(curExtractFunction_intermediateDirectory);
    [~, ~, ~] = mkdir(curExtractFunction_dataDirectory);
    [~, ~, ~] = mkdir(curExtractFunction_imageDirectory);
    
    workQueue_basicStats_all = {};
    workQueue_basicStats_current = {};
    workQueue_perPoseStats_current = {};
    workQueue_perPoseStats_all = {};
    
    for iTest = 1:size(allTestData, 1)
        numPoses = length(allTestData{iTest}.jsonData.Poses);
        
        basicStats_filenames = cell(numPoses,1);
        perPoseStats_dataFilenames = cell(numPoses,1);
        perPoseStats_imageFilenames = cell(numPoses,1);
        for iPose = 1:numPoses
            basicStats_filenames{iPose} = [curExtractFunction_intermediatePrefix, allTestData{iTest}.testFilename, sprintf('_basicStats_pose%05d_%s.mat', iPose - 1, strrep(extractionFunctionName, '/', '_'))];
            perPoseStats_dataFilenames{iPose} = [curExtractFunction_dataPrefix, allTestData{iTest}.testFilename, sprintf('_perPose_pose%05d_%s.mat', iPose - 1, strrep(extractionFunctionName, '/', '_'))];
            perPoseStats_imageFilenames{iPose} = [curExtractFunction_imagePrefix, allTestData{iTest}.testFilename, sprintf('_pose%05d_%s.png', iPose - 1, strrep(extractionFunctionName, '/', '_'))];
            
            basicStats_filenames{iPose} = strrep(strrep(basicStats_filenames{iPose}, '\', '/'), '//', '/');
            perPoseStats_dataFilenames{iPose} = strrep(strrep(perPoseStats_dataFilenames{iPose}, '\', '/'), '//', '/');
            perPoseStats_imageFilenames{iPose} = strrep(strrep(perPoseStats_imageFilenames{iPose}, '\', '/'), '//', '/');
        end
        
        clear iPose
        
        % Both basics and perPose share some fields
        newWorkItemBoth.iTest = iTest;
        newWorkItemBoth.extractionFunctionName = extractionFunctionName;
        
        % Basics stores all filenames and poses for the given test
        newWorkItemBasics = newWorkItemBoth;
        newWorkItemBasics.iPoses = 1:length(allTestData{iTest}.jsonData.Poses);
        newWorkItemBasics.basicStats_filenames = basicStats_filenames;
        newWorkItemBasics.perPoseStats_dataFilenames = perPoseStats_dataFilenames;
        newWorkItemBasics.perPoseStats_imageFilenames = perPoseStats_imageFilenames;
        
        newWorkItemBasics.CameraCalibration = allTestData{iTest}.jsonData.CameraCalibration;
        
        newWorkItemBasics.templateWidth_mm = allTestData{iTest}.jsonData.Blocks{1}.templateWidth_mm;
        
        workQueue_basicStats_all{end+1} = newWorkItemBasics; %#ok<AGROW>
        
        %
        % Basic stats
        %
        
        rerunBasics = false;
        
        % If any of the basics filenames are missing, rerun the whole basics sequence.
        for iPose = 1:numPoses
            % If the basic stats results don't exist
            if ~exist(basicStats_filenames{iPose}, 'file')
                rerunBasics = true;
                break;
            end
            
            % If the basic stats results are older than the test json
            modificationTime_basicStatsResults = dir(basicStats_filenames{iPose});
            modificationTime_basicStatsResults = modificationTime_basicStatsResults(1).datenum;
            modificationTime_test = dir([allTestData{iTest}.testPath, allTestData{iTest}.testFilename]);
            modificationTime_test = modificationTime_test(1).datenum;
            if modificationTime_basicStatsResults < modificationTime_test
                rerunBasics = true;
                break;
            end
            
            % If the basic stats results are older than the input image file
            modificationTime_inputImage = dir([allTestData{iTest}.testPath, allTestData{iTest}.jsonData.Poses{iPose}.ImageFile]);
            modificationTime_inputImage = modificationTime_inputImage(1).datenum;
            if modificationTime_basicStatsResults < modificationTime_inputImage
                rerunBasics = true;
                break;
            end
        end
        
        if rerunBasics
            workQueue_basicStats_current{end+1} = newWorkItemBasics; %#ok<AGROW>
        end
        
        %
        % Per-pose stats
        %
        
        for iPose = 1:numPoses
            addToPerPoseWorkQueue = false;
            if rerunBasics
                addToPerPoseWorkQueue = true;
            elseif ~exist(perPoseStats_dataFilenames{iPose}, 'file') || ~exist(perPoseStats_imageFilenames{iPose}, 'file')
                % If the per-pose results don't exist
                addToPerPoseWorkQueue = true;
            else
                % If the per-pose results are older than the basic stats results
                modificationTime_perPoseData = dir(perPoseStats_dataFilenames{iPose});
                modificationTime_perPoseData = modificationTime_perPoseData(1).datenum;
                modificationTime_perPoseImage = dir(perPoseStats_dataFilenames{iPose});
                modificationTime_perPoseImage = modificationTime_perPoseImage(1).datenum;
                if modificationTime_perPoseImage < modificationTime_basicStatsResults || modificationTime_perPoseData < modificationTime_basicStatsResults
                    addToPerPoseWorkQueue = true;
                end
            end
            
            % PerPose stores just the current filename and pose
            newWorkItemBasics = newWorkItemBoth;
            newWorkItemBasics.iPose = iPose;
            newWorkItemBasics.basicStats_filename = basicStats_filenames{iPose};
            newWorkItemBasics.perPoseStats_dataFilename = perPoseStats_dataFilenames{iPose};
            newWorkItemBasics.perPoseStats_imageFilename = perPoseStats_imageFilenames{iPose};
            
            workQueue_perPoseStats_all{end+1} = newWorkItemBasics; %#ok<AGROW>
            
            if addToPerPoseWorkQueue
                workQueue_perPoseStats_current{end+1} = newWorkItemBasics; %#ok<AGROW>
            end
        end % for iPose = 1:numPoses
    end % for iTest = 1:size(allTestData, 1)
end % computeWorkQueues

function resultsData_basics = run_recompileBasics(numComputeThreads, workQueue_todo, workQueue_all, temporaryDirectory, allTestData, rotationList, algorithmParameters) %#ok<INUSD>
    recompileBasicsTic = tic();
    
    if isempty(workQueue_todo)
        load(workQueue_all{1}.basicStats_filenames{1}, 'resultsData_basics');
    end
    
    numResultsData = 0;
    
    if exist('resultsData_basics', 'var')
        for i = 1:length(resultsData_basics) %#ok<NODEF>
            numResultsData = numResultsData + length(resultsData_basics{i});
        end
    end
    
    if numResultsData ~= length(workQueue_all)
        if ~isempty(workQueue_todo)
            allInputFilename = [temporaryDirectory, '/recompileBasicsAllInput.mat'];
            
            save(allInputFilename, 'allTestData', 'rotationList', 'algorithmParameters');
            
            matlabCommandString = ['disp(''Loading input...''); load(''', allInputFilename, '''); disp(''Input loaded''); ' , 'runTests_tracking_basicStats(localWorkQueue, allTestData, rotationList, algorithmParameters);'];
            
            runParallelProcesses(numComputeThreads, workQueue_todo, temporaryDirectory, matlabCommandString, true);
            
            delete(allInputFilename);
        end
        
        resultsData_basics = cell(length(allTestData), 1);
        for iTest = 1:length(allTestData)
            resultsData_basics{iTest} = cell(length(allTestData{iTest}.jsonData.Poses), 1);
        end
        
        for iWork = 1:length(workQueue_all)
            for iPose = workQueue_all{iWork}.iPoses
                load(workQueue_all{iWork}.basicStats_filenames{iPose}, 'curResultsData_basics');
                resultsData_basics{workQueue_all{iWork}.iTest}{iPose} = curResultsData_basics;
            end
        end % for iWork = 1:length(workQueue_all)
        
        % resave all results in the data for the first work element
        load(workQueue_all{1}.basicStats_filenames{1}, 'curResultsData_basics');
        save(workQueue_all{1}.basicStats_filenames{1}, 'curResultsData_basics', 'resultsData_basics');
    end
    
    disp(sprintf('Basic stat computation took %f seconds', toc(recompileBasicsTic)));
end % run_recompileBasics()

function resultsData_perPose = run_recompilePerPoseStats(numComputeThreads, workQueue_todo, workQueue_all, temporaryDirectory, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth) %#ok<INUSD>
    perPoseTic = tic();
    
    if isempty(workQueue_todo)
        load(workQueue_all{1}.perPoseStats_dataFilename, 'resultsData_perPose');
    end
    
    numResultsData = 0;
    
    if exist('resultsData_perPose', 'var')
        for i = 1:length(resultsData_perPose) %#ok<NODEF>
            numResultsData = numResultsData + length(resultsData_perPose{i});
        end
    end
    
    if numResultsData ~= length(workQueue_all)
        if ~isempty(workQueue_todo)
            if numComputeThreads ~= 1
                showImageDetections = false; %#ok<NASGU>
            end
            
            allInputFilename = [temporaryDirectory, '/perPoseAllInput.mat'];
            
            save(allInputFilename, 'allTestData', 'resultsData_basics', 'maxMatchDistance_pixels', 'maxMatchDistance_percent', 'showImageDetections', 'showImageDetectionWidth');
            
            matlabCommandString = ['disp(''Loading input...''); load(''', allInputFilename, '''); disp(''Input loaded''); ' , 'runTests_detectFiducialMarkers_compilePerPoseStats(localWorkQueue, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);'];
            
            runParallelProcesses(numComputeThreads, workQueue_todo, temporaryDirectory, matlabCommandString, true);
            
            delete(allInputFilename);
        end % if ~isempty(workQueue_todo)
        
        resultsData_perPose = cell(length(allTestData), 1);
        for iTest = 1:length(allTestData)
            resultsData_perPose{iTest} = cell(length(allTestData{iTest}.jsonData.Poses), 1);
        end
        
        for iWork = 1:length(workQueue_all)
            load(workQueue_all{iWork}.perPoseStats_dataFilename, 'curResultsData_perPose');
            resultsData_perPose{workQueue_all{iWork}.iTest}{workQueue_all{iWork}.iPose} = curResultsData_perPose;
        end % for iWork = 1:length(workQueue_all)
        
        % resave all results in the data for the first work element
        load(workQueue_all{1}.perPoseStats_dataFilename, 'curResultsData_perPose');
        save(workQueue_all{1}.perPoseStats_dataFilename, 'curResultsData_perPose', 'resultsData_perPose');
    end
    
    disp(sprintf('Per-pose stat computation took %f seconds', toc(perPoseTic)));
end % run_recompileperPoseStats()

function resultsData_overall = run_compileOverallStats(resultsData_perPose, showPlots)
    resultsData_overall = runTests_detectFiducialMarkers_compileOverallStats(resultsData_perPose, showPlots);
end % run_compileOverallStats()
