% function allCompiledResults = runTests_detectFiducialMarkers()

% allCompiledResults = runTests_detectFiducialMarkers('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_*_all.json', 'C:/Anki/systemTestImages/results/', 'Z:/Documents/Box Documents');

function allCompiledResults = runTests_detectFiducialMarkers(testJsonPattern, resultsDirectory, boxSyncDirectory)
    global currentCompileNumber;
    currentCompileNumber = 0;
    
    % To be a match, all corners of a quad must be within these thresholds
    maxMatchDistance_pixels = 5;
    maxMatchDistance_percent = 0.2;
       
    numComputeThreads = 4;
    
    showImageDetections = true;
    showImageDetectionWidth = 640;
    %     showOverallStats = true;
    
    assert(exist('testJsonPattern', 'var') == 1);
    assert(exist('resultsDirectory', 'var') == 1);
    assert(exist('boxSyncDirectory', 'var') == 1);
    
    allTestData = getTestData(testJsonPattern);
    
    % Compute the accuracy for each test type (matlab-with-refinement, c-with-matlab-quads, etc.), and each set of parameters
    imageSize = [240,320];
    minSideLength = round(0.03*max(imageSize(1),imageSize(2)));
    maxSideLength = round(0.97*min(imageSize(1),imageSize(2)));
        
    algorithmParameters.scaleImage_thresholdMultiplier = 1.0;
    algorithmParameters.scaleImage_numPyramidLevels = 3;
    algorithmParameters.component1d_minComponentWidth = 0;
    algorithmParameters.component1d_maxSkipDistance = 0;
    algorithmParameters.component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
    algorithmParameters.component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
    algorithmParameters.component_sparseMultiplyThreshold = 1000.0;
    algorithmParameters.component_solidMultiplyThreshold = 2.0;
    algorithmParameters.component_minHollowRatio = 1.0;
    algorithmParameters.quads_minQuadArea = 100 / 4;
    algorithmParameters.quads_quadSymmetryThreshold = 2.0;
    algorithmParameters.quads_minDistanceFromImageEdge = 2;
    algorithmParameters.decode_minContrastRatio = 1.25;
    algorithmParameters.refine_quadRefinementMaxCornerChange = 2;
    algorithmParameters.refine_numRefinementSamples = 100;
    algorithmParameters.refine_quadRefinementIterations = 25;
    algorithmParameters.useMatlabForAll = false;
    algorithmParameters.useMatlabForQuadExtraction = false;
    
     
    
    algorithmParametersN = algorithmParameters;
    algorithmParametersN.useMatlabForAll = true;
    algorithmParametersN.name = 'matlab-with-refinement';
    compileAll(algorithmParametersN, boxSyncDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);
    
    algorithmParametersN = algorithmParameters;
    algorithmParametersN.useMatlabForAll = true;
    algorithmParametersN.refine_quadRefinementIterations = 0;
    algorithmParametersN.name = 'matlab-no-refinement';
    compileAll(algorithmParametersN, boxSyncDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);
    
    algorithmParametersN = algorithmParameters;
    algorithmParametersN.useMatlabForQuadExtraction = true;
    algorithmParametersN.name = 'matlabQuad-with-refinement';
    compileAll(algorithmParametersN, boxSyncDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);
    
    algorithmParametersN = algorithmParameters;
    algorithmParametersN.useMatlabForQuadExtraction = true;
    algorithmParametersN.refine_quadRefinementIterations = 0;
    algorithmParametersN.name = 'matlabQuad-no-refinement';
    compileAll(algorithmParametersN, boxSyncDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);
    
    algorithmParametersN = algorithmParameters;
    algorithmParametersN.name = 'c-with-refinement';
    compileAll(algorithmParametersN, boxSyncDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);
    
    algorithmParametersN = algorithmParameters;
    algorithmParametersN.refine_quadRefinementIterations = 0;
    algorithmParametersN.name = 'c-no-refinement';
    compileAll(algorithmParametersN, boxSyncDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);
end % runTests_detectFiducialMarkers()

function allTestData = getTestData(testJsonPattern)
    testJsonPattern = strrep(testJsonPattern, '\', '/');
    slashIndexes = strfind(testJsonPattern, '/');
    testPath = testJsonPattern(1:(slashIndexes(end)));
    
    allTestFilenamesRaw = dir(testJsonPattern);
    
    allTestData = cell(length(allTestFilenamesRaw), 1);
    
    for iTest = 1:length(allTestFilenamesRaw)
        allTestFilename = [testPath, allTestFilenamesRaw(iTest).name];
        
        % Resave json files to mat files
        allTestData{iTest} = convertJsonToMat(allTestFilename);
    end
end % getTestFilenames()

function compileAll(algorithmParameters, boxSyncDirectory, resultsDirectory, allTestData, numComputeThreads, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth)
    global currentCompileNumber;
    
    markerDirectoryList = {
        [boxSyncDirectory, '/Cozmo SE/VisionMarkers/symbols/withFiducials/'],...
        [boxSyncDirectory, '/Cozmo SE/VisionMarkers/letters/withFiducials'],...
        [boxSyncDirectory, '/Cozmo SE/VisionMarkers/dice/withFiducials']};
    
    rotationList = getListOfSymmetricMarkers(markerDirectoryList);
    
    [workQueue_basics, workQueue_perTestStats, workQueue_all] = computeWorkQueues(resultsDirectory, allTestData, currentCompileNumber);
    
    disp(sprintf('workQueue_basics has %d elements and workQueue_perTestStats has %d elements', length(workQueue_basics), length(workQueue_perTestStats)));
    
    resultsData_basics = run_recompileBasics(numComputeThreads, workQueue_basics, workQueue_all, allTestData, rotationList, algorithmParameters);
    %         save(basicsFilename, 'resultsData', 'testPath', 'allTestFilenames', 'testFunctions', 'testFunctionNames');
    
    perTestStats = run_recompilePerTestStats(numComputeThreads, workQueue_perTestStats, workQueue_all, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);    
    %         save(perTestStatsFilename, 'perTestStats');
    
    %     if recompileOverallStats
    %         overallStats = run_compileOverallStats(numComputeThreads, ignoreModificationTime, allTestFilenames, perTestStats, showOverallStats);
    %         save(overallStatsFilename, 'overallStats');
    %     else
    %         load(overallStatsFilename);
    %     end
    
    %     save([resultsDirectory, 'allCompiledResults.mat'], 'perTestStats');
    
    currentCompileNumber = currentCompileNumber + 1;
end % compileAll()

function [workQueue_basicStats, workQueue_perTestStats, workQueue_all] = computeWorkQueues(resultsDirectory, allTestData, currentCompileNumber)
    
    thisFilename = [mfilename('fullpath'), '.m'];
    thisFileChangeTime = dir(thisFilename);
%     thisFileChangeString = datestr(thisFileChangeTime(1).datenum, 'yyyy-mm-dd_HH-MM-SS');
    thisFileChangeString = 'rrr';
    
    resultsDirectory_curTime = [resultsDirectory, '/', thisFileChangeString, '/'];
    intermediateDirectory = [resultsDirectory_curTime, 'intermediate/'];
    
    [~, ~, ~] = mkdir(resultsDirectory);
    [~, ~, ~] = mkdir(resultsDirectory_curTime);
    [~, ~, ~] = mkdir(intermediateDirectory);
    
    workQueue_basicStats = {};
    workQueue_perTestStats = {};
    workQueue_all = {};
    
    for iTest = 1:size(allTestData, 1)
        for iPose = 1:length(allTestData{iTest}.jsonData.Poses)
            basicStats_filename = [intermediateDirectory, allTestData{iTest}.testFilename, sprintf('_basicStats_pose%05d_cn%05d.mat', iPose, currentCompileNumber)];
            perTestStats_dataFilename = [intermediateDirectory, allTestData{iTest}.testFilename, sprintf('_per_test_pose%05d_cn%05d.mat', iPose, currentCompileNumber)];
            perTestStats_imageFilename = [resultsDirectory_curTime, allTestData{iTest}.testFilename, sprintf('_pose%05d_cn%05d.png', iPose, currentCompileNumber)];
            
            basicStats_filename = strrep(strrep(basicStats_filename, '\', '/'), '//', '/');
            perTestStats_dataFilename = strrep(strrep(perTestStats_dataFilename, '\', '/'), '//', '/');
            perTestStats_imageFilename = strrep(strrep(perTestStats_imageFilename, '\', '/'), '//', '/');
            
            newWorkItem.iTest = iTest;
            newWorkItem.iPose = iPose;
            newWorkItem.basicStats_filename = basicStats_filename;
            newWorkItem.perTestStats_dataFilename = perTestStats_dataFilename;
            newWorkItem.perTestStats_imageFilename = perTestStats_imageFilename;
            
            workQueue_all{end+1} = newWorkItem; %#ok<AGROW>
            
            %
            % Basic stats
            %
            
            % If the basic stats results don't exist
            if ~exist(basicStats_filename, 'file')
                workQueue_basicStats{end+1} = newWorkItem; %#ok<AGROW>
                workQueue_perTestStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the basic stats results are older than the test json
            modificationTime_basicStatsResults = dir(basicStats_filename);
            modificationTime_basicStatsResults = modificationTime_basicStatsResults(1).datenum;
            modificationTime_test = dir([allTestData{iTest}.testPath, allTestData{iTest}.testFilename]);
            modificationTime_test = modificationTime_test(1).datenum;
            if modificationTime_basicStatsResults < modificationTime_test
                workQueue_basicStats{end+1} = newWorkItem; %#ok<AGROW>
                workQueue_perTestStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the basic stats results are older than the input image file
            modificationTime_inputImage = dir([allTestData{iTest}.testPath, allTestData{iTest}.jsonData.Poses{iPose}.ImageFile]);
            modificationTime_inputImage = modificationTime_inputImage(1).datenum;
            if modificationTime_basicStatsResults < modificationTime_inputImage
                workQueue_basicStats{end+1} = newWorkItem; %#ok<AGROW>
                workQueue_perTestStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            %
            % Per-test stats
            %
            
            % If the per-test results don't exist
            if ~exist(perTestStats_dataFilename, 'file') || ~exist(perTestStats_imageFilename, 'file')
                workQueue_perTestStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the per-test results are older than the basic stats results
            modificationTime_perTestData = dir(perTestStats_dataFilename);
            modificationTime_perTestData = modificationTime_perTestData(1).datenum;
            modificationTime_perTestImage = dir(perTestStats_dataFilename);
            modificationTime_perTestImage = modificationTime_perTestImage(1).datenum;
            if modificationTime_perTestImage < modificationTime_basicStatsResults || modificationTime_perTestData < modificationTime_basicStatsResults
                workQueue_perTestStats{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
        end % for iPose = 1:length(allTestData{iTest}.Poses)
    end % for iTest = 1:size(allTestData, 1)
end % computeWorkQueues

function data = convertJsonToMat(jsonFilename)
    jsonFilename = strrep(strrep(jsonFilename, '\', '/'), '//', '/');
    matFilename = strrep(jsonFilename, '.json', '.mat');
    
    doConversion = false;
    
    % If the results don't exist
    if ~exist(matFilename, 'file')
        doConversion = true;
    else
        modificationTime_mat = dir(matFilename);
        modificationTime_mat = modificationTime_mat(1).datenum;
        
        modificationTime_json = dir(jsonFilename);
        modificationTime_json = modificationTime_json(1).datenum;
        
        if modificationTime_mat < modificationTime_json
            doConversion = true;
        end
    end
    
    if doConversion
        jsonData = loadjson(jsonFilename);
        save(matFilename, 'jsonData');
    else
        load(matFilename);
    end
    
    testFileModificationTime = dir(jsonFilename);
    testFileModificationTime = testFileModificationTime(1).datenum;
    
    slashIndexes = strfind(jsonFilename, '/');
    lastSlashIndex = slashIndexes(end);
    testPath = jsonFilename(1:lastSlashIndex);
    testFilename = jsonFilename((lastSlashIndex+1):end);
    
    data.jsonData = jsonData;
    data.testFilename = testFilename;
    data.testPath = testPath;
    data.testFileModificationTime = testFileModificationTime;
end % convertJsonToMat()

function resultsData_basics = run_recompileBasics(numComputeThreads, workQueue_todo, workQueue_all, allTestData, rotationList, algorithmParameters)
    recompileBasicsTic = tic();
    
    save('recompileBasicsAllInput.mat', 'allTestData', 'rotationList', 'algorithmParameters');

    matlabCommandString = ['load(''recompileBasicsAllInput.mat''); ' , 'runTests_detectFiducialMarkers_basicStats(localWorkQueue, allTestData, rotationList, algorithmParameters);'];

    runParallelProcesses(numComputeThreads, workQueue_todo, matlabCommandString);

    delete('recompileBasicsAllInput.mat');
    
    resultsData_basics = cell(length(allTestData), 1);
    for iTest = 1:length(allTestData)
        resultsData_basics{iTest} = cell(length(allTestData{iTest}.jsonData.Poses), 1);
    end
    
    for iWork = 1:length(workQueue_all)
        load(workQueue_all{iWork}.basicStats_filename, 'curResultsData');
        resultsData_basics{workQueue_all{iWork}.iTest}{workQueue_all{iWork}.iPose} = curResultsData;
    end % for iWork = 1:length(workQueue_all)

    disp(sprintf('Basic stat computation took %f seconds', toc(recompileBasicsTic)));
end % run_recompileBasics()

function resultsData_perTest = run_recompilePerTestStats(numComputeThreads, workQueue_todo, workQueue_all, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth)
    perTestTic = tic();
    
    if numComputeThreads ~= 1
        showImageDetections = false;
    end
    
    save('perTestAllInput.mat', 'allTestData', 'resultsData_basics', 'maxMatchDistance_pixels', 'maxMatchDistance_percent', 'showImageDetections', 'showImageDetectionWidth');

    matlabCommandString = ['load(''perTestAllInput.mat''); ' , 'runTests_detectFiducialMarkers_compilePerTestStats(localWorkQueue, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth);'];

    runParallelProcesses(numComputeThreads, workQueue_todo, matlabCommandString);

    delete('perTestAllInput.mat');
    
    resultsData_perTest = cell(length(allTestData), 1);
    for iTest = 1:length(allTestData)
        resultsData_perTest{iTest} = cell(length(allTestData{iTest}.jsonData.Poses), 1);
    end
    
    for iWork = 1:length(workQueue_all)
        load(workQueue_all{iWork}.perTestStats_dataFilename, 'curCompiledData');
        resultsData_perTest{workQueue_all{iWork}.iTest}{workQueue_all{iWork}.iPose} = curCompiledData;
    end % for iWork = 1:length(workQueue_all)

    disp(sprintf('Per-test stat computation took %f seconds', toc(perTestTic)));    
end % run_recompilePerTestStats()

% function overallStats = run_compileOverallStats(numComputeThreads, ignoreModificationTime, allTestFilenames, perTestStats, showOverallStats)
%     overallStats = runTests_detectFiducialMarkers_compileOverallStats(allTestFilenames, perTestStats, showOverallStats);
% end % run_compileOverallStats()
