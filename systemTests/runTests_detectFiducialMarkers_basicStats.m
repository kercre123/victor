% function runTests_detectFiducialMarkers_basicStats(testFunctions, rotationList, workList)

% Don't call this function directly, instead call runTests_detectFiducialMarkers.m

% Set rotationList = getListOfSymmetricMarkers(markerDirectoryList);

function runTests_detectFiducialMarkers_basicStats(testFunctions, rotationList, workList)
    global g_rotationList;
    
    g_rotationList = rotationList;
    
    lastTest = -1;
    
    % Go through every item in the work list, and compute the accuracy of
    % the fiducial detection
    tic;
    for iWork = 1:length(workList)
        curTestId = workList{iWork}{1};
        curTestFilename = workList{iWork}{2};
        curPoseId = workList{iWork}{3};
        curResultsFilename = workList{iWork}{4};
        
        if curTestId ~= lastTest
            if lastTest ~= -1
                disp(sprintf('Finished test %d in %f seconds', lastTest, toc()));
            end
            
            jsonData = loadjson(curTestFilename);
            jsonData.Poses = makeCellArray(jsonData.Poses);
            
            slashIndexes = strfind(curTestFilename, '/');
            testPath = curTestFilename(1:(slashIndexes(end)));
            
            lastTest = curTestId;
        end
        
        image = imread([testPath, jsonData.Poses{curPoseId}.ImageFile]);
        
        if ~isfield(jsonData.Poses{curPoseId}, 'VisionMarkers')
            continue;
        end
        
        curResultsData = cell(length(testFunctions), 1);
        
        curTestData.Scene = jsonData.Poses{curPoseId}.Scene;
        curTestData.ImageFile = jsonData.Poses{curPoseId}.ImageFile;
        
        groundTruthQuads = jsonToQuad(jsonData.Poses{curPoseId}.VisionMarkers);
        
        groundTruthQuads = makeCellArray(groundTruthQuads);
        
        for iTestFunction = 1:length(testFunctions)
            [detectedQuads, detectedQuadValidity, detectedMarkers] = testFunctions{iTestFunction}(image);
            
            %check if the quads are in the right places
            
            [justQuads_bestDistances_mean, justQuads_bestDistances_max, justQuads_bestIndexes, ~] = findClosestMatches(groundTruthQuads, detectedQuads, []);
            
            detectedMarkerQuads = makeCellArray(markersToQuad(detectedMarkers));
            
            visionMarkers_groundTruth = makeCellArray(jsonData.Poses{curPoseId}.VisionMarkers);
            
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
            
            curResultsData{iTestFunction}.justQuads_bestDistances_mean = justQuads_bestDistances_mean;
            curResultsData{iTestFunction}.justQuads_bestDistances_max = justQuads_bestDistances_max;
            curResultsData{iTestFunction}.justQuads_bestIndexes = justQuads_bestIndexes;
            curResultsData{iTestFunction}.markers_bestDistances_mean = markers_bestDistances_mean;
            curResultsData{iTestFunction}.markers_bestDistances_max = markers_bestDistances_max;
            curResultsData{iTestFunction}.markers_bestIndexes = markers_bestIndexes;
            curResultsData{iTestFunction}.markers_areRotationsCorrect = markers_areRotationsCorrect;
            curResultsData{iTestFunction}.fiducialSizes_groundTruth = fiducialSizes_groundTruth;
            curResultsData{iTestFunction}.markerNames_groundTruth = makeCellArray(markerNames_groundTruth);
            curResultsData{iTestFunction}.markerLocations_groundTruth = makeCellArray(groundTruthQuads);
            
            curResultsData{iTestFunction}.markerNames_detected = makeCellArray(markerNames_detected);
            curResultsData{iTestFunction}.detectedQuads = makeCellArray(detectedQuads);
            curResultsData{iTestFunction}.detectedQuadValidity = detectedQuadValidity;
            curResultsData{iTestFunction}.detectedMarkers = makeCellArray(detectedMarkers);
        end
        
        save(curResultsFilename, 'curResultsData', 'curTestData');
        
        pause(.01);
%         keyboard
    end % for iWork = 1:length(workList)
    
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