% function runTests_detectFiducialMarkers_basicStats()

% Don't call this function directly, instead call runTests_detectFiducialMarkers.m

% Set rotationList = getListOfSymmetricMarkers(markerDirectoryList);

function runTests_detectFiducialMarkers_basicStats(workQueue, allTestData, rotationList, algorithmParameters)
    global g_rotationList;
    
    useImageMagick = false;
    
    g_rotationList = rotationList;
    
    lastTestId = -1;
    
    % Go through every item in the work list, and compute the accuracy of
    % the fiducial detection
    tic;
    for iWork = 1:length(workQueue)
        if workQueue{iWork}.iTest ~= lastTestId
            if lastTestId ~= -1
                disp(sprintf(' Finished in %0.4f seconds', toc()));
                tic
            end
            
            curTestData = allTestData{workQueue{iWork}.iTest};
            jsonData = curTestData.jsonData;
            jsonData.Poses = makeCellArray(jsonData.Poses);
            
            lastTestId = workQueue{iWork}.iTest;
            
            fprintf('Starting basicStats %d ', lastTestId);
        end
        
        [image, fileInfo] = testUtilities_loadImage(...
            [curTestData.testPath, jsonData.Poses{workQueue{iWork}.iPose}.ImageFile],...
            algorithmParameters.imageCompression,...
            algorithmParameters.preprocessingFunction,...
            workQueue{iWork}.basicStats_filename);

        if ~isfield(jsonData.Poses{workQueue{iWork}.iPose}, 'VisionMarkers')
            keyboard;
            continue;
        end
        
        curTestData.Scene = jsonData.Poses{workQueue{iWork}.iPose}.Scene;
        
        curTestData.ImageFile = jsonData.Poses{workQueue{iWork}.iPose}.ImageFile;
        
        groundTruthQuads = testUtilities_jsonToQuad(jsonData.Poses{workQueue{iWork}.iPose}.VisionMarkers);
        
        groundTruthQuads = makeCellArray(groundTruthQuads);
        
        % Detect the markers
        [detectedQuads, detectedQuadValidity, detectedMarkers] = extractMarkers(image, algorithmParameters);
        
        %check if the quads are in the right places
        
        [justQuads_bestDistances_mean, justQuads_bestDistances_max, justQuads_bestIndexes, ~] = findClosestMatches(groundTruthQuads, detectedQuads, []);
        
        detectedMarkerQuads = makeCellArray(testUtilities_markersToQuad(detectedMarkers));
        
        visionMarkers_groundTruth = makeCellArray(jsonData.Poses{workQueue{iWork}.iPose}.VisionMarkers);
        
        [markers_bestDistances_mean, markers_bestDistances_max, markers_bestIndexes, markers_areRotationsCorrect] = findClosestMatches(groundTruthQuads, detectedMarkerQuads, visionMarkers_groundTruth);
        
        markerNames_groundTruth = cell(length(groundTruthQuads), 1);
        fiducialSizes_groundTruth = zeros(length(groundTruthQuads), 2);
        
        for iMarker = 1:length(groundTruthQuads)
            markerNames_groundTruth{iMarker,1} = visionMarkers_groundTruth{iMarker}.markerType;
            
            fiducialSizes_groundTruth(iMarker,:) = [...
                max(groundTruthQuads{iMarker}(:,1)) - min(groundTruthQuads{iMarker}(:,1)),...
                max(groundTruthQuads{iMarker}(:,2)) - min(groundTruthQuads{iMarker}(:,2))];
        end
        
        markerNames_detected = cell(length(detectedMarkers), 1);
        for iMarker = 1:length(detectedMarkers)
            markerNames_detected{iMarker} = detectedMarkers{iMarker}.name;
        end
        
        curResultsData_basics.justQuads_bestDistances_mean = justQuads_bestDistances_mean;
        curResultsData_basics.justQuads_bestDistances_max = justQuads_bestDistances_max;
        curResultsData_basics.justQuads_bestIndexes = justQuads_bestIndexes;
        curResultsData_basics.markers_bestDistances_mean = markers_bestDistances_mean;
        curResultsData_basics.markers_bestDistances_max = markers_bestDistances_max;
        curResultsData_basics.markers_bestIndexes = markers_bestIndexes;
        curResultsData_basics.markers_areRotationsCorrect = markers_areRotationsCorrect;
        curResultsData_basics.fiducialSizes_groundTruth = fiducialSizes_groundTruth;
        curResultsData_basics.markerNames_groundTruth = makeCellArray(markerNames_groundTruth);
        curResultsData_basics.markerLocations_groundTruth = makeCellArray(groundTruthQuads);
        
        curResultsData_basics.markerNames_detected = makeCellArray(markerNames_detected);
        curResultsData_basics.detectedQuads = makeCellArray(detectedQuads);
        curResultsData_basics.detectedQuadValidity = detectedQuadValidity;
        curResultsData_basics.detectedMarkers = makeCellArray(detectedMarkers);
        
        curResultsData_basics.uncompressedFileSize = numel(image);
        curResultsData_basics.compressedFileSize = fileInfo.bytes;
        
        save(workQueue{iWork}.basicStats_filename, 'curResultsData_basics', 'curTestData');
        
        fprintf('.');
        pause(.01);
    end % for iWork = 1:length(workQueue)
end % runTests_detectFiducialMarkers_basicStats()    
    
function [bestDistances_mean, bestDistances_max, bestIndexes, areRotationsCorrect] = findClosestMatches(groundTruthQuads, queryQuads, markerNames_groundTruth)
    global g_rotationList;
    
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
            
            index = find(strcmp(g_rotationList(:,1), curName));
            
            % TODO: make assert false
            %             assert(~isempty(index));
            if isempty(index)
                numRotations = 4;
            else
                numRotations = g_rotationList{index, 2};
            end
            
            %             disp(sprintf('%s %d', curName, numRotations));
        end
        
        for iQuery = 1:length(queryQuads)
            queryQuad = queryQuads{iQuery};
            
            for iRotation = 0:3
                %                 distances = sqrt(sum((queryQuad - gtQuad).^2, 2));
                distances = sqrt((queryQuad(:,1) - gtQuad(:,1)).^2 + (queryQuad(:,2) - gtQuad(:,2)).^2);
                
                % Compute the closest match based on the mean distance
                
                % distance_mean = mean(distances);
                distance_mean = (distances(1) + distances(2) + distances(3) + distances(4)) / 4;
                
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
            end % for iRotation = 0:3
        end % for iQuery = 1:length(queryQuads)
    end % for iGroundTruth = 1:length(groundTruthQuads)
end % findClosestMatches()
