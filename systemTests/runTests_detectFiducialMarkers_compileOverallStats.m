function resultsData_overall = runTests_detectFiducialMarkers_compileOverallStats(resultsData_perPose, showPlots)
    %#ok<*FNDSB>
    
    sceneTypes = getSceneTypes(resultsData_perPose);
    
    numCameraExposures = length(sceneTypes.CameraExposures);
    numDistances = length(sceneTypes.Distances);
    numAngles = length(sceneTypes.Angles);
    numLights = length(sceneTypes.Lights);
    
    quadExtraction_events = extractEvents(resultsData_perPose, sceneTypes);
    
    if showPlots
        for iAngle = 1:numAngles
            for iLight = 1:numLights
                
                dataPoints = zeros(numCameraExposures, numDistances);
                for iCameraExposure = 1:numCameraExposures
                    for iDistance = 1:numDistances
                        quadExtraction_numQuadsDetected = squeeze(quadExtraction_events(iCameraExposure, iDistance, iAngle, iLight, 1));
                        quadExtraction_numQuadsNotIgnored = squeeze(quadExtraction_events(iCameraExposure, iDistance, iAngle, iLight, 2));
                        
                        dataPoints(iCameraExposure, iDistance) = quadExtraction_numQuadsDetected / quadExtraction_numQuadsNotIgnored;
                    end
                end
                
                figure(iLight);
                subplot(1,numAngles,iAngle);
                bar(sceneTypes.Distances, dataPoints')
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
    
    resultsData_overall.percentQuadsExtracted = sum(sum(sum(sum(quadExtraction_events(:,:,:,:,1))))) / sum(sum(sum(sum(quadExtraction_events(:,:,:,:,2)))));
    
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
    quadExtraction_events = zeros(length(sceneTypes.CameraExposures), length(sceneTypes.Distances), length(sceneTypes.Angles), length(sceneTypes.Lights), 2);
    
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
                                            quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights, 1) = quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights, 1) + curResult.numQuadsDetected;
                                            quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights, 2) = quadExtraction_events(iCameraExposures, iDistances, iAngles, iLights, 2) + curResult.numQuadsNotIgnored;
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
