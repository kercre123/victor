% function allCompiledResults = runTests_detectFiducialMarkers(testJsonPattern, resultsFilename)

% allCompiledResults = runTests_detectFiducialMarkers('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_*.json', 'C:/Anki/systemTestImages/results/');

function allCompiledResults = runTests_detectFiducialMarkers(testJsonPattern, resultsDirectory)
    
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    % useUndistortion = false;
    
    % To be a match, all corners of a quad must be within these thresholds
    maxMatchDistance_pixels = 5;
    maxMatchDistance_percent = 0.2;
    
    showImageDetections = false;
    showImageDetectionWidth = 640;
    showOverallStats = true;
    
    recompileBasics = false;
    recompilePerTestStats = false;
    
    basicsFilename = 'basicsResults.mat';
    perTestStatsFilename = 'perTestStatsResults.mat';
        
    markerDirectoryList = {'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/symbols/withFiducials/', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/letters/withFiducials', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/dice/withFiducials'};
    
    assert(exist('testJsonPattern', 'var') == 1);
    assert(exist('resultsDirectory', 'var') == 1);
    
    if showImageDetections
        figure(1);
        close 1
    end
    
    if recompileBasics
        [resultsData, testPath, allTestFilenames, testFunctions, testFunctionNames] = computeBasics(markerDirectoryList, testJsonPattern);
        save(basicsFilename, 'resultsData', 'testPath', 'allTestFilenames', 'testFunctions', 'testFunctionNames');
    else
        load(basicsFilename);
    end
    
    if recompilePerTestStats
        perTestStats = compilePerTestStats(resultsData, testPath, allTestFilenames, testFunctionNames, resultsDirectory, showImageDetections, showImageDetectionWidth);
        
        save(perTestStatsFilename, 'perTestStats');
    else 
        load(perTestStatsFilename);
    end
        
    overallStats = compileOverallStats(allTestFilenames, perTestStats, showOverallStats);
    
    save([resultsDirectory, 'allCompiledResults.mat'], 'perTestStats');
    
    keyboard
    
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
        perTestStats{iTest} = cell(length(resultsData{iTest}), 1);
        
        if showImageDetections
            jsonData = loadjson(allTestFilenames{iTest});
        end
        
        for iPose = 1:length(perTestStats{iTest})
            perTestStats{iTest}{iPose} = cell(length(resultsData{iTest}{iPose}), 1);
            
            if showImageDetections
                imageFilename = [testPath, jsonData.Poses{iPose}.ImageFile];
                imageFilename = strrep(imageFilename, '//', '/');
                image = imread(imageFilename);
                
                outputFilenameImage = [resultsDirectory, sprintf('detection_dist%d_angle%d_expose%0.1f_light%d.png',...
                    jsonData.Poses{iPose}.Scene.Distance,...
                    jsonData.Poses{iPose}.Scene.angle,...
                    jsonData.Poses{iPose}.Scene.CameraExposure,...
                    jsonData.Poses{iPose}.Scene.light)];
                
                imwrite(image, outputFilenameImage);

                slashIndexes = strfind(outputFilenameImage, '/');
                outputFilenameImageJustName = outputFilenameImage((slashIndexes(end)+1):end);
                slashIndexes = strfind(imageFilename, '/');
                imageFilenameJustName = imageFilename((slashIndexes(end)+1):end);
                
                filenameNameLookup = [filenameNameLookup, sprintf('%d %d \t%s \t%s\n', iTest, iPose, outputFilenameImageJustName, imageFilenameJustName)]; %#ok<AGROW>
                
                imageWithBorder = zeros([size(image,1) + 50, size(image,2)], 'uint8');
                imageWithBorder(1:size(image,1), :) = image;
                
                if showImageDetectionWidth(1) ~= size(image,2)
                    showImageDetectionsScale = showImageDetectionWidth(1) / size(image,2);
                    imageWithBorder = imresize(imageWithBorder, size(imageWithBorder)*showImageDetectionsScale, 'nearest');
                else
                    showImageDetectionsScale = 1;
                end
            end % if showImageDetections
            
            for iTestFunction = 1:length(perTestStats{iTest}{iPose})
                curResultsData = resultsData{iTest}{iPose}{iTestFunction};
                
                if showImageDetections
                    figureHandle = figure(1);
                    
                    hold off;
                    
                    useImpixelinfo = false;
                    
                    imshow(imageWithBorder);
                    
                    if old_useImpixelinfo ~= -1
                        useImpixelinfo = old_useImpixelinfo;
                    else
                        clear useImpixelinfo;
                    end
                    
                    hold on;
                end % if showImageDetections
                
                [curCompiled.numQuadsTotal, curCompiled.numQuadsDetected] = compileQuadResults(curResultsData, showImageDetections, showImageDetectionsScale);
                
                [curCompiled.numCorrect_positionLabelRotation, curCompiled.numCorrect_positionLabel, curCompiled.numCorrect_position, curCompiled.numSpurriousDetections, curCompiled.numUndetected] = compileMarkerResults(curResultsData, showImageDetections, showImageDetectionsScale);
                
                perTestStats{iTest}{iPose}{iTestFunction} = curCompiled;
                
                if showImageDetections
                    resultsText = sprintf('Test %d %d %d\nDist:%dmm angle:%d expos:%0.1f light:%d %s\nmarkers:%d/%d/%d/%d/%d spurrious:%d',...
                        iTest, iPose, iTestFunction,...
                        jsonData.Poses{iPose}.Scene.Distance,...
                        jsonData.Poses{iPose}.Scene.angle,...
                        jsonData.Poses{iPose}.Scene.CameraExposure,...
                        jsonData.Poses{iPose}.Scene.light,...
                        testFunctionNames{iTestFunction},...
                        curCompiled.numCorrect_positionLabelRotation, curCompiled.numCorrect_positionLabel, curCompiled.numCorrect_position, curCompiled.numQuadsDetected, curCompiled.numQuadsTotal,...
                        curCompiled.numSpurriousDetections);
                    
                    text(0, size(image,1)*showImageDetectionsScale, resultsText, 'Color', [.95,.95,.95], 'FontSize', 8*showImageDetectionsScale, 'VerticalAlignment', 'top');
                    
                    outputFilenameResult = [resultsDirectory, sprintf('detection_dist%d_angle%d_expose%0.1f_light%d_%s.png',...
                        jsonData.Poses{iPose}.Scene.Distance,...
                        jsonData.Poses{iPose}.Scene.angle,...
                        jsonData.Poses{iPose}.Scene.CameraExposure,...
                        jsonData.Poses{iPose}.Scene.light,...
                        testFunctionNames{iTestFunction})];
                    
                    set(figureHandle, 'PaperUnits', 'inches', 'PaperPosition', [0,0,size(imageWithBorder,2)/100,size(imageWithBorder,1)/100])
                    print(figureHandle, '-dpng', '-r100', outputFilenameResult)
                    
                    disp(resultsText);
                    disp(' ');
                    
                    pause(.03)
                end % if showImageDetections                
            end % for iTestFunction = 1:length(perTestStats{iTest}{iPose})
        end % for iPose = 1:length(perTestStats{iTest})
    end % for iTest = 1:length(resultsData)
    
    outputFilenameNameLookup = [resultsDirectory, 'filenameLookup.txt'];
    
    fileId = fopen(outputFilenameNameLookup, 'w');
    fprintf(fileId, filenameNameLookup);
    fclose(fileId);
    
function [numTotal, numGood] = compileQuadResults(curResultsData, showImageDetections, showImageDetectionsScale)
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    numTotal = length(curResultsData.justQuads_bestDistances_max);
    numGood = 0;
    
    for iQuad = 1:numTotal
        curDistance = curResultsData.justQuads_bestDistances_max(iQuad,:);
        curFiducialSize = curResultsData.fiducialSizes(iQuad,:);
        
        % Are all distances small enough?
        if curDistance(1) <= maxMatchDistance_pixels &&...
                curDistance(2) <= maxMatchDistance_pixels &&...
                curDistance(3) <= maxMatchDistance_pixels &&...
                curDistance(2) <= (curFiducialSize(1) * maxMatchDistance_percent) &&...
                curDistance(3) <= (curFiducialSize(2) * maxMatchDistance_percent)
            
            numGood = numGood + 1;
        end
    end % for iQuad = 1:numTotal
    
    if showImageDetections
        for iQuad = 1:length(curResultsData.detectedQuads)
            plot(curResultsData.detectedQuads{iQuad}([1,2,4,3,1],1)*showImageDetectionsScale, curResultsData.detectedQuads{iQuad}([1,2,4,3,1],2)*showImageDetectionsScale, 'Color', [.95,.95,.95], 'LineWidth', 3); % .95 not 1, because printing converts pure white into pure black
            plot(curResultsData.detectedQuads{iQuad}([1,2,4,3,1],1)*showImageDetectionsScale, curResultsData.detectedQuads{iQuad}([1,2,4,3,1],2)*showImageDetectionsScale, 'Color', [0,0,0], 'LineWidth', 1);
        end
    end % if showImageDetections
    
function [numCorrect_positionLabelRotation, numCorrect_positionLabel, numCorrect_position, numSpurriousDetections, numUndetected] = compileMarkerResults(curResultsData, showImageDetections, showImageDetectionsScale)
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    numCorrect_positionLabelRotation = 0;
    numCorrect_positionLabel = 0;
    numCorrect_position = 0;
    numSpurriousDetections = 0;
    numUndetected = 0;
    
    % Check all ground truth markers
    for iMarker = 1:length(curResultsData.markerNames_groundTruth)
        curDistance = curResultsData.markers_bestDistances_max(iMarker,:);
        curFiducialSize = curResultsData.fiducialSizes(iMarker,:);
        
        % Are all distances small enough?
        if curDistance(1) <= maxMatchDistance_pixels &&...
                curDistance(2) <= maxMatchDistance_pixels &&...
                curDistance(3) <= maxMatchDistance_pixels &&...
                curDistance(2) <= (curFiducialSize(1) * maxMatchDistance_percent) &&...
                curDistance(3) <= (curFiducialSize(2) * maxMatchDistance_percent)
            
            matchIndex = curResultsData.markers_bestIndexes(iMarker);
            if strcmp(curResultsData.markerNames_groundTruth{iMarker}, curResultsData.markerNames_detected{matchIndex})
                if curResultsData.markers_areRotationsCorrect(iMarker)
                    numCorrect_positionLabelRotation = numCorrect_positionLabelRotation + 1;
                    numCorrect_positionLabel = numCorrect_positionLabel + 1;
                    numCorrect_position = numCorrect_position + 1;
                    
                    if showImageDetections
                        plotOneMarker(curResultsData.detectedMarkers{matchIndex}.corners, curResultsData.markerNames_detected{matchIndex}(8:end), showImageDetectionsScale, 'g', 'k');
                    end
                else
                    numCorrect_positionLabel = numCorrect_positionLabel + 1;
                    numCorrect_position = numCorrect_position + 1;
                    
                    if showImageDetections
                        plotOneMarker(curResultsData.detectedMarkers{matchIndex}.corners, curResultsData.markerNames_detected{matchIndex}(8:end), showImageDetectionsScale, 'b', 'k');
                    end
                end
            else
                numCorrect_position = numCorrect_position + 1;
                
                if showImageDetections
                    plotOneMarker(curResultsData.detectedMarkers{matchIndex}.corners, curResultsData.markerNames_detected{matchIndex}(8:end), showImageDetectionsScale, 'y', 'k');
                end
            end
        else
            numUndetected = numUndetected + 1;
        end
    end
    
    % Look for spurrious detections
    allDetectedIndexes = unique(curResultsData.markers_bestIndexes);
    for iMarker = 1:length(curResultsData.markerNames_detected)
        if isempty(find(iMarker == allDetectedIndexes, 1))
            numSpurriousDetections = numSpurriousDetections + 1;
            
            if showImageDetections
                plotOneMarker(curResultsData.detectedMarkers{iMarker}.corners, curResultsData.markerNames_detected{iMarker}(8:end), showImageDetectionsScale, 'r', 'k');
            end
        end
    end
    
function plotOneMarker(corners, name, showImageDetectionsScale, quadColor, topBarColor)
    sortedXValues = sortrows([corners(:,1)'; 1:4]', 1);
    
    firstCorner = sortedXValues(1,2);
    secondCorner = sortedXValues(2,2);
    
    plot(corners([1,2,4,3,1],1)*showImageDetectionsScale, corners([1,2,4,3,1],2)*showImageDetectionsScale, quadColor, 'LineWidth', 3);
    plot(corners([1,3],1)*showImageDetectionsScale, corners([1,3],2)*showImageDetectionsScale, topBarColor, 'LineWidth', 3);
    midX = (corners(firstCorner,1) + corners(secondCorner,1)) / 2;
    midY = (corners(firstCorner,2) + corners(secondCorner,2)) / 2;
    text(midX*showImageDetectionsScale + 5, midY*showImageDetectionsScale, name, 'Color', quadColor);
    
function [resultsData, testPath, allTestFilenames, testFunctions, testFunctionNames] = computeBasics(markerDirectoryList, testJsonPattern)
    global rotationList;
    
    rotationList = getListOfSymmetricMarkers(markerDirectoryList);
    
    % if useUndistortion
    %     load('Z:\Documents\Box Documents\Cozmo SE\calibCozmoProto1_head.mat');
    %     cam = Camera('calibration', calibCozmoProto1_head);
    % end
    
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
        @extractMarkers_matlabOriginal_withRefinement};
    
    testFunctionNames = {...
        'c-noRef',...
        'c-ref',...
        'matlab-noRef',...
        'matlab-ref'};
    
    resultsData = cell(length(allTestFilenames), 1);
    testData = cell(length(allTestFilenames), 1);
    
    for iTest = 1:length(allTestFilenames)
        %     for iTest = length(allTestFilenames)
        tic;
        
        jsonData = loadjson(allTestFilenames{iTest});
        
        if ~iscell(jsonData.Poses)
            jsonData.Poses = { jsonData.Poses };
        end
        
        resultsData{iTest} = cell(length(jsonData.Poses), 1);
        testData{iTest} = cell(length(jsonData.Poses), 1);
        
        for iPose = 1:length(jsonData.Poses)
            image = imread([testPath, jsonData.Poses{iPose}.ImageFile]);
            
            resultsData{iTest}{iPose} = cell(length(testFunctions), 1);
            
            if ~isfield(jsonData.Poses{iPose}, 'VisionMarkers')
                continue;
            end
            
            testData{iTest}{iPose}.Scene = jsonData.Poses{iPose}.Scene;
            testData{iTest}{iPose}.ImageFile = jsonData.Poses{iPose}.ImageFile;
            
            groundTruthQuads = jsonToQuad(jsonData.Poses{iPose}.VisionMarkers);
            
            for iTestFunction = 1:length(testFunctions)
                [detectedQuads, detectedMarkers] = testFunctions{iTestFunction}(image);
                
                %check if the quads are in the right places
                
                [justQuads_bestDistances_mean, justQuads_bestDistances_max, justQuads_bestIndexes, ~] = findClosestMatches(groundTruthQuads, detectedQuads, []);
                
                [markers_bestDistances_mean, markers_bestDistances_max, markers_bestIndexes, markers_areRotationsCorrect] = findClosestMatches(groundTruthQuads, markersToQuad(detectedMarkers), jsonData.Poses{iPose}.VisionMarkers);
                
                markerNames_groundTruth = cell(length(groundTruthQuads), 1);
                fiducialSizes = zeros(length(groundTruthQuads), 2);
                
                for iMarker = 1:length(groundTruthQuads)
                    markerNames_groundTruth{iMarker,1} = jsonData.Poses{iPose}.VisionMarkers{iMarker}.markerType;
                    
                    fiducialSizes(iMarker,:) = [...
                        max(groundTruthQuads{iMarker}(:,1)) - min(groundTruthQuads{iMarker}(:,1)),...
                        max(groundTruthQuads{iMarker}(:,2)) - min(groundTruthQuads{iMarker}(:,2))];
                    
                    %                 markerNames_groundTruth{iMarker,1} = detectedMarkers{markers_bestIndexes(iMarker)}.name;
                    %                 markerNames_groundTruth{iMarker,2} = jsonData.Poses{iPose}.VisionMarkers{iMarker}.markerType;%
                end
                
                markerNames_detected = cell(length(detectedMarkers), 1);
                for iMarker = 1:length(detectedMarkers)
                    markerNames_detected{iMarker} = detectedMarkers{iMarker}.name;
                end
                
                resultsData{iTest}{iPose}{iTestFunction}.justQuads_bestDistances_mean = justQuads_bestDistances_mean;
                resultsData{iTest}{iPose}{iTestFunction}.justQuads_bestDistances_max = justQuads_bestDistances_max;
                resultsData{iTest}{iPose}{iTestFunction}.justQuads_bestIndexes = justQuads_bestIndexes;
                resultsData{iTest}{iPose}{iTestFunction}.markers_bestDistances_mean = markers_bestDistances_mean;
                resultsData{iTest}{iPose}{iTestFunction}.markers_bestDistances_max = markers_bestDistances_max;
                resultsData{iTest}{iPose}{iTestFunction}.markers_bestIndexes = markers_bestIndexes;
                resultsData{iTest}{iPose}{iTestFunction}.markers_areRotationsCorrect = markers_areRotationsCorrect;
                resultsData{iTest}{iPose}{iTestFunction}.fiducialSizes = fiducialSizes;
                resultsData{iTest}{iPose}{iTestFunction}.markerNames_groundTruth = markerNames_groundTruth;
                resultsData{iTest}{iPose}{iTestFunction}.markerNames_detected = markerNames_detected;
                resultsData{iTest}{iPose}{iTestFunction}.detectedQuads = detectedQuads;
                resultsData{iTest}{iPose}{iTestFunction}.detectedMarkers = detectedMarkers;
                
                %             figure(1);
                %             hold off;
                %             imshow(image);
                %             hold on;
                %             for i = 1:length(detectedQuads)
                %                 plot(detectedQuads{i}(:,1), detectedQuads{i}(:,2));
                %                 text(detectedQuads{i}(1,1), detectedQuads{i}(1,2), sprintf('%d', bestIndexes(i)));
                %             end
            end
        end
        
        disp(sprintf('Finished test %d/%d in %f seconds', iTest, length(allTestFilenames), toc()));
    end % for iTest = 1:length(allTestFilenames)
    
function [bestDistances_mean, bestDistances_max, bestIndexes, areRotationsCorrect] = findClosestMatches(groundTruthQuads, queryQuads, markerNames_groundTruth)
    global rotationList;
    
    bestDistances_mean = Inf * ones(length(groundTruthQuads), 1);
    bestDistances_max = Inf * ones(length(groundTruthQuads), 3);
    bestIndexes = -1 * ones(length(groundTruthQuads), 1);
    areRotationsCorrect = false * ones(length(groundTruthQuads), 1);
    
    for iGroundTruth = 1:length(groundTruthQuads)
        gtQuad = groundTruthQuads{iGroundTruth};
        
        if isempty(markerNames_groundTruth)
            numRotations = 4;
        else
            curName = markerNames_groundTruth{iGroundTruth}.markerType(8:end);
            
            index = find(strcmp(rotationList(:,1), curName));
            
            % TODO: make assert false
            %             assert(~isempty(index));
            if isempty(index)
                numRotations = 4;
            else
                numRotations = rotationList{index, 2};
            end
            
            %             disp(sprintf('%s %d', curName, numRotations));
        end
        
        for iQuery = 1:length(queryQuads)
            queryQuad = queryQuads{iQuery};
            
            for iRotation = 0:3
                distances = sqrt(sum((queryQuad - gtQuad).^2, 2));
                
                % Compute the closest match based on the mean distance
                distance_mean = mean(distances);
                
                if distance_mean < bestDistances_mean(iGroundTruth)
                    bestDistances_mean(iGroundTruth) = distance_mean;
                    bestDistances_max(iGroundTruth, :) = [max(distances), max(abs(queryQuad - gtQuad))];
                    bestIndexes(iGroundTruth) = iQuery;
                    
                    if numRotations == 4
                        if iRotation == 0
                            areRotationsCorrect(iGroundTruth) = true;
                        else
                            areRotationsCorrect(iGroundTruth) = false;
                        end
                    elseif numRotations == 2
                        if iRotation == 0 || iRotation == 2
                            areRotationsCorrect(iGroundTruth) = true;
                        else
                            areRotationsCorrect(iGroundTruth) = false;
                        end
                    elseif numRotations == 1
                        areRotationsCorrect(iGroundTruth) = true;
                    else
                        assert(false);
                    end
                end
                
                % rotate the quad
                queryQuad = queryQuad([3,1,4,2],:);
            end
        end
    end
    
function quads = jsonToQuad(jsonQuads)
    if ~iscell(jsonQuads)
        jsonQuads = { jsonQuads };
    end
    
    quads = cell(length(jsonQuads), 1);
    
    for iQuad = 1:length(jsonQuads)
        quads{iQuad} = [...
            jsonQuads{iQuad}.x_imgUpperLeft, jsonQuads{iQuad}.y_imgUpperLeft;
            jsonQuads{iQuad}.x_imgLowerLeft, jsonQuads{iQuad}.y_imgLowerLeft;
            jsonQuads{iQuad}.x_imgUpperRight, jsonQuads{iQuad}.y_imgUpperRight;
            jsonQuads{iQuad}.x_imgLowerRight, jsonQuads{iQuad}.y_imgLowerRight];
    end
    
function quads = markersToQuad(markers)
    if ~iscell(markers)
        markers = { markers };
    end
    
    quads = cell(length(markers), 1);
    
    for iQuad = 1:length(markers)
        quads{iQuad} = markers{iQuad}.corners;
    end
