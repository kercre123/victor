% function allCompiledResults = runTests_detectFiducialMarkers(testJsonPattern, resultsFilename)

% allCompiledResults = runTests_detectFiducialMarkers('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_*_all.json', 'C:/Anki/systemTestImages/results/');

function allCompiledResults = runTests_detectFiducialMarkers(testJsonPattern, resultsDirectory)
    
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    % useUndistortion = false;
    
    % To be a match, all corners of a quad must be within these thresholds
    maxMatchDistance_pixels = 5;
    maxMatchDistance_percent = 0.2;
    
    numComputeThreads = 3;
    
    showImageDetections = true;
    showImageDetectionWidth = 640;
    showOverallStats = true;
    
    recompileBasics = true;
    recompilePerTestStats = true;
    recompileOverallStats = false;
    
    basicsFilename = 'basicsResults';
    perTestStatsFilename = 'perTestStatsResults';
    
    markerDirectoryList = {'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/symbols/withFiducials/', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/letters/withFiducials', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/dice/withFiducials'}; %#ok<NASGU>
    
    assert(exist('testJsonPattern', 'var') == 1);
    assert(exist('resultsDirectory', 'var') == 1);
    
    [~, ~, ~] = mkdir(resultsDirectory);
    
    testJsonPattern = strrep(testJsonPattern, '\', '/');
    slashIndexes = strfind(testJsonPattern, '/');
    testPath = testJsonPattern(1:(slashIndexes(end)));
    
    allTestFilenamesRaw = dir(testJsonPattern);
    
    allTestFilenames = cell(length(allTestFilenamesRaw), 1);
    
    for i = 1:length(allTestFilenamesRaw)
        allTestFilenames{i} = [testPath, allTestFilenamesRaw(i).name];
    end
    
    testFunctions = {...
        @extractMarkers_c_noRefinement,...
        @extractMarkers_c_withRefinement,...
        @extractMarkers_matlabOriginal_noRefinement,...
        @extractMarkers_matlabOriginal_withRefinement,...
        @extractMarkers_matlabOriginalQuads_cExtraction_noRefinement,...
        @extractMarkers_matlabOriginalQuads_cExtraction_withRefinement};
    
    testFunctionNames = {...
        'c-noRef',...
        'c-ref',...
        'mat-noRef',...
        'mat-ref',...
        'matQuad-cExt-noRef',...
        'matQuad-cExt-ref'};
    
    if recompileBasics
        resultsData = run_recompileBasics(numComputeThreads, allTestFilenames, markerDirectoryList, testFunctions, basicsFilename);
        save(basicsFilename, 'resultsData', 'testPath', 'allTestFilenames', 'testFunctions', 'testFunctionNames');
    else % if recompileBasics
        load(basicsFilename);
    end % if recompileBasics ... else
    
    if recompilePerTestStats
        perTestStats = run_recompilePerTestStats(numComputeThreads, resultsData, testPath, allTestFilenames, testFunctionNames, resultsDirectory, showImageDetections, showImageDetectionWidth);
        save(perTestStatsFilename, 'perTestStats');
    else
        load(perTestStatsFilename);
    end
    
    if recompileOverallStats
        overallStats = run_compileOverallStats(numComputeThreads, allTestFilenames, perTestStats, showOverallStats);
        save(overallStatsFilename, 'overallStats');
    else
        load(overallStatsFilename);
    end
    
%     save([resultsDirectory, 'allCompiledResults.mat'], 'perTestStats');
    
    keyboard
end % runTests_detectFiducialMarkers()

function resultsData = run_recompileBasics(numComputeThreads, allTestFilenames, markerDirectoryList, testFunctions, basicsFilename)
    ignoreModificationTime = false;
    
    recompileBasicsTic = tic();
    
    slashIndexes = strfind(allTestFilenames{1}, '/');
    testPath = allTestFilenames{1}(1:(slashIndexes(end)));
    intermediateDirectory = [testPath, 'intermediate/'];
    if ~exist(intermediateDirectory, 'file')
        mkdir(intermediateDirectory);
    end
    
    % create a list of test-pose work
    workList = {};
    for iTest = 1:length(allTestFilenames)
        %         for iTest = 3
        modificationTime_test = dir(allTestFilenames{iTest});
        modificationTime_test = modificationTime_test(1).datenum;
        
        curTestFilename_imagesOnly = strrep(allTestFilenames{iTest}, '_all.json', '_justFilenames.json');
        
        jsonData = loadjson(curTestFilename_imagesOnly);
        for iPose = 1:length(jsonData.Poses)
            %             for iPose = 2
            jsonTestFilename = allTestFilenames{iTest};
            
            slashIndexes = strfind(jsonTestFilename, '/');
            testPath = jsonTestFilename(1:(slashIndexes(end)));
            resultsFilename = [intermediateDirectory, jsonTestFilename((slashIndexes(end)+1):end), sprintf('_pose%05d.mat', iPose)];
            
            newWorkItem = {iTest, jsonTestFilename, iPose, resultsFilename};
            
            % If we're ignoring the modification time, add it to the queue
            if ignoreModificationTime
                workList{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the results don't exist, add it to the queue
            if ~exist(resultsFilename, 'file')
                workList{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the results are older than the test json, add it to the queue
            modificationTime_results = dir(resultsFilename);
            modificationTime_results = modificationTime_results(1).datenum;
            if modificationTime_results < modificationTime_test
                workList{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the results are older than the image file, add it to the queue
            modificationTime_image = dir([testPath, jsonData.Poses{iPose}.ImageFile]);
            modificationTime_image = modificationTime_image(1).datenum;
            if modificationTime_results < modificationTime_image
                workList{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
        end
    end
    
    disp(sprintf('workList has %d elements', length(workList)));
    
    rotationList = getListOfSymmetricMarkers(markerDirectoryList);
    
    % If one thread, just compute locally
    if numComputeThreads == 1 || isempty(workList)
        runTests_detectFiducialMarkers_basicStats(testFunctions, rotationList, workList);
    else % if numComputeThreads == 1
        %             matlabLaunchCommand = 'matlab -nojvm -noFigureWindows -nosplash ';
        %             matlabLaunchCommand = 'matlab -noFigureWindows -nosplash ';
        matlabLaunchCommand = 'matlab -noFigureWindows -nosplash ';
        
        threadCompletionMutexFilenames = cell(numComputeThreads, 1);
        workerInputFilenames = cell(numComputeThreads, 1);
        
        % launch threads
        for iThread = 0:(numComputeThreads-1)
            workerInputFilenames{iThread+1} = sprintf('%s_input%d.mat', basicsFilename, iThread);
            localWorkList = workList((1+iThread):numComputeThreads:end); %#ok<NASGU>
            save(workerInputFilenames{iThread+1}, 'testFunctions', 'rotationList', 'localWorkList');
            
            threadCompletionMutexFilenames{iThread+1} = sprintf('%s_mutex%d.mat', basicsFilename, iThread);
            
            try
                oldWarnings = warning();
                warning('off', 'MATLAB:DELETE:FileNotFound');
                delete(threadCompletionMutexFilenames{iThread+1});
                warning(oldWarnings);
            catch
            end
            
            hideWindowsCommand = 'frames = java.awt.Frame.getFrames; for frameIdx = 3:length(frames) try awtinvoke(frames(frameIdx),''setVisible'',0); catch end; end;';
            
            commandString = sprintf('-r "%s load(''%s''); runTests_detectFiducialMarkers_basicStats(testFunctions, rotationList, localWorkList); mut=1; save(''%s'',''mut''); exit;"', hideWindowsCommand, workerInputFilenames{iThread+1}, threadCompletionMutexFilenames{iThread+1});
            system(['start /b ', matlabLaunchCommand, commandString]);
        end
        
        % Wait for thread completion
        for iThread = 1:length(threadCompletionMutexFilenames)
            while ~exist(threadCompletionMutexFilenames{iThread}, 'file')
                pause(.1);
            end
            delete(threadCompletionMutexFilenames{iThread});
            delete(workerInputFilenames{iThread});
        end
    end
    
    disp('Computation complete. Merging thread results...');
    
    resultsData = cell(length(allTestFilenames), 1);
    for iTest = 1:length(allTestFilenames)
        curTestFilename_imagesOnly = strrep(allTestFilenames{iTest}, '_all.json', '_justFilenames.json');
        jsonData = loadjson(curTestFilename_imagesOnly);
        
        jsonTestFilename = allTestFilenames{iTest};
        
        resultsData{iTest} = cell(length(jsonData.Poses), 1);
        
        for iPose = 1:length(jsonData.Poses)
            slashIndexes = strfind(jsonTestFilename, '/');
            testPath = jsonTestFilename(1:(slashIndexes(end)));
            resultsFilename = [testPath, 'intermediate/', jsonTestFilename((slashIndexes(end)+1):end), sprintf('_pose%05d.mat', iPose)];
            
            load(resultsFilename, 'curResultsData');
            
            resultsData{iTest}{iPose} = curResultsData;
        end % for iPose = 1:length(jsonData.Poses)
    end % for iTest = 1:length(allTestFilenames)
    
    disp(sprintf('Basic stat computation took %f seconds', toc(recompileBasicsTic)));
end % run_recompileBasics()

function overallStats = run_compileOverallStats(numComputeThreads, allTestFilenames, perTestStats, showOverallStats)
    overallStats = runTests_detectFiducialMarkers_compileOverallStats(allTestFilenames, perTestStats, showOverallStats);
end % run_compileOverallStats()

function perTestStats = run_recompilePerTestStats(numComputeThreads, resultsData, testPath, allTestFilenames, testFunctionNames, resultsDirectory, showImageDetections, showImageDetectionWidth)
    perTestStats = runTests_detectFiducialMarkers_compilePerTestStats(resultsData, testPath, allTestFilenames, testFunctionNames, resultsDirectory, showImageDetections, showImageDetectionWidth);
end % run_recompilePerTestStats()
