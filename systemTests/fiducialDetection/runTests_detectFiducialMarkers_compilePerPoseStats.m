% function runTests_detectFiducialMarkers_compilePerPoseStats()

function runTests_detectFiducialMarkers_compilePerPoseStats(workQueue, allTestData, resultsData_basics, maxMatchDistance_pixels, maxMatchDistance_percent, showImageDetections, showImageDetectionWidth)
    % TODO: At different distances / angles
    
    global useImpixelinfo;
    global g_maxMatchDistance_pixels;
    global g_maxMatchDistance_percent;
    
    if exist('useImpixelinfo', 'var')
        old_useImpixelinfo = useImpixelinfo; %#ok<NASGU>
    else
        old_useImpixelinfo = -1; %#ok<NASGU>
    end
    
    g_maxMatchDistance_pixels = maxMatchDistance_pixels;
    g_maxMatchDistance_percent = maxMatchDistance_percent;
    
    perPoseStats = cell(length(resultsData_basics), 1);
    
    lastTestId = -1;
    
    tic
    
    for iWork = 1:length(workQueue)
        if workQueue{iWork}.iTest ~= lastTestId
            if lastTestId ~= -1
                %                 disp(sprintf('Compiled test results %d/%d in %f seconds', lastTestId, length(resultsData_basics), toc()));
                disp(sprintf(' Finished in %0.4f seconds', toc()));
                tic
            end
            
            perPoseStats{workQueue{iWork}.iTest} = cell(length(resultsData_basics{workQueue{iWork}.iTest}), 1);
            
            curTestData = allTestData{workQueue{iWork}.iTest};
            jsonData = curTestData.jsonData;
            
            lastTestId = workQueue{iWork}.iTest;
            
            fprintf('Starting perPose %d ', lastTestId);
        end
        
        perPoseStats{workQueue{iWork}.iTest}{workQueue{iWork}.iPose} = cell(length(resultsData_basics{workQueue{iWork}.iTest}{workQueue{iWork}.iPose}), 1);
        
        imageFilename = [curTestData.testPath, jsonData.Poses{workQueue{iWork}.iPose}.ImageFile];
        imageFilename = strrep(imageFilename, '//', '/');
        image = rgb2gray2(imread(imageFilename));
        
        if showImageDetectionWidth(1) ~= size(image,2)
            showImageDetectionsScale = showImageDetectionWidth(1) / size(image,2);
        else
            showImageDetectionsScale = 1;
        end
        
        curResultsData = resultsData_basics{workQueue{iWork}.iTest}{workQueue{iWork}.iPose};
       
        curResultsData_perPose.uncompressedFileSize = curResultsData.uncompressedFileSize;
        curResultsData_perPose.compressedFileSize = curResultsData.compressedFileSize;
        
        curResultsData_perPose.Scene = allTestData{workQueue{iWork}.iTest}.jsonData.Poses{workQueue{iWork}.iPose}.Scene;
       
        [curResultsData_perPose.numMarkersGroundTruth, curResultsData_perPose.numQuadsDetected] = compileQuadResults(curResultsData);
        
        [curResultsData_perPose.numCorrect_positionLabelRotation,...
            curResultsData_perPose.numCorrect_positionLabel,...
            curResultsData_perPose.numCorrect_position,...
            curResultsData_perPose.numSpurriousDetections,...
            curResultsData_perPose.numUndetected,...
            markersToDisplay] = compileMarkerResults(curResultsData);
        
        toShowResults = {
            workQueue{iWork}.iTest - 1,...
            workQueue{iWork}.iPose - 1,...
            curResultsData_perPose.Scene.Distance,...
            curResultsData_perPose.Scene.Angle,...
            curResultsData_perPose.Scene.CameraExposure,...
            curResultsData_perPose.Scene.Light,...
            workQueue{iWork}.extractionFunctionName,...
            curResultsData_perPose.numCorrect_positionLabelRotation,...
            curResultsData_perPose.numCorrect_positionLabel,...
            curResultsData_perPose.numCorrect_position,...
            curResultsData_perPose.numQuadsDetected,...
            curResultsData_perPose.numMarkersGroundTruth,...
            curResultsData_perPose.numSpurriousDetections};
        
        for iMarker = 1:length(markersToDisplay(:,3))
            assert(~iscell(markersToDisplay{iMarker,3}));
        end
        
        drawnImage = mexDrawSystemTestResults(uint8(image), curResultsData.detectedQuads, curResultsData.detectedQuadValidity, markersToDisplay(:,1), int32(cell2mat(markersToDisplay(:,2))), markersToDisplay(:,3), showImageDetectionsScale, workQueue{iWork}.perPoseStats_imageFilename, toShowResults);
        drawnImage = drawnImage(:,:,[3,2,1]);
        
        perPoseStats{workQueue{iWork}.iTest}{workQueue{iWork}.iPose} = curResultsData_perPose;
        
        save(workQueue{iWork}.perPoseStats_dataFilename, 'curResultsData_perPose');
        
        if showImageDetections
            imshow(drawnImage)
            pause(.03);
        end
        
        fprintf('.');
        pause(.01);        
    end % for iWork = 1:length(workQueue)
end % runTests_detectFiducialMarkers_compileperPoseStats()

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
            assert(~iscell(curResultsData.markerNames_detected{iMarker}));
            
            numSpurriousDetections = numSpurriousDetections + 1;            
            markersToDisplay(end+1,:) = {curResultsData.detectedMarkers{iMarker}.corners, 6, curResultsData.markerNames_detected{iMarker}(8:end)}; %#ok<AGROW>
        end
    end
end % compileMarkerResults()
