% function runTests_tracking_basicStats()

% Don't call this function directly, instead call runTests_detectFiducialMarkers.m

% Set rotationList = getListOfSymmetricMarkers(markerDirectoryList);

function runTests_tracking_basicStats(workQueue, allTestData, rotationList, algorithmParameters)
    global g_rotationList;
    
    g_rotationList = rotationList;
    
    lastTestId = -1;
        
    % Go through every item in the work list, and compute the accuracy of
    % the tracking algorithm
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
            
            trackerIsValid = true;
            
            fprintf('Starting basicStats %d ', lastTestId);
        end
        
        templateQuad = testUtilities_jsonToQuad(jsonData.Poses{1}.VisionMarkers{1});
        templateQuad = templateQuad{1};
        
        for iPose = workQueue{iWork}.iPoses
            if iPose == 1
                pixelShift = [0, 0];
            elseif strcmp(algorithmParameters.shaking{1}, 'horizontal')
                if mod(iPose, 2) == 0
                    pixelShift = [algorithmParameters.shaking{2}, 0];
                else
                    pixelShift = [-algorithmParameters.shaking{2}, 0];
                end
            end
            
            [image, fileInfo] = testUtilities_loadImage(...
                [curTestData.testPath, jsonData.Poses{iPose}.ImageFile],...
                algorithmParameters.imageCompression,...
                algorithmParameters.preprocessingFunction,...
                workQueue{iWork}.basicStats_filenames{iPose},...
                'pixelShift', pixelShift);
            
            if strcmp(algorithmParameters.grayvalueShift{1}, 'uniformPercent')
                % This is saturating addition/subtraction
                image = double(image);
                
                if mod(iPose, 2) == 0
                    image = image * (1.0 + algorithmParameters.grayvalueShift{2});
                else
                    image = image * (1.0 - algorithmParameters.grayvalueShift{2});
                end
                
                image = uint8(image);
%                 figure(10); imshow(image);
%                 pause(.03);
            end
            
            if size(image,1) ~= jsonData.CameraCalibration.nrows || size(image,2) ~= jsonData.CameraCalibration.ncols
                disp(sprintf('Size mismatch %dx%d != %dx%d', size(image,2), size(image,1), jsonData.CameraCalibration.ncols, jsonData.CameraCalibration.nrows));
                keyboard
            end
            
            if iPose ~= 1 && trackerIsValid
                if strcmp(algorithmParameters.whichAlgorithm, 'c_6dof')
                    [verify_converged,...
                        verify_meanAbsoluteDifference,...
                        verify_numInBounds,...
                        verify_numSimilarPixels,...
                        currentQuad,...
                        angleX,...
                        angleY,...
                        angleZ,...
                        tX,...
                        tY,...
                        tZ,...
                        tform] = mexPlanar6dofTrack(...
                        image,...
                        algorithmParameters.track_maxIterations,...
                        algorithmParameters.track_convergenceTolerance_angle,...
                        algorithmParameters.track_convergenceTolerance_distance,...
                        algorithmParameters.track_verify_maxPixelDifference,...
                        algorithmParameters.normalizeImage);
                elseif strcmp(algorithmParameters.whichAlgorithm, 'matlab_klt')
                    currentQuad = trackQuadWithKLT(previousImage, currentQuad, image);
                    previousImage = image;
                else
                    keyboard
                    assert(false);
                end
            end
            
            if iPose == 1
                currentQuad = templateQuad;
            elseif algorithmParameters.track_maxSnapToClosestQuadDistance > 0 && trackerIsValid
                currentQuad = snapToClosestQuads(image, currentQuad, algorithmParameters);
            end
            
            if trackerIsValid && (iPose == 1 || algorithmParameters.track_maxSnapToClosestQuadDistance > 0)
                trackerIsValid = false;
                if strcmp(algorithmParameters.whichAlgorithm, 'c_6dof')
                    try
                        [samples,...
                            angleX,...
                            angleY,...
                            angleZ,...
                            tX,...
                            tY,...
                            tZ] = mexPlanar6dofTrack(...
                            image,...
                            currentQuad,...
                            jsonData.CameraCalibration.focalLength_x,...
                            jsonData.CameraCalibration.focalLength_y,...
                            jsonData.CameraCalibration.center_x,...
                            jsonData.CameraCalibration.center_y,...
                            workQueue{iWork}.templateWidth_mm,...
                            algorithmParameters.init_scaleTemplateRegionPercent,...
                            algorithmParameters.init_numPyramidLevels,...
                            algorithmParameters.init_maxSamplesAtBaseLevel,...
                            algorithmParameters.init_numSamplingRegions,...
                            algorithmParameters.init_numFiducialSquareSamples,...
                            algorithmParameters.init_fiducialSquareWidthFraction,...
                            algorithmParameters.normalizeImage);
                        
                        trackerIsValid = true;
                    catch exception
                        disp(getReport(exception));
                        trackerIsValid = false;
                        currentQuad = [30,20; 20,20; 30,30; 20,30];
                    end % try ... catch
                elseif strcmp(algorithmParameters.whichAlgorithm, 'matlab_klt')
                    previousImage = image;
                    trackerIsValid = true;
                else
                    keyboard
                    assert(false);
                end
            end % if trackerIsValid && (iPose == 1 || algorithmParameters.track_maxSnapToClosestQuadDistance > 0)
            
            if ~isfield(jsonData.Poses{iPose}, 'VisionMarkers')
                keyboard;
                continue;
            end
            
            curTestData.Scene = jsonData.Poses{iPose}.Scene;
            
            curTestData.ImageFile = jsonData.Poses{iPose}.ImageFile;
            
            groundTruthQuads = testUtilities_jsonToQuad(jsonData.Poses{iPose}.VisionMarkers);
            
            groundTruthQuads = makeCellArray(groundTruthQuads);
            
            currentQuadShifted = currentQuad;
            currentQuadShifted(:,1) = currentQuadShifted(:,1) - pixelShift(1);
            currentQuadShifted(:,2) = currentQuadShifted(:,2) - pixelShift(2);
            
            detectedQuads = {currentQuadShifted};
            detectedQuadValidity = int32([1]);
            detectedMarkers = {};
            detectedMarkers{1}.name = jsonData.Poses{1}.VisionMarkers{1}.markerType;
            detectedMarkers{1}.isValid = 1;
            detectedMarkers{1}.corners = currentQuadShifted;
            
            %check if the quads are in the right places
            
            [justQuads_bestDistances_mean, justQuads_bestDistances_max, justQuads_bestIndexes, ~] = findClosestMatches(groundTruthQuads, detectedQuads, []);
            
            detectedMarkerQuads = makeCellArray(testUtilities_markersToQuad(detectedMarkers));
            
            visionMarkers_groundTruth = makeCellArray(jsonData.Poses{iPose}.VisionMarkers);
            
            [markers_bestDistances_mean, markers_bestDistances_max, markers_bestIndexes, markers_areRotationsCorrect] = findClosestMatches(groundTruthQuads, detectedMarkerQuads, visionMarkers_groundTruth);
            
            % TODO: fix rotations check
            markers_areRotationsCorrect(:) = 1;
            
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
            
            save(workQueue{iWork}.basicStats_filenames{iPose}, 'curResultsData_basics', 'curTestData');
            
            if mod(iPose,10) == 0
                fprintf('x');
            else
                fprintf('.');
            end
            
            pause(.01);
        end % for iPose = workQueue{iWork}.iPoses
    end % for iWork = 1:length(workQueue)
end % runTests_tracking_basicStats()

function snappedQuad = snapToClosestQuads(image, rawQuad, algorithmParameters)
    
    % If any of the tracked corners are out of bounds, don't snap
    if min(rawQuad(:,1)) <= 0 || min(rawQuad(:,2)) <= 0 || max(rawQuad(:,1)) > size(image,2) || min(rawQuad(:,1)) > size(image,1)
        snappedQuad = rawQuad;
        return;
    end
    
    [detectedQuads, ~, ~] = extractMarkers(image, algorithmParameters.detection);
    
    closestQuadInd = -1;
    closestQuadDist = Inf;
    
    for iQuad = 1:length(detectedQuads)
        curQuad = detectedQuads{iQuad};
        
        sumCornerDists = 0;
        
        % For quad iQuad, what is the sum of the best distances between all four corners?
        for iCorner = 1:4
            %             closestCornerInd = -1;
            closestCornerDist = Inf;
            
            for jCorner = 1:4
                curCornerDist = sqrt(...
                    (curQuad(jCorner,1) - rawQuad(iCorner,1)) ^ 2 +...
                    (curQuad(jCorner,2) - rawQuad(iCorner,2)) ^ 2);
                
                if curCornerDist < closestCornerDist
                    closestCornerDist = curCornerDist;
                    %                     closestCornerInd = jCorner;
                end
            end % for jCorner = 1:4
            
            sumCornerDists = sumCornerDists + closestCornerDist;
        end % for iCorner = 1:4
        
        if sumCornerDists < closestQuadDist
            closestQuadDist = sumCornerDists;
            closestQuadInd = iQuad;
        end
    end % for iQuad = 1:length(detectedQuads)
    
    if closestQuadDist <= algorithmParameters.track_maxSnapToClosestQuadDistance
        snappedQuad = detectedQuads{closestQuadInd};
        
        % Rotate the best quad to match the original
        bestRotatedQuad = [];
        bestRotatedDistance = Inf;
        
        snappedQuadRotated = snappedQuad;
        for iRotation = 1:4
            totalDist = 0;
            for iCorner = 1:4
                totalDist = totalDist + sqrt(...
                    (snappedQuadRotated(iCorner,1) - rawQuad(iCorner,1))^2 +...
                    (snappedQuadRotated(iCorner,2) - rawQuad(iCorner,2))^2);
            end
                        
            if totalDist < bestRotatedDistance
                bestRotatedDistance = totalDist;
                bestRotatedQuad = snappedQuadRotated;
            end
            
            snappedQuadRotated = [snappedQuadRotated(2,:); snappedQuadRotated(4,:); snappedQuadRotated(1,:); snappedQuadRotated(3,:)];
        end % for iRotation = 1:4
        
%         if min(snappedQuad == bestRotatedQuad) ~= 1
%             keyboard
%         end
        
        snappedQuad = bestRotatedQuad;
    else
        snappedQuad = rawQuad;
    end
end % snapToClosestQuads()

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

