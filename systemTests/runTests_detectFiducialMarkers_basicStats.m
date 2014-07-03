% function runTests_detectFiducialMarkers_basicStats()

% Don't call this function directly, instead call runTests_detectFiducialMarkers.m

% Set rotationList = getListOfSymmetricMarkers(markerDirectoryList);

function runTests_detectFiducialMarkers_basicStats(workQueue, allTestData, rotationList, algorithmParameters)
    global g_rotationList;
    
    g_rotationList = rotationList;
    
    lastTestId = -1;
    
    % Go through every item in the work list, and compute the accuracy of
    % the fiducial detection
    tic;
    for iWork = 1:length(workQueue)              
        if workQueue{iWork}.iTest ~= lastTestId
            if lastTestId ~= -1
                disp(sprintf('Finished test %d in %f seconds', lastTestId, toc()));
                tic
            end
            
            curTestData = allTestData{workQueue{iWork}.iTest};
            jsonData = curTestData.jsonData;
            jsonData.Poses = makeCellArray(jsonData.Poses);
            
            lastTestId = workQueue{iWork}.iTest;
        end
        
        image = imread([curTestData.testPath, jsonData.Poses{workQueue{iWork}.iPose}.ImageFile]);
        
        if ~isfield(jsonData.Poses{workQueue{iWork}.iPose}, 'VisionMarkers')
            assert(false);
            continue;
        end
        
        curTestData.Scene = jsonData.Poses{workQueue{iWork}.iPose}.Scene;
        curTestData.ImageFile = jsonData.Poses{workQueue{iWork}.iPose}.ImageFile;
        
        groundTruthQuads = jsonToQuad(jsonData.Poses{workQueue{iWork}.iPose}.VisionMarkers);
        
        groundTruthQuads = makeCellArray(groundTruthQuads);
        
        % Detect the markers
        [detectedQuads, detectedQuadValidity, detectedMarkers] = extractMarkers(image, algorithmParameters);

        %check if the quads are in the right places

        [justQuads_bestDistances_mean, justQuads_bestDistances_max, justQuads_bestIndexes, ~] = findClosestMatches(groundTruthQuads, detectedQuads, []);

        detectedMarkerQuads = makeCellArray(markersToQuad(detectedMarkers));

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

        curResultsData.justQuads_bestDistances_mean = justQuads_bestDistances_mean;
        curResultsData.justQuads_bestDistances_max = justQuads_bestDistances_max;
        curResultsData.justQuads_bestIndexes = justQuads_bestIndexes;
        curResultsData.markers_bestDistances_mean = markers_bestDistances_mean;
        curResultsData.markers_bestDistances_max = markers_bestDistances_max;
        curResultsData.markers_bestIndexes = markers_bestIndexes;
        curResultsData.markers_areRotationsCorrect = markers_areRotationsCorrect;
        curResultsData.fiducialSizes_groundTruth = fiducialSizes_groundTruth;
        curResultsData.markerNames_groundTruth = makeCellArray(markerNames_groundTruth);
        curResultsData.markerLocations_groundTruth = makeCellArray(groundTruthQuads);

        curResultsData.markerNames_detected = makeCellArray(markerNames_detected);
        curResultsData.detectedQuads = makeCellArray(detectedQuads);
        curResultsData.detectedQuadValidity = detectedQuadValidity;
        curResultsData.detectedMarkers = makeCellArray(detectedMarkers);
            
        save(workQueue{iWork}.basicStats_filename, 'curResultsData', 'curTestData');
        pause(.01);
    end % for iWork = 1:length(workQueue)
    
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
    jsonQuads = makeCellArray(jsonQuads);
    
    quads = cell(length(jsonQuads), 1);
    
    for iQuad = 1:length(jsonQuads)
        quads{iQuad} = [...
            jsonQuads{iQuad}.x_imgUpperLeft, jsonQuads{iQuad}.y_imgUpperLeft;
            jsonQuads{iQuad}.x_imgLowerLeft, jsonQuads{iQuad}.y_imgLowerLeft;
            jsonQuads{iQuad}.x_imgUpperRight, jsonQuads{iQuad}.y_imgUpperRight;
            jsonQuads{iQuad}.x_imgLowerRight, jsonQuads{iQuad}.y_imgLowerRight];
    end
    
function quads = markersToQuad(markers)
    markers = makeCellArray(markers);
    
    quads = cell(length(markers), 1);
    
    for iQuad = 1:length(markers)
        quads{iQuad} = markers{iQuad}.corners;
    end