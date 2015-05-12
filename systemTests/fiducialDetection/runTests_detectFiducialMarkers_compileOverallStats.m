function resultsData_overall = runTests_detectFiducialMarkers_compileOverallStats(resultsData_perPose, showPlots)
    %#ok<*FNDSB>
    
    sceneTypes = getSceneTypes(resultsData_perPose);
    
    numCameraExposures = length(sceneTypes.CameraExposures);
    numDistances = length(sceneTypes.Distances);
    numAngles = length(sceneTypes.Angles);
    numLights = length(sceneTypes.Lights);
    
    [groundTruth_events, quadExtraction_events, markerDetection_events, fileSizes] = extractEvents(resultsData_perPose, sceneTypes);
    
    if showPlots
        for iAngle = 1:numAngles
            for iLight = 1:numLights
                
                quadExtractionPercents = zeros(numCameraExposures, numDistances);
                markerCorrectPercents = zeros(numCameraExposures, numDistances);
                markerErrorCounts = zeros(numCameraExposures, numDistances);
                for iCameraExposure = 1:numCameraExposures
                    for iDistance = 1:numDistances
                        numMarkersGroundTruth = squeeze(groundTruth_events(iCameraExposure, iDistance, iAngle, iLight));
                        
                        quadExtraction_numQuadsDetected = squeeze(quadExtraction_events(iCameraExposure, iDistance, iAngle, iLight));
                        quadExtractionPercents(iCameraExposure, iDistance) = quadExtraction_numQuadsDetected / numMarkersGroundTruth;
                        
                        % TODO: show graphs of the marker detection statistics
                        markerDetection_numTotallyCorrect = squeeze(markerDetection_events(iCameraExposure, iDistance, iAngle, iLight, 1));
                        markerCorrectPercents(iCameraExposure, iDistance) = markerDetection_numTotallyCorrect / numMarkersGroundTruth;
                        
                        markerErrorCounts(iCameraExposure, iDistance) = ...
                            squeeze(markerDetection_events(iCameraExposure, iDistance, iAngle, iLight, 4)) +...
                            (squeeze(markerDetection_events(iCameraExposure, iDistance, iAngle, iLight, 3)) - squeeze(markerDetection_events(iCameraExposure, iDistance, iAngle, iLight, 1)));
                    end
                end
                
                figure(iLight);
                subplot(1,numAngles,iAngle);
                bar(sceneTypes.Distances, quadExtractionPercents')
                shading flat
                a = axis();
                a(4) = 1.0;
                axis(a);
                title(sprintf('Angle:%d Light%d', sceneTypes.Angles(iAngle), sceneTypes.Lights(iLight)));
            end
        end
    end % if showPlots
    
    
    
    % 1. Bar of exposure, distance. Graphs for angle and light.
    
    % 2. Line of distance, each line of exposure
    
    % 3. Bar of angle, distance.
    
    resultsData_overall.percentQuadsExtracted = sum(sum(sum(sum(quadExtraction_events(:,:,:,:))))) / sum(sum(sum(sum(groundTruth_events(:,:,:,:)))));
    
    resultsData_overall.percentMarkersCorrect = sum(sum(sum(sum(markerDetection_events(:,:,:,:,1))))) / sum(sum(sum(sum(groundTruth_events(:,:,:,:)))));
    
    resultsData_overall.numMarkersCorrect = sum(sum(sum(sum(markerDetection_events(:,:,:,:,1)))));
    
    % Number of spurrios detections, plus 
    % number of detections with the correct position, minus number totally correct
    resultsData_overall.numMarkerErrors = ...
        sum(sum(sum(sum(markerDetection_events(:,:,:,:,4))))) + ...
        ( sum(sum(sum(sum(markerDetection_events(:,:,:,:,3))))) - sum(sum(sum(sum(markerDetection_events(:,:,:,:,1))))));
    
    resultsData_overall.uncompressedFileSizeTotal = sum(sum(sum(sum(fileSizes(:,:,:,:,1)))));
    resultsData_overall.compressedFileSizeTotal = sum(sum(sum(sum(fileSizes(:,:,:,:,2)))));
    
    %     keyboard
end % runTests_detectFiducialMarkers_compileOverallStats()

% function [quadExtraction_means, quadExtraction_variances] = computeStatistics(cellOfEvents)
%     quadExtraction_means = zeros(size(cellOfEvents));
%     quadExtraction_variances = zeros(size(cellOfEvents));
%
%     for iList = 1:size(cellOfEvents,1)
%         for jList = 1:size(cellOfEvents,2)
%             curEvents = cellOfEvents{iList,jList};
%
%             quadExtraction_percents = zeros(length(curEvents), 1);
%             for iEvent = 1:length(curEvents)
%                 quadExtraction_percents(iEvent) = curEvents(iEvent).numQuadsDetected / curEvents(iEvent).numMarkersGroundTruth;
%             end
%
%             quadExtraction_means(iList,jList) = mean(quadExtraction_percents);
%             quadExtraction_variances(iList,jList) = var(quadExtraction_percents);
%         end
%     end
%
% end % computeStatistics()

function [groundTruth_events, quadExtraction_events, markerDetection_events, fileSizes] = extractEvents(resultsData_perPose, sceneTypes)
    groundTruth_events = zeros(length(sceneTypes.CameraExposures), length(sceneTypes.Distances), length(sceneTypes.Angles), length(sceneTypes.Lights));
    quadExtraction_events = zeros(length(sceneTypes.CameraExposures), length(sceneTypes.Distances), length(sceneTypes.Angles), length(sceneTypes.Lights));
    markerDetection_events = zeros(length(sceneTypes.CameraExposures), length(sceneTypes.Distances), length(sceneTypes.Angles), length(sceneTypes.Lights), 5);
    fileSizes = zeros(length(sceneTypes.CameraExposures), length(sceneTypes.Distances), length(sceneTypes.Angles), length(sceneTypes.Lights), 2);
    
    for iTest = 1:length(resultsData_perPose)
        for iPose = 1:length(resultsData_perPose{iTest})
            curResult = resultsData_perPose{iTest}{iPose};
            
            curCameraExposure = curResult.Scene.CameraExposure;
            curDistance = curResult.Scene.Distance;
            curAngle = curResult.Scene.Angle;
            curLight = curResult.Scene.Light;
            
            for iCameraExposures = 1:length(sceneTypes.CameraExposures)
                if curCameraExposure == sceneTypes.CameraExposures(iCameraExposures)
                    for iDistances = 1:length(sceneTypes.Distances)
                        if curDistance == sceneTypes.Distances(iDistances)
                            for iAngles = 1:length(sceneTypes.Angles)
                                if curAngle == sceneTypes.Angles(iAngles)
                                    for iLights = 1:length(sceneTypes.Lights)
                                        if curLight == sceneTypes.Lights(iLights)
                                            groundTruth_events(iCameraExposures, iDistances, iAngles, iLights) = groundTruth_events(iCameraExposures, iDistances, iAngles, iLights) + curResult.numMarkersGroundTruth;
                                            
                                            quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights) = quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights) + curResult.numQuadsDetected;
                                            
                                            markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 1) = markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 1) + curResult.numCorrect_positionLabelRotation;
                                            markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 2) = markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 2) + curResult.numCorrect_positionLabel;
                                            markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 3) = markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 3) + curResult.numCorrect_position;
                                            markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 4) = markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 4) + curResult.numSpurriousDetections;
                                            markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 5) = markerDetection_events(iCameraExposures, iDistances, iAngles, iLights, 5) + curResult.numUndetected;
                                            
                                            fileSizes(iCameraExposures, iDistances, iAngles, iLights, 1) = fileSizes(iCameraExposures, iDistances, iAngles, iLights, 1) + curResult.uncompressedFileSize;
                                            fileSizes(iCameraExposures, iDistances, iAngles, iLights, 2) = fileSizes(iCameraExposures, iDistances, iAngles, iLights, 2) + curResult.compressedFileSize;
                                        end
                                    end % for iLights = 1:length(sceneTypes.Lights)
                                end
                            end % for iAngles = 1:length(sceneTypes.Angles)
                        end
                    end % for iDistances = 1:length(sceneTypes.Distances)
                end
            end % for iCameraExposures = 1:length(sceneTypes.CameraExposures)
        end % for iPose = 1:length(resultsData_perPose{iTest})
    end % for iTest = 1:length(resultsData_perPose)
end % computeStatistics()

function sceneTypes = getSceneTypes(resultsData_perPose)
    sceneTypes.CameraExposures = [];
    sceneTypes.Distances = [];
    sceneTypes.Angles = [];
    sceneTypes.Lights = [];
    
    for iTest = 1:length(resultsData_perPose)
        for iPose = 1:length(resultsData_perPose{iTest})
            curScene = resultsData_perPose{iTest}{iPose}.Scene;
            if isempty(find(sceneTypes.CameraExposures == curScene.CameraExposure, 1))
                sceneTypes.CameraExposures(end+1) = curScene.CameraExposure;
            end
            
            if isempty(find(sceneTypes.Distances == curScene.Distance, 1))
                sceneTypes.Distances(end+1) = curScene.Distance;
            end
            
            if isempty(find(sceneTypes.Angles == curScene.Angle, 1))
                sceneTypes.Angles(end+1) = curScene.Angle;
            end
            
            if isempty(find(sceneTypes.Lights == curScene.Light, 1))
                sceneTypes.Lights(end+1) = curScene.Light;
            end
        end
    end
    
    sceneTypes.CameraExposures = sort(sceneTypes.CameraExposures);
    sceneTypes.Distances = sort(sceneTypes.Distances);
    sceneTypes.Angles = sort(sceneTypes.Angles);
    sceneTypes.Lights = sort(sceneTypes.Lights);
end % getSceneTypes()
