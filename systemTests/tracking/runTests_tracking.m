% function allCompiledResults = runTests_tracking()

% On PC
% allCompiledResults = runTests_tracking('C:/Anki/products-cozmo-large-files/systemTestsData/trackingScripts/fiducialDetection_*.json', 'C:/Anki/products-cozmo-large-files/systemTestsData/results/', 'z:/Box Sync');

% On Mac
% allCompiledResults = runTests_tracking('~/Documents/Anki/products-cozmo-large-files/systemTestsData/trackingScripts/fiducialDetection_*.json', '~/Documents/Anki/products-cozmo-large-files/systemTestsData/results/', '~/Box Sync');

function allCompiledResults = runTests_tracking(testJsonPattern, resultsDirectory, boxSyncDirectory)
    % To be a match, all corners of a quad must be within these thresholds
    maxMatchDistance_pixels = 10;
    maxMatchDistance_percent = 0.2;
    
    numComputeThreads.basics = 3;
    numComputeThreads.perPose = 3;
    
    % If makeNewResultsDirectory is true, make a new directory if runTests_tracking.m is changed. Otherwise, use the last created directory.
%     makeNewResultsDirectory = true;
    makeNewResultsDirectory = false;
    
    runWhichAlgorithms = {...
        'c',...
        };
    
    assert(exist('testJsonPattern', 'var') == 1);
    assert(exist('resultsDirectory', 'var') == 1);
    assert(exist('boxSyncDirectory', 'var') == 1);
    
    testJsonPattern = strrep(testJsonPattern, '~/', [tildeToPath(),'/']);
    resultsDirectory = strrep(resultsDirectory, '~/', [tildeToPath(),'/']);
    boxSyncDirectory = strrep(boxSyncDirectory, '~/', [tildeToPath(),'/']);
    
    thisFilename = [mfilename('fullpath'), '.m'];
    thisFileChangeTime = dir(thisFilename);
    
    fprintf('Loading json test data...');
    allTestData = getTestData(testJsonPattern);
    fprintf('Loaded\n');
    
    % Compute the accuracy for each test type (matlab-with-refinement, c-with-matlab-quads, etc.), and each set of parameters   
    algorithmParameters.normalizeImage = false;
    
    algorithmParameters.init_scaleTemplateRegionPercent  = 0.9;
    algorithmParameters.init_numPyramidLevels            = 3;
    algorithmParameters.init_maxSamplesAtBaseLevel       = 500;
    algorithmParameters.init_numSamplingRegions          = 5;
    algorithmParameters.init_numFiducialSquareSamples    = 500;
    algorithmParameters.init_fiducialSquareWidthFraction = 0.1;
    
    algorithmParameters.track_maxIterations                 = 50;
    algorithmParameters.track_convergenceTolerance_angle    = 0.05 * 180 / pi;
    algorithmParameters.track_convergenceTolerance_distance = 0.05;
    algorithmParameters.track_verify_maxPixelDifference     = 30;
    
%     algorithmParameters.track_ = ;
%     algorithmParameters.imageCompression = {'none', 0}; % Applies compression before running the algorithm
    
    algorithmParametersOrig = algorithmParameters;
    
    if length(runWhichAlgorithms) ~= length(unique(runWhichAlgorithms))
        disp('There is a repeat');
        keyboard
    end
    
    resultsDirectory_curTime = makeResultsDirectory(resultsDirectory, thisFileChangeTime, makeNewResultsDirectory);
    
    clear resultsData
    
    for iAlgorithm = 1:length(runWhichAlgorithms)
        algorithmParametersN = algorithmParametersOrig;
        algorithmParametersN.extractionFunctionName = runWhichAlgorithms{iAlgorithm};
        isSimpleTest = true;
        
        if strcmp(runWhichAlgorithms{iAlgorithm}, 'c')
            % Default parameters
        else
            % Unknown runWhichAlgorithms{iAlgorithm}
            assert(false);
        end
        
        % If it's a simple test (without loops or whatnot)
        if isSimpleTest
            curResults = compileAll(algorithmParametersN, boxSyncDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, thisFileChangeTime, false);
            resultsData.(runWhichAlgorithms{iAlgorithm}) = curResults;
            disp(sprintf('Results for %s = %f %f %d fileSizePercent = %f', runWhichAlgorithms{iAlgorithm}, curResults.percentQuadsExtracted, curResults.percentMarkersCorrect, curResults.numMarkerErrors, curResults.compressedFileSizeTotal / curResults.uncompressedFileSizeTotal));
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

function resultsData_overall = compileAll(algorithmParameters, boxSyncDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, thisFileChangeTime, showPlots)
    compileAllTic = tic();
    
    markerDirectoryList = {
        [boxSyncDirectory, '/Cozmo SE/VisionMarkers/symbols/withFiducials/'],...
        [boxSyncDirectory, '/Cozmo SE/VisionMarkers/letters/withFiducials/'],...
        [boxSyncDirectory, '/Cozmo SE/VisionMarkers/dice/withFiducials/']};
    
    rotationList = getListOfSymmetricMarkers(markerDirectoryList);
    
    [workQueue_basics, workQueue_perPoseStats, workQueue_all] = computeWorkQueues(resultsDirectory, allTestData, algorithmParameters.extractionFunctionName, thisFileChangeTime);
    
    disp(sprintf('%s: workQueue_basics has %d elements and workQueue_perPoseStats has %d elements', algorithmParameters.extractionFunctionName, length(workQueue_basics), length(workQueue_perPoseStats)));
    
    slashIndexes = strfind(workQueue_all{1}.basicStats_filename, '/');
    lastSlashIndex = slashIndexes(end);
    temporaryDirectory = workQueue_all{1}.basicStats_filename(1:lastSlashIndex);
    
    resultsData_basics = run_recompileBasics(numComputeThreads.basics, workQueue_basics, workQueue_all, temporaryDirectory, allTestData, rotationList, algorithmParameters);
    
    resultsData_perPose = run_recompilePerPoseStats(numComputeThreads.perPose, workQueue_perPoseStats, workQueue_all, temporaryDirectory, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, algorithmParameters.showImageDetections, algorithmParameters.showImageDetectionWidth);
    
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

function [workQueue_basicStats, workQueue_perPoseStats, workQueue_all] = computeWorkQueues(resultsDirectory, allTestData, extractionFunctionName, thisFileChangeTime)
    
    resultsDirectory_curTime = makeResultsDirectory(resultsDirectory, thisFileChangeTime, false);
    
    curExtractFunction_intermediateDirectory = [resultsDirectory_curTime, 'intermediate/', extractionFunctionName, '/'];
    curExtractFunction_dataDirectory = [resultsDirectory_curTime, 'data/', extractionFunctionName, '/'];
    curExtractFunction_imageDirectory = [resultsDirectory_curTime, 'images/', extractionFunctionName, '/'];
    
    [~, ~, ~] = mkdir(curExtractFunction_intermediateDirectory);
    [~, ~, ~] = mkdir(curExtractFunction_dataDirectory);
    [~, ~, ~] = mkdir(curExtractFunction_imageDirectory);
    
    workQueue_basicStats = {};
    workQueue_perPoseStats = {};
    workQueue_all = {};
    
    for iTest = 1:size(allTestData, 1)
        for iPose = 1:length(allTestData{iTest}.jsonData.Poses)
            basicStats_filename = [curExtractFunction_intermediateDirectory, allTestData{iTest}.testFilename, sprintf('_basicStats_pose%05d_%s.mat', iPose - 1, strrep(extractionFunctionName, '/', '_'))];
            perPoseStats_dataFilename = [curExtractFunction_dataDirectory, allTestData{iTest}.testFilename, sprintf('_perPose_pose%05d_%s.mat', iPose - 1, strrep(extractionFunctionName, '/', '_'))];
            perPoseStats_imageFilename = [curExtractFunction_imageDirectory, allTestData{iTest}.testFilename, sprintf('_pose%05d_%s.png', iPose - 1, strrep(extractionFunctionName, '/', '_'))];
            
            basicStats_filename = strrep(strrep(basicStats_filename, '\', '/'), '//', '/');
            perPoseStats_dataFilename = strrep(strrep(perPoseStats_dataFilename, '\', '/'), '//', '/');
            perPoseStats_imageFilename = strrep(strrep(perPoseStats_imageFilename, '\', '/'), '//', '/');
            
            newWorkItem.iTest = iTest;
            newWorkItem.iPose = iPose;
            newWorkItem.basicStats_filename = basicStats_filename;
            newWorkItem.perPoseStats_dataFilename = perPoseStats_dataFilename;
            newWorkItem.perPoseStats_imageFilename = perPoseStats_imageFilename;
            newWorkItem.extractionFunctionName = extractionFunctionName;
            
            workQueue_all{end+1} = newWorkItem; %#ok<AGROW>
            
            %
            % Basic stats
            %
            
            % If the basic stats results don't exist
            if ~exist(basicStats_filename, 'file')
                workQueue_basicStats{end+1} = newWorkItem; %#ok<AGROW>
                workQueue_perPoseStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the basic stats results are older than the test json
            modificationTime_basicStatsResults = dir(basicStats_filename);
            modificationTime_basicStatsResults = modificationTime_basicStatsResults(1).datenum;
            modificationTime_test = dir([allTestData{iTest}.testPath, allTestData{iTest}.testFilename]);
            modificationTime_test = modificationTime_test(1).datenum;
            if modificationTime_basicStatsResults < modificationTime_test
                workQueue_basicStats{end+1} = newWorkItem; %#ok<AGROW>
                workQueue_perPoseStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the basic stats results are older than the input image file
            modificationTime_inputImage = dir([allTestData{iTest}.testPath, allTestData{iTest}.jsonData.Poses{iPose}.ImageFile]);
            modificationTime_inputImage = modificationTime_inputImage(1).datenum;
            if modificationTime_basicStatsResults < modificationTime_inputImage
                workQueue_basicStats{end+1} = newWorkItem; %#ok<AGROW>
                workQueue_perPoseStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            %
            % Per-pose stats
            %
            
            % If the per-pose results don't exist
            if ~exist(perPoseStats_dataFilename, 'file') || ~exist(perPoseStats_imageFilename, 'file')
                workQueue_perPoseStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the per-pose results are older than the basic stats results
            modificationTime_perPoseData = dir(perPoseStats_dataFilename);
            modificationTime_perPoseData = modificationTime_perPoseData(1).datenum;
            modificationTime_perPoseImage = dir(perPoseStats_dataFilename);
            modificationTime_perPoseImage = modificationTime_perPoseImage(1).datenum;
            if modificationTime_perPoseImage < modificationTime_basicStatsResults || modificationTime_perPoseData < modificationTime_basicStatsResults
                workQueue_perPoseStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
        end % for iPose = 1:length(allTestData{iTest}.Poses)
    end % for iTest = 1:size(allTestData, 1)
end % computeWorkQueues

function resultsData_basics = run_recompileBasics(numComputeThreads, workQueue_todo, workQueue_all, temporaryDirectory, allTestData, rotationList, algorithmParameters) %#ok<INUSD>
    recompileBasicsTic = tic();
    
    if isempty(workQueue_todo)
        load(workQueue_all{1}.basicStats_filename, 'resultsData_basics');
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
            load(workQueue_all{iWork}.basicStats_filename, 'curResultsData_basics');
            resultsData_basics{workQueue_all{iWork}.iTest}{workQueue_all{iWork}.iPose} = curResultsData_basics;
        end % for iWork = 1:length(workQueue_all)
        
        % resave all results in the data for the first work element
        load(workQueue_all{1}.basicStats_filename, 'curResultsData_basics');
        save(workQueue_all{1}.basicStats_filename, 'curResultsData_basics', 'resultsData_basics');
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
            
            matlabCommandString = ['disp(''Loading input...''); load(''', allInputFilename, '''); disp(''Input loaded''); ' , 'runTests_tracking_compilePerPoseStats(localWorkQueue, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);'];
            
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
    resultsData_overall = runTests_tracking_compileOverallStats(resultsData_perPose, showPlots);
end % run_compileOverallStats()
