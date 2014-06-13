% function allCompiledResults = runTests_detectFiducialMarkers(testJsonPattern, resultsFilename)

% allCompiledResults = runTests_detectFiducialMarkers('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_*.json', 'C:/Anki/systemTestImages/results/');

function allCompiledResults = runTests_detectFiducialMarkers(testJsonPattern, resultsDirectory)
    
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    % useUndistortion = false;
    
    % To be a match, all corners of a quad must be within these thresholds
    maxMatchDistance_pixels = 5;
    maxMatchDistance_percent = 0.2;
    
    numComputeThreads = 3; %#ok<NASGU>
    
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
    
    if showImageDetections
        figure(1);
        close 1
    end
    
    if recompileBasics
        tic
        
        partFilenameInput = sprintf('%s_input.mat', basicsFilename);
        save(partFilenameInput, 'markerDirectoryList', 'testJsonPattern');
        
        % launch threads
        for iThread = 0:(numComputeThreads-1)
            partFilename = sprintf('%s_outputPart%d.mat', basicsFilename, iThread);
            delete(partFilename);
            commandString = sprintf('matlab -nojvm -noFigureWindows -nosplash -r "load(''%s''); [resultsData_part, testPath, allTestFilenames, testFunctions, testFunctionNames] = runTests_detectFiducialMarkers_basicStats(markerDirectoryList, testJsonPattern, %d, %d); save(''%s'', ''resultsData_part'', ''testPath'', ''allTestFilenames'', ''testFunctions'', ''testFunctionNames''); exit;"', partFilenameInput, iThread, numComputeThreads, partFilename);
            system(['start /b ', commandString]);
        end
        
        % wait for threads to complete and compile results
        for iThread = 0:(numComputeThreads-1)
            partFilename = sprintf('%s_outputPart%d.mat', basicsFilename, iThread);
            
            while ~exist(partFilename, 'file')
                pause(.1);
            end
            
            % Wait for the file system to catch up or something?
            pause(5);
            
            load(partFilename)
            
            if iThread == 0
                resultsData = resultsData_part;
            else
                for iTest = 1:length(allTestFilenames)
                    resultsData{iTest}((iThread+1):numComputeThreads:end) = resultsData_part{iTest}((iThread+1):numComputeThreads:end);
                end
            end
        end
        
        disp(sprintf('Basic stat computation took %f seconds', toc()));
        
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
        %     for iTest = length(resultsData)
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
                
                %                 imwrite(image, outputFilenameImage);
                
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
                
                %                 if showImageDetections
                %                     figureHandle = figure(1);
                %
                %                     hold off;
                %
                %                     useImpixelinfo = false;
                %
                %                     imshow(imageWithBorder);
                %
                %                     if old_useImpixelinfo ~= -1
                %                         useImpixelinfo = old_useImpixelinfo;
                %                     else
                %                         clear useImpixelinfo;
                %                     end
                %
                %                     hold on;
                %                 end % if showImageDetections
                
%                 if ~(iTest == 6 && iPose == 2 && iTestFunction == 1)
%                     continue;
%                 else
%                     keyboard;
%                 end
                
                [curCompiled.numQuadsNotIgnored, curCompiled.numQuadsDetected] = compileQuadResults(curResultsData, showImageDetections, showImageDetectionsScale);
                
                [curCompiled.numCorrect_positionLabelRotation,...
                    curCompiled.numCorrect_positionLabel,...
                    curCompiled.numCorrect_position,...
                    curCompiled.numSpurriousDetections,...
                    curCompiled.numUndetected,...
                    markersToDisplay] = compileMarkerResults(curResultsData, showImageDetections, showImageDetectionsScale);
                
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
                
                %                 keyboard
                
                
                drawnImage = mexDrawSystemTestResults(uint8(image), curResultsData.detectedQuads, curResultsData.detectedQuadValidity, markersToDisplay(:,1), int32(cell2mat(markersToDisplay(:,2))), markersToDisplay(:,3), showImageDetectionsScale, outputFilenameResult, toShowResults);
                drawnImage = drawnImage(:,:,[3,2,1]);
                
                perTestStats{iTest}{iPose}{iTestFunction} = curCompiled;
                
                if showImageDetections
                    imshow(drawnImage)
                    pause(.03);
                end
                %                 if showImageDetections
                %                     resultsText = sprintf('Test %d %d %d\nDist:%dmm angle:%d expos:%0.1f light:%d %s\nmarkers:%d/%d/%d/%d/%d\nspurrious:%d',...
                %                         iTest, iPose, iTestFunction,...
                %                         jsonData.Poses{iPose}.Scene.Distance,...
                %                         jsonData.Poses{iPose}.Scene.angle,...
                %                         jsonData.Poses{iPose}.Scene.CameraExposure,...
                %                         jsonData.Poses{iPose}.Scene.light,...
                %                         testFunctionNames{iTestFunction},...
                %                         curCompiled.numCorrect_positionLabelRotation, curCompiled.numCorrect_positionLabel, curCompiled.numCorrect_position, curCompiled.numQuadsDetected, curCompiled.numQuadsNotIgnored,...
                %                         curCompiled.numSpurriousDetections);
                %
                %                     text(0, size(image,1)*showImageDetectionsScale, resultsText, 'Color', [.95,.95,.95], 'FontSize', 8*showImageDetectionsScale, 'VerticalAlignment', 'top');
                %
                %
                %                     set(figureHandle, 'PaperUnits', 'inches', 'PaperPosition', [0,0,size(imageWithBorder,2)/100,size(imageWithBorder,1)/100])
                %                     print(figureHandle, '-dpng', '-r100', outputFilenameResult)
                %
                %                     disp(resultsText);
                %                     disp(' ');
                %
                %                     pause(.03);
                %                     %                     pause();
                %                 end % if showImageDetections
            end % for iTestFunction = 1:length(perTestStats{iTest}{iPose})
        end % for iPose = 1:length(perTestStats{iTest})
    end % for iTest = 1:length(resultsData)
    
    outputFilenameNameLookup = [resultsDirectory, 'filenameLookup.txt'];
    
    fileId = fopen(outputFilenameNameLookup, 'w');
    fprintf(fileId, filenameNameLookup);
    fclose(fileId);
    
function [numNotIgnored, numGood] = compileQuadResults(curResultsData, showImageDetections, showImageDetectionsScale)
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    numTotal = length(curResultsData.justQuads_bestDistances_max);
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
    
    %     if showImageDetections
    %         for iQuad = 1:length(curResultsData.detectedQuads)
    %             plot(curResultsData.detectedQuads{iQuad}([1,2,4,3,1],1)*showImageDetectionsScale, curResultsData.detectedQuads{iQuad}([1,2,4,3,1],2)*showImageDetectionsScale, 'Color', [.95,.95,.95], 'LineWidth', 3); % .95 not 1, because printing converts pure white into pure black
    %             plot(curResultsData.detectedQuads{iQuad}([1,2,4,3,1],1)*showImageDetectionsScale, curResultsData.detectedQuads{iQuad}([1,2,4,3,1],2)*showImageDetectionsScale, 'Color', [0,0,0], 'LineWidth', 1);
    %
    %             if curResultsData.detectedQuadValidity(iQuad) > 0
    %                 minX = min(curResultsData.detectedQuads{iQuad}(:,1));
    %
    %                 indsX = find((minX+3) >= curResultsData.detectedQuads{iQuad}(:,1));
    %
    %                 if length(indsX) == 1
    %                     minY = min(curResultsData.detectedQuads{iQuad}(indsX(1),2));
    %                 else
    %                     minY = min(curResultsData.detectedQuads{iQuad}(indsX,2));
    %
    %                     indsY = find(minY == curResultsData.detectedQuads{iQuad}(indsX,2));
    %
    %                     minX = min(curResultsData.detectedQuads{iQuad}(indsX(indsY(1)),1));
    %                     minY = min(curResultsData.detectedQuads{iQuad}(indsX(indsY(1)),2));
    %                 end
    %
    %                 minX = minX*showImageDetectionsScale;
    %                 minY = minY*showImageDetectionsScale;
    %
    %                 rectangle('Position', [minX-2, minY-10, 15, 15], 'FaceColor', 'k');
    %
    %                 if curResultsData.detectedQuadValidity(iQuad) < 100000
    %                     validText = sprintf('%d', curResultsData.detectedQuadValidity(iQuad));
    %                     text(minX, minY, validText, 'Color', [.95,.95,.95], 'FontSize', 16.0);
    %                 end
    %             end % if curResultsData.detectedQuadValidity(iQuad) > 0
    %         end % for iQuad = 1:length(curResultsData.detectedQuads)
    %     end % if showImageDetections
    
function [numCorrect_positionLabelRotation, numCorrect_positionLabel, numCorrect_position, numSpurriousDetections, numUndetected, markersToDisplay] = compileMarkerResults(curResultsData, showImageDetections, showImageDetectionsScale)
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    numCorrect_positionLabelRotation = 0;
    numCorrect_positionLabel = 0;
    numCorrect_position = 0;
    numSpurriousDetections = 0;
    numUndetected = 0;
    
%     displayMarkerTypes = zeros([length(curResultsData.detectedMarkers), 1], 'int32');
    
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
                %                     corners = curResultsData.detectedMarkers{matchIndex}.corners;
                %                     plotText = 'IGNORE';
%                 assert(displayMarkerTypes(matchIndex) == 0);
%                 displayMarkerTypes(matchIndex) = 1;
                markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{matchIndex}.corners, 1, 'IGNORE'}; %#ok<AGROW>
                
                matchedIndexes(end+1) = matchIndex; %#ok<AGROW>
            else
                %                     corners = curResultsData.markerLocations_groundTruth{iMarker};
                %                     plotText = 'REJECT';
%                 assert(displayMarkerTypes(matchIndex) == 0);
%                 displayMarkerTypes(matchIndex) = 2;
                markersToDisplay(end+1,:) = {curResultsData.markerLocations_groundTruth{iMarker}, 2, 'REJECT'}; %#ok<AGROW>
            end
            
            if showImageDetections
                %                 drawOneMarker(corners, plotText, showImageDetectionsScale, [.7,.7,.7], 'k'); % 1
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
                    
                    %                     if showImageDetections
                    %                         drawOneMarker(curResultsData.detectedMarkers{matchIndex}.corners, curResultsData.markerNames_detected{matchIndex}(8:end), showImageDetectionsScale, 'g', 'k'); % 3
                    %                     end
%                     assert(displayMarkerTypes(matchIndex) == 0);
%                     displayMarkerTypes(matchIndex) = 3;
                    markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{matchIndex}.corners, 3, curResultsData.markerNames_detected{matchIndex}(8:end)}; %#ok<AGROW>
                else
                    numCorrect_positionLabel = numCorrect_positionLabel + 1;
                    numCorrect_position = numCorrect_position + 1;
                    
                    %                     if showImageDetections
                    %                         drawOneMarker(curResultsData.detectedMarkers{matchIndex}.corners, curResultsData.markerNames_detected{matchIndex}(8:end), showImageDetectionsScale, 'b', 'k'); % 4
                    %                     end
%                     assert(displayMarkerTypes(matchIndex) == 0);
%                     displayMarkerTypes(matchIndex) = 4;
                    markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{matchIndex}.corners, 4, curResultsData.markerNames_detected{matchIndex}(8:end)}; %#ok<AGROW>
                end
            else
                numCorrect_position = numCorrect_position + 1;
                
                %                 if showImageDetections
                %                     drawOneMarker(curResultsData.detectedMarkers{matchIndex}.corners, curResultsData.markerNames_detected{matchIndex}(8:end), showImageDetectionsScale, 'y', 'k'); % 5
                %                 end
%                 assert(displayMarkerTypes(matchIndex) == 0);
%                 displayMarkerTypes(matchIndex) = 5;
                markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{matchIndex}.corners, 5, curResultsData.markerNames_detected{matchIndex}(8:end)}; %#ok<AGROW>
            end
        else
            numUndetected = numUndetected + 1;
        end
    end % for iMarker = 1:length(curResultsData.markerNames_groundTruth)
    
    clear matchIndex;
    
    % Look for spurrious detections
%     allDetectedIndexes = unique(curResultsData.markers_bestIndexes);
    allDetectedIndexes = unique(matchedIndexes);
    for iMarker = 1:length(curResultsData.markerNames_detected)
        if isempty(find(iMarker == allDetectedIndexes, 1))
            numSpurriousDetections = numSpurriousDetections + 1;
            
            %             if showImageDetections
            %                 drawOneMarker(curResultsData.detectedMarkers{iMarker}.corners, curResultsData.markerNames_detected{iMarker}(8:end), showImageDetectionsScale, 'r', 'k'); % 6
            %             end
%             assert(displayMarkerTypes(iMarker) == 0);
%             displayMarkerTypes(iMarker) = 6;
            markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{iMarker}.corners, 6, curResultsData.markerNames_detected{iMarker}(8:end)}; %#ok<AGROW>
        end
    end
    
    % function drawOneMarker(corners, name, showImageDetectionsScale, quadColor, topBarColor)
    %     sortedXValues = sortrows([corners(:,1)'; 1:4]', 1);
    %
    %     firstCorner = sortedXValues(1,2);
    %     secondCorner = sortedXValues(2,2);
    %
    %     plot(corners([1,2,4,3,1],1)*showImageDetectionsScale, corners([1,2,4,3,1],2)*showImageDetectionsScale, 'Color', quadColor, 'LineWidth', 3);
    %     plot(corners([1,3],1)*showImageDetectionsScale, corners([1,3],2)*showImageDetectionsScale, 'Color', topBarColor, 'LineWidth', 3);
    %     midX = (corners(firstCorner,1) + corners(secondCorner,1)) / 2;
    %     midY = (corners(firstCorner,2) + corners(secondCorner,2)) / 2;
    %     text(midX*showImageDetectionsScale + 5, midY*showImageDetectionsScale, name, 'Color', quadColor);
    
function quads = markersToQuad(markers)
    if ~iscell(markers)
        markers = { markers };
    end
    
    quads = cell(length(markers),1);
    
    for iQuad = 1:length(markers)
        quads{iQuad} = markers{iQuad}.corners;
    end
    