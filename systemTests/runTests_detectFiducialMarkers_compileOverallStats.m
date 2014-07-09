function resultsData_overall = runTests_detectFiducialMarkers_compileOverallStats(resultsData_perPose)
    %#ok<*FNDSB>
    
    sceneTypes = getSceneTypes(resultsData_perPose);
    
    quadExtraction_events = extractEvents(resultsData_perPose, sceneTypes);
    
    % 1. Bar of exposure, distance. Graphs for angle and light.
    
    % 2. Line of distance, each line of exposure
    
    % 3. Bar of angle, distance.
    
    
    keyboard
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
%                 quadExtraction_percents(iEvent) = curEvents(iEvent).numQuadsDetected / curEvents(iEvent).numQuadsNotIgnored;
%             end
%
%             quadExtraction_means(iList,jList) = mean(quadExtraction_percents);
%             quadExtraction_variances(iList,jList) = var(quadExtraction_percents);
%         end
%     end
%
% end % computeStatistics()

function quadExtraction_events = extractEvents(resultsData_perPose, sceneTypes)
    quadExtraction_events = zeros(length(sceneTypes.cameraExposures), length(sceneTypes.distances), length(sceneTypes.angles), length(sceneTypes.lights), 2);
    
    for iTest = 1:length(resultsData_perPose)
        for iPose = 1:length(resultsData_perPose{iTest})
            curResult = resultsData_perPose{iTest}{iPose};
            
            for iCameraExposures = 1:length(sceneTypes.cameraExposures)
                for iDistances = 1:length(sceneTypes.distances)
                    for iAngles = 1:length(sceneTypes.angles)
                        for iLights = 1:length(sceneTypes.lights)
                            if curResult.Scene.CameraExposure == sceneTypes.cameraExposures(iCameraExposures) &&...
                                    curResult.Scene.Distance == sceneTypes.distances(iDistances) &&...
                                    curResult.Scene.angle == sceneTypes.angles(iAngles) &&...
                                    curResult.Scene.light == sceneTypes.lights(iLights)
                                
                                quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights, 1) = quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights, 1) + curResult.numQuadsDetected;
                                quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights, 2) = quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights, 2) + curResult.numQuadsNotIgnored;
                            end
                        end
                    end
                end
            end
        end
    end
    
end % computeStatistics()

function sceneTypes = getSceneTypes(resultsData_perPose)
    sceneTypes.cameraExposures = [];
    sceneTypes.distances = [];
    sceneTypes.angles = [];
    sceneTypes.lights = [];
    
    for iTest = 1:length(resultsData_perPose)
        for iPose = 1:length(resultsData_perPose{iTest})
            curScene = resultsData_perPose{iTest}{iPose}.Scene;
            if isempty(find(sceneTypes.cameraExposures == curScene.CameraExposure, 1))
                sceneTypes.cameraExposures(end+1) = curScene.CameraExposure;
            end
            
            if isempty(find(sceneTypes.distances == curScene.Distance, 1))
                sceneTypes.distances(end+1) = curScene.Distance;
            end
            
            if isempty(find(sceneTypes.angles == curScene.angle, 1))
                sceneTypes.angles(end+1) = curScene.angle;
            end
            
            if isempty(find(sceneTypes.lights == curScene.light, 1))
                sceneTypes.lights(end+1) = curScene.light;
            end
        end
    end
    
    sceneTypes.cameraExposures = sort(sceneTypes.cameraExposures);
    sceneTypes.distances = sort(sceneTypes.distances);
    sceneTypes.angles = sort(sceneTypes.angles);
    sceneTypes.lights = sort(sceneTypes.lights);
end % getSceneTypes()
