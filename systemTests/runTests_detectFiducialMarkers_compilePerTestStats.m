% function runTests_detectFiducialMarkers_compilePerTestStats()

function perTestStats = runTests_detectFiducialMarkers_compilePerTestStats(workQueue, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth)
    % TODO: At different distances / angles
    
    global useImpixelinfo;
    global g_maxMatchDistance_pixels;
    global g_maxMatchDistance_percent;
    
    if exist('useImpixelinfo', 'var')
        old_useImpixelinfo = useImpixelinfo;
    else
        old_useImpixelinfo = -1;
    end
    
    g_maxMatchDistance_pixels = maxMatchDistance_pixels;
    g_maxMatchDistance_percent = maxMatchDistance_percent;
    
    perTestStats = cell(length(resultsData_basics), 1);
    
    lastTestId = -1;
    
    filenameNameLookup = '';
    
    tic
    
    for iWork = 1:length(workQueue)        
        if workQueue{iWork}.iTest ~= lastTestId
            if lastTestId ~= -1
                disp(sprintf('Compiled test results %d/%d in %f seconds', lastTestId, length(resultsData_basics), toc()));
                tic
            end
            
            perTestStats{workQueue{iWork}.iTest} = cell(length(resultsData_basics{workQueue{iWork}.iTest}), 1);
            
            curTestData = allTestData{workQueue{iWork}.iTest};
            jsonData = curTestData.jsonData;
                        
            lastTestId = workQueue{iWork}.iTest;
        end
        
        perTestStats{workQueue{iWork}.iTest}{workQueue{iWork}.iPose} = cell(length(resultsData_basics{workQueue{iWork}.iTest}{workQueue{iWork}.iPose}), 1);
        
        imageFilename = [curTestData.testPath, jsonData.Poses{workQueue{iWork}.iPose}.ImageFile];
        imageFilename = strrep(imageFilename, '//', '/');
        image = imread(imageFilename);
        
%         outputFilenameImage = [resultsDirectory, sprintf('detection_dist%d_angle%d_expose%0.1f_light%d.png',...
%             jsonData.Poses{workQueue{iWork}.iPose}.Scene.Distance,...
%             jsonData.Poses{workQueue{iWork}.iPose}.Scene.angle,...
%             jsonData.Poses{workQueue{iWork}.iPose}.Scene.CameraExposure,...
%             jsonData.Poses{workQueue{iWork}.iPose}.Scene.light)];

%         % Create the text lookup between file names
%         slashIndexes = strfind(workQueue{iWork}.perTestStats_imageFilename, '/');
%         outputFilenameImageJustName = workQueue{iWork}.perTestStats_imageFilename((slashIndexes(end)+1):end);        
%         slashIndexes = strfind(jsonData.Poses{workQueue{iWork}.iPose}.ImageFile, '/');
%         imageFilenameJustName = jsonData.Poses{workQueue{iWork}.iPose}.ImageFile((slashIndexes(end)+1):end);        
%         filenameNameLookup = [filenameNameLookup, sprintf('%d %d \t%s \t%s\n', workQueue{iWork}.iTest, workQueue{iWork}.iPose, outputFilenameImageJustName, imageFilenameJustName)]; %#ok<AGROW>
        
        if showImageDetectionWidth(1) ~= size(image,2)
            showImageDetectionsScale = showImageDetectionWidth(1) / size(image,2);
        else
            showImageDetectionsScale = 1;
        end
        
        curResultsData = resultsData_basics{workQueue{iWork}.iTest}{workQueue{iWork}.iPose};

        [curCompiledData.numQuadsNotIgnored, curCompiledData.numQuadsDetected] = compileQuadResults(curResultsData);

        [curCompiledData.numCorrect_positionLabelRotation,...
            curCompiledData.numCorrect_positionLabel,...
            curCompiledData.numCorrect_position,...
            curCompiledData.numSpurriousDetections,...
            curCompiledData.numUndetected,...
            markersToDisplay] = compileMarkerResults(curResultsData);

%             outputFilenameResult = [resultsDirectory, sprintf('result_%03d%03d%03d_dist%d_angle%d_expose%0.1f_light%d_%s.png',...
%                 workQueue{iWork}.iTest, workQueue{iWork}.iPose, iTestFunction,...
%                 jsonData.Poses{workQueue{iWork}.iPose}.Scene.Distance,...
%                 jsonData.Poses{workQueue{iWork}.iPose}.Scene.angle,...
%                 jsonData.Poses{workQueue{iWork}.iPose}.Scene.CameraExposure,...
%                 jsonData.Poses{workQueue{iWork}.iPose}.Scene.light,...
%                 testFunctionNames{iTestFunction})];

        toShowResults = {
            workQueue{iWork}.iTest,...
            workQueue{iWork}.iPose,...
            0,... % TODO: remove iTestFunction
            jsonData.Poses{workQueue{iWork}.iPose}.Scene.Distance,...
            jsonData.Poses{workQueue{iWork}.iPose}.Scene.angle,...
            jsonData.Poses{workQueue{iWork}.iPose}.Scene.CameraExposure,...
            jsonData.Poses{workQueue{iWork}.iPose}.Scene.light,...
            ' ',... % TODO: add the test function name
            curCompiledData.numCorrect_positionLabelRotation,...
            curCompiledData.numCorrect_positionLabel,...
            curCompiledData.numCorrect_position,...
            curCompiledData.numQuadsDetected,...
            curCompiledData.numQuadsNotIgnored,...
            curCompiledData.numSpurriousDetections};

        drawnImage = mexDrawSystemTestResults(uint8(image), curResultsData.detectedQuads, curResultsData.detectedQuadValidity, markersToDisplay(:,1), int32(cell2mat(markersToDisplay(:,2))), markersToDisplay(:,3), showImageDetectionsScale, workQueue{iWork}.perTestStats_imageFilename, toShowResults);
        drawnImage = drawnImage(:,:,[3,2,1]);

        perTestStats{workQueue{iWork}.iTest}{workQueue{iWork}.iPose} = curCompiledData;
        
        save(workQueue{iWork}.perTestStats_dataFilename, 'curCompiledData');

        if showImageDetections
            imshow(drawnImage)
            pause(.03);
        end
        
    end % for iWork = 1:length(workQueue)
    
%     outputFilenameNameLookup = [resultsDirectory, 'filenameLookup.txt'];    
%     fileId = fopen(outputFilenameNameLookup, 'w');
%     fprintf(fileId, filenameNameLookup);
%     fclose(fileId);
end % runTests_detectFiducialMarkers_compilePerTestStats()

function [numNotIgnored, numGood] = compileQuadResults(curResultsData)
    global g_maxMatchDistance_pixels;
    global g_maxMatchDistance_percent;
    
    numTotal = length(curResultsData.markerNames_groundTruth);
    numNotIgnored = 0;
    numGood = 0;
    
    for iQuad = 1:numTotal
        if ~strcmp(curResultsData.markerNames_groundTruth{iQuad}, 'MARKER_IGNORE')
            numNotIgnored = numNotIgnored + 1;
            
            curDistance = curResultsData.justQuads_bestDistances_max(iQuad,:);
            curFiducialSize = curResultsData.fiducialSizes_groundTruth(iQuad,:);
            
            % Are all distances small enough?
            if curDistance(1) <= g_maxMatchDistance_pixels &&...
                    curDistance(2) <= g_maxMatchDistance_pixels &&...
                    curDistance(3) <= g_maxMatchDistance_pixels &&...
                    curDistance(2) <= (curFiducialSize(1) * g_maxMatchDistance_percent) &&...
                    curDistance(3) <= (curFiducialSize(2) * g_maxMatchDistance_percent)
                
                numGood = numGood + 1;
            end
        end
    end % for iQuad = 1:numTotal
end % compileQuadResults()

function [numCorrect_positionLabelRotation, numCorrect_positionLabel, numCorrect_position, numSpurriousDetections, numUndetected, markersToDisplay] = compileMarkerResults(curResultsData)
    global g_maxMatchDistance_pixels;
    global g_maxMatchDistance_percent;
    
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
            
            if curDistance(1) <= g_maxMatchDistance_pixels &&...
                    curDistance(2) <= g_maxMatchDistance_pixels &&...
                    curDistance(3) <= g_maxMatchDistance_pixels &&...
                    curDistance(2) <= (curFiducialSize(1) * g_maxMatchDistance_percent) &&...
                    curDistance(3) <= (curFiducialSize(2) * g_maxMatchDistance_percent)
                
                matchIndex = curResultsData.markers_bestIndexes(iMarker);
                markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{matchIndex}.corners, 1, 'IGNORE'}; %#ok<AGROW>
                
                matchedIndexes(end+1) = matchIndex; %#ok<AGROW>
            else
                markersToDisplay(end+1,:) = {curResultsData.markerLocations_groundTruth{iMarker}, 2, 'REJECT'}; %#ok<AGROW>
            end
            
            continue;
        end
        
        % Are all distances small enough?
        if curDistance(1) <= g_maxMatchDistance_pixels &&...
                curDistance(2) <= g_maxMatchDistance_pixels &&...
                curDistance(3) <= g_maxMatchDistance_pixels &&...
                curDistance(2) <= (curFiducialSize(1) * g_maxMatchDistance_percent) &&...
                curDistance(3) <= (curFiducialSize(2) * g_maxMatchDistance_percent)
            
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
