% function runTests_detectFiducialMarkers_compilePerTestStats(resultsData, testPath, allTestFilenames, testFunctionNames, resultsDirectory, showImageDetections, showImageDetectionWidth)

function perTestStats = runTests_detectFiducialMarkers_compilePerTestStats(resultsData, testPath, allTestFilenames, testFunctionNames, resultsDirectory, showImageDetections, showImageDetectionWidth)
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
end % runTests_detectFiducialMarkers_compilePerTestStats()

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
    