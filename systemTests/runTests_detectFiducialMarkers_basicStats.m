% function [resultsData, testPath, allTestFilenames, testFunctions, testFunctionNames] = runTests_detectFiducialMarkers_basicStats(markerDirectoryList, testJsonPattern, threadId, numThreads)

% threadId starts at zero

function [resultsData, testPath, allTestFilenames, testFunctions, testFunctionNames] = runTests_detectFiducialMarkers_basicStats(markerDirectoryList, testJsonPattern, threadId, numThreads)
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
    
    resultsData = cell(length(allTestFilenames), 1);
    testData = cell(length(allTestFilenames), 1);
    
    for iTest = 1:length(allTestFilenames)
%     for iTest = 10
%     for iTest = 2
        tic;
        
        jsonData = loadjson(allTestFilenames{iTest});
        
        jsonData.Poses = makeCellArray(jsonData.Poses);
                
        resultsData{iTest} = cell(length(jsonData.Poses), 1);
        testData{iTest} = cell(length(jsonData.Poses), 1);
        
        for iPose = (threadId+1):numThreads:length(jsonData.Poses)
            image = imread([testPath, jsonData.Poses{iPose}.ImageFile]);
            
            resultsData{iTest}{iPose} = cell(length(testFunctions), 1);
            
            if ~isfield(jsonData.Poses{iPose}, 'VisionMarkers')
                continue;
            end
            
            testData{iTest}{iPose}.Scene = jsonData.Poses{iPose}.Scene;
            testData{iTest}{iPose}.ImageFile = jsonData.Poses{iPose}.ImageFile;
            
            groundTruthQuads = jsonToQuad(jsonData.Poses{iPose}.VisionMarkers);
            
            groundTruthQuads = makeCellArray(groundTruthQuads);
                        
            for iTestFunction = 1:length(testFunctions)
                [detectedQuads, detectedQuadValidity, detectedMarkers] = testFunctions{iTestFunction}(image);
                
                %check if the quads are in the right places
                
                [justQuads_bestDistances_mean, justQuads_bestDistances_max, justQuads_bestIndexes, ~] = findClosestMatches(groundTruthQuads, detectedQuads, []);
                
                detectedMarkerQuads = makeCellArray(markersToQuad(detectedMarkers));
                                
                visionMarkers_groundTruth = makeCellArray(jsonData.Poses{iPose}.VisionMarkers);
                                
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
                
                %                 % Remove any markers with ground truth label MARKER_IGNORE
                %                 validInds = [];
                %                 for iMarker = 1:length(groundTruthQuads)
                %                     if ~strcmp(markerNames_groundTruth, 'MARKER_IGNORE')
                %                         validInds = [validInds, iMarker]; %#ok<AGROW>
                %                     end
                %                 end
                
                resultsData{iTest}{iPose}{iTestFunction}.justQuads_bestDistances_mean = justQuads_bestDistances_mean;
                resultsData{iTest}{iPose}{iTestFunction}.justQuads_bestDistances_max = justQuads_bestDistances_max;
                resultsData{iTest}{iPose}{iTestFunction}.justQuads_bestIndexes = justQuads_bestIndexes;
                resultsData{iTest}{iPose}{iTestFunction}.markers_bestDistances_mean = markers_bestDistances_mean;
                resultsData{iTest}{iPose}{iTestFunction}.markers_bestDistances_max = markers_bestDistances_max;
                resultsData{iTest}{iPose}{iTestFunction}.markers_bestIndexes = markers_bestIndexes;
                resultsData{iTest}{iPose}{iTestFunction}.markers_areRotationsCorrect = markers_areRotationsCorrect;
                resultsData{iTest}{iPose}{iTestFunction}.fiducialSizes_groundTruth = fiducialSizes_groundTruth;
                resultsData{iTest}{iPose}{iTestFunction}.markerNames_groundTruth = makeCellArray(markerNames_groundTruth);
                resultsData{iTest}{iPose}{iTestFunction}.markerLocations_groundTruth = makeCellArray(groundTruthQuads);
                
                resultsData{iTest}{iPose}{iTestFunction}.markerNames_detected = makeCellArray(markerNames_detected);
                resultsData{iTest}{iPose}{iTestFunction}.detectedQuads = makeCellArray(detectedQuads);
                resultsData{iTest}{iPose}{iTestFunction}.detectedQuadValidity = detectedQuadValidity;
                resultsData{iTest}{iPose}{iTestFunction}.detectedMarkers = makeCellArray(detectedMarkers);
                
                %             figure(1);
                %             hold off;
                %             imshow(image);
                %             hold on;
                %             for i = 1:length(detectedQuads)
                %                 plot(detectedQuads{i}(:,1), detectedQuads{i}(:,2));
                %                 text(detectedQuads{i}(1,1), detectedQuads{i}(1,2), sprintf('%d', bestIndexes(i)));
                %             end
                
                pause(.01);
            end
        end
        
        disp(sprintf('Finished test %d/%d in %f seconds', iTest, length(allTestFilenames), toc()));
        pause(.01);
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