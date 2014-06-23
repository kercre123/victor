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
    
    basicsFilename = 'basicsResults';
    perTestStatsFilename = 'perTestStatsResults';
    
    markerDirectoryList = {'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/symbols/withFiducials/', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/letters/withFiducials', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/dice/withFiducials'}; %#ok<NASGU>
    
    assert(exist('testJsonPattern', 'var') == 1);
    assert(exist('resultsDirectory', 'var') == 1);
    
    try
        mkdir(resultsDirectory);
    catch
    end
       
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
        if numComputeThreads == 1
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
                    delete(threadCompletionMutexFilenames{iThread+1});
                catch
                end
                
                commandString = sprintf('-r "load(''%s''); runTests_detectFiducialMarkers_basicStats(testFunctions, rotationList, localWorkList); mut=1; save(''%s'',''mut''); exit;"', workerInputFilenames{iThread+1}, threadCompletionMutexFilenames{iThread+1});
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
        
        save(basicsFilename, 'resultsData', 'testPath', 'allTestFilenames', 'testFunctions', 'testFunctionNames');
    else % if recompileBasics
        load(basicsFilename);
    end % if recompileBasics ... else
    
    if recompilePerTestStats
        perTestStats = compilePerTestStats(resultsData, testPath, allTestFilenames, testFunctionNames, resultsDirectory, showImageDetections, showImageDetectionWidth);
        
        save(perTestStatsFilename, 'perTestStats');
    else
        load(perTestStatsFilename);
    end
    
    overallStats = compileOverallStats(allTestFilenames, perTestStats, showOverallStats);
    
    save([resultsDirectory, 'allCompiledResults.mat'], 'perTestStats');
    
    keyboard
end % runTests_detectFiducialMarkers()

function overallStats = compileOverallStats(allTestFilenames, perTestStats, showOverallStats)
    
    allJsonData = cell(length(resultsData), 1);
    
    for iTest = 1:length(resultsData)
        allJsonData{iTest} = loadjson(allTestFilenames{iTest});
    end
    
    numTestFunctions = length(perTestStats{1}{1});
    
    for iTestFunction = 1:numTestFunctions
        for iTest = 1:length(perTestStats)
            for iPose = 1:length(perTestStats{iTest})
            end
        end
    end
end % compileOverallStats()


function perTestStats = compilePerTestStats(resultsData, testPath, allTestFilenames, testFunctionNames, resultsDirectory, showImageDetections, showImageDetectionWidth)
    % TODO: At different distances / angles
    
    global useImpixelinfo;
    
    if exist('useImpixelinfo', 'var')
        old_useImpixelinfo = useImpixelinfo;
    else
        old_useImpixelinfo = -1;
    end
    
    showImageDetectionsScale = 1;
    
    perTestStats = cell(length(resultsData), 1);
    
    filenameNameLookup = '';
    
    for iTest = 1:length(resultsData)
        %     for iTest = length(resultsData)
        
        tic
        
        perTestStats{iTest} = cell(length(resultsData{iTest}), 1);
        
        jsonData = loadjson(allTestFilenames{iTest});
        
        for iPose = 1:length(perTestStats{iTest})
            perTestStats{iTest}{iPose} = cell(length(resultsData{iTest}{iPose}), 1);
            
            imageFilename = [testPath, jsonData.Poses{iPose}.ImageFile];
            imageFilename = strrep(imageFilename, '//', '/');
            image = imread(imageFilename);
            
            outputFilenameImage = [resultsDirectory, sprintf('detection_dist%d_angle%d_expose%0.1f_light%d.png',...
                jsonData.Poses{iPose}.Scene.Distance,...
                jsonData.Poses{iPose}.Scene.angle,...
                jsonData.Poses{iPose}.Scene.CameraExposure,...
                jsonData.Poses{iPose}.Scene.light)];
            
            slashIndexes = strfind(outputFilenameImage, '/');
            outputFilenameImageJustName = outputFilenameImage((slashIndexes(end)+1):end);
            slashIndexes = strfind(imageFilename, '/');
            imageFilenameJustName = imageFilename((slashIndexes(end)+1):end);
            
            filenameNameLookup = [filenameNameLookup, sprintf('%d %d \t%s \t%s\n', iTest, iPose, outputFilenameImageJustName, imageFilenameJustName)]; %#ok<AGROW>
            
            if showImageDetectionWidth(1) ~= size(image,2)
                showImageDetectionsScale = showImageDetectionWidth(1) / size(image,2);
            else
                showImageDetectionsScale = 1;
            end
            
            for iTestFunction = 1:length(perTestStats{iTest}{iPose})
                curResultsData = resultsData{iTest}{iPose}{iTestFunction};
                
                [curCompiled.numQuadsNotIgnored, curCompiled.numQuadsDetected] = compileQuadResults(curResultsData);
                
                [curCompiled.numCorrect_positionLabelRotation,...
                    curCompiled.numCorrect_positionLabel,...
                    curCompiled.numCorrect_position,...
                    curCompiled.numSpurriousDetections,...
                    curCompiled.numUndetected,...
                    markersToDisplay] = compileMarkerResults(curResultsData);
                
                outputFilenameResult = [resultsDirectory, sprintf('result_%03d%03d%03d_dist%d_angle%d_expose%0.1f_light%d_%s.png',...
                    iTest, iPose, iTestFunction,...
                    jsonData.Poses{iPose}.Scene.Distance,...
                    jsonData.Poses{iPose}.Scene.angle,...
                    jsonData.Poses{iPose}.Scene.CameraExposure,...
                    jsonData.Poses{iPose}.Scene.light,...
                    testFunctionNames{iTestFunction})];
                
                toShowResults = {
                    iTest,...
                    iPose,...
                    iTestFunction,...
                    jsonData.Poses{iPose}.Scene.Distance,...
                    jsonData.Poses{iPose}.Scene.angle,...
                    jsonData.Poses{iPose}.Scene.CameraExposure,...
                    jsonData.Poses{iPose}.Scene.light,...
                    testFunctionNames{iTestFunction},...
                    curCompiled.numCorrect_positionLabelRotation,...
                    curCompiled.numCorrect_positionLabel,...
                    curCompiled.numCorrect_position,...
                    curCompiled.numQuadsDetected,...
                    curCompiled.numQuadsNotIgnored,...
                    curCompiled.numSpurriousDetections};
                
                drawnImage = mexDrawSystemTestResults(uint8(image), curResultsData.detectedQuads, curResultsData.detectedQuadValidity, markersToDisplay(:,1), int32(cell2mat(markersToDisplay(:,2))), markersToDisplay(:,3), showImageDetectionsScale, outputFilenameResult, toShowResults);
                drawnImage = drawnImage(:,:,[3,2,1]);
                
                perTestStats{iTest}{iPose}{iTestFunction} = curCompiled;
                
                if showImageDetections
                    imshow(drawnImage)
                    pause(.03);
                end
            end % for iTestFunction = 1:length(perTestStats{iTest}{iPose})
        end % for iPose = 1:length(perTestStats{iTest})
        
        disp(sprintf('Compiled test results %d/%d in %f seconds', iTest, length(resultsData), toc()));
    end % for iTest = 1:length(resultsData)
    
    outputFilenameNameLookup = [resultsDirectory, 'filenameLookup.txt'];
    
    fileId = fopen(outputFilenameNameLookup, 'w');
    fprintf(fileId, filenameNameLookup);
    fclose(fileId);
end % compilePerTestStats()

function [numNotIgnored, numGood] = compileQuadResults(curResultsData)
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    numTotal = length(curResultsData.markerNames_groundTruth);
    numNotIgnored = 0;
    numGood = 0;
    
    for iQuad = 1:numTotal
        if ~strcmp(curResultsData.markerNames_groundTruth{iQuad}, 'MARKER_IGNORE')
            numNotIgnored = numNotIgnored + 1;
            
            curDistance = curResultsData.justQuads_bestDistances_max(iQuad,:);
            curFiducialSize = curResultsData.fiducialSizes_groundTruth(iQuad,:);
            
            % Are all distances small enough?
            if curDistance(1) <= maxMatchDistance_pixels &&...
                    curDistance(2) <= maxMatchDistance_pixels &&...
                    curDistance(3) <= maxMatchDistance_pixels &&...
                    curDistance(2) <= (curFiducialSize(1) * maxMatchDistance_percent) &&...
                    curDistance(3) <= (curFiducialSize(2) * maxMatchDistance_percent)
                
                numGood = numGood + 1;
            end
        end
    end % for iQuad = 1:numTotal
end % compileQuadResults()

function [numCorrect_positionLabelRotation, numCorrect_positionLabel, numCorrect_position, numSpurriousDetections, numUndetected, markersToDisplay] = compileMarkerResults(curResultsData)
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    numCorrect_positionLabelRotation = 0;
    numCorrect_positionLabel = 0;
    numCorrect_position = 0;
    numSpurriousDetections = 0;
    numUndetected = 0;
    
    matchedIndexes = [];
    
    markersToDisplay = cell(0,3); % {quad, type, name}
    
    % Check all ground truth markers
    for iMarker = 1:length(curResultsData.markerNames_groundTruth)
        curDistance = curResultsData.markers_bestDistances_max(iMarker,:);
        curFiducialSize = curResultsData.fiducialSizes_groundTruth(iMarker,:);
        
        clear matchIndex;
        
        if strcmp(curResultsData.markerNames_groundTruth{iMarker}, 'MARKER_IGNORE')
            
            if curDistance(1) <= maxMatchDistance_pixels &&...
                    curDistance(2) <= maxMatchDistance_pixels &&...
                    curDistance(3) <= maxMatchDistance_pixels &&...
                    curDistance(2) <= (curFiducialSize(1) * maxMatchDistance_percent) &&...
                    curDistance(3) <= (curFiducialSize(2) * maxMatchDistance_percent)
                
                matchIndex = curResultsData.markers_bestIndexes(iMarker);
                markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{matchIndex}.corners, 1, 'IGNORE'}; %#ok<AGROW>
                
                matchedIndexes(end+1) = matchIndex; %#ok<AGROW>
            else
                markersToDisplay(end+1,:) = {curResultsData.markerLocations_groundTruth{iMarker}, 2, 'REJECT'}; %#ok<AGROW>
            end
            
            continue;
        end
        
        % Are all distances small enough?
        if curDistance(1) <= maxMatchDistance_pixels &&...
                curDistance(2) <= maxMatchDistance_pixels &&...
                curDistance(3) <= maxMatchDistance_pixels &&...
                curDistance(2) <= (curFiducialSize(1) * maxMatchDistance_percent) &&...
                curDistance(3) <= (curFiducialSize(2) * maxMatchDistance_percent)
            
            matchIndex = curResultsData.markers_bestIndexes(iMarker);
            
            matchedIndexes(end+1) = matchIndex; %#ok<AGROW>
            
            if strcmp(curResultsData.markerNames_groundTruth{iMarker}, curResultsData.markerNames_detected{matchIndex})
                if curResultsData.markers_areRotationsCorrect(iMarker)
                    numCorrect_positionLabelRotation = numCorrect_positionLabelRotation + 1;
                    numCorrect_positionLabel = numCorrect_positionLabel + 1;
                    numCorrect_position = numCorrect_position + 1;
                    markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{matchIndex}.corners, 3, curResultsData.markerNames_detected{matchIndex}(8:end)}; %#ok<AGROW>
                else
                    numCorrect_positionLabel = numCorrect_positionLabel + 1;
                    numCorrect_position = numCorrect_position + 1;
                    markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{matchIndex}.corners, 4, curResultsData.markerNames_detected{matchIndex}(8:end)}; %#ok<AGROW>
                end
            else
                numCorrect_position = numCorrect_position + 1;
                markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{matchIndex}.corners, 5, curResultsData.markerNames_detected{matchIndex}(8:end)}; %#ok<AGROW>
            end
        else
            numUndetected = numUndetected + 1;
        end
    end % for iMarker = 1:length(curResultsData.markerNames_groundTruth)
    
    clear matchIndex;
    
    % Look for spurrious detections
    allDetectedIndexes = unique(matchedIndexes);
    for iMarker = 1:length(curResultsData.markerNames_detected)
        if isempty(find(iMarker == allDetectedIndexes, 1))
            numSpurriousDetections = numSpurriousDetections + 1;
            markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{iMarker}.corners, 6, curResultsData.markerNames_detected{iMarker}(8:end)}; %#ok<AGROW>
        end
    end
end % compileMarkerResults()
