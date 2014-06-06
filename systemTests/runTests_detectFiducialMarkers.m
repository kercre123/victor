% function resultsData = runTests_detectFiducialMarkers(testJsonPattern, resultsFilename)

% resultsData = runTests_detectFiducialMarkers('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_*.json', 'C:/Anki/systemTestImages/results_runTests_detectFiducialMarkers.mat');

function resultsData = runTests_detectFiducialMarkers(testJsonPattern, resultsFilename)

global maxMatchDistance_pixels;
global maxMatchDistance_percent;

% useUndistortion = false;

% If all corners of a quad are not detected within these threshold, it is considered a complete failure
maxMatchDistance_pixels = 5;
maxMatchDistance_percent = 0.2;

showImageDetections = false;

recompileBasics = true;
basicsFilename = 'basicsResults.mat';
markerDirectoryList = {'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/symbols/withFiducials/', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/letters/withFiducials', 'Z:/Documents/Box Documents/Cozmo SE/VisionMarkers/dice/withFiducials'};

assert(exist('testJsonPattern', 'var') == 1);
assert(exist('resultsFilename', 'var') == 1);

if recompileBasics
    [resultsData, testPath, allTestFilenames, testFunctions, testFunctionNames] = computeBasics(markerDirectoryList, testJsonPattern);
    save(basicsFilename, '*');
else
    load(basicsFilename);
end

allCompiledResults = compileStats(resultsData, testPath, allTestFilenames, testFunctions, testFunctionNames, showImageDetections);

save(resultsFilename, 'allCompiledResults');

keyboard

function allCompiledResults = compileStats(resultsData, testPath, allTestFilenames, testFunctions, testFunctionNames, showImageDetections)
    % TODO: At different distances / angles

    global rotationList;
    
    allCompiledResults = cell(length(resultsData), 1);

    for iTest = 1:length(resultsData)
        allCompiledResults{iTest} = cell(length(resultsData{iTest}), 1);

        if showImageDetections
            jsonData = loadjson(allTestFilenames{iTest});
        end

        for iPose = 1:length(allCompiledResults{iTest})
            allCompiledResults{iTest}{iPose} = cell(length(resultsData{iTest}{iPose}), 1);

            for iTestFunction = 1:length(allCompiledResults{iTest}{iPose})
                curResultsData = resultsData{iTest}{iPose}{iTestFunction};

                [curCompiled.numQuadsTotal, curCompiled.numQuadsDetected] = compileQuadResults(curResultsData);

                [curCompiled.numCorrect_positionLabelRotation, curCompiled.numCorrect_positionLabel, curCompiled.numCorrect_position, curCompiled.numSpurriousDetections, curCompiled.numUndetected] = compileMarkerResults(curResultsData);

                allCompiledResults{iTest}{iPose}{iTestFunction} = curCompiled;

                if showImageDetections
                    disp(curCompiled)

                    image = imread([testPath, jsonData.Poses{iPose}.ImageFile]);

                    [detectedQuads, detectedMarkers] = testFunctions{iTestFunction}(image);

                    figure(1);
                    hold off;
                    imshow(image);
                    hold on;

                    for iQuad = 1:length(detectedQuads)
                        plot(detectedQuads{iQuad}([1,2,4,3,1],1), detectedQuads{iQuad}([1,2,4,3,1],2), 'y', 'LineWidth', 3);
                    end

                    for iMarker = 1:length(detectedMarkers)
                        sortedXValues = sortrows([detectedMarkers{iMarker}.corners(:,1)'; 1:4]', 1);

                        firstCorner = sortedXValues(1,2);
                        secondCorner = sortedXValues(2,2);

                        plot(detectedMarkers{iMarker}.corners([1,2,4,3,1],1), detectedMarkers{iMarker}.corners([1,2,4,3,1],2), 'g', 'LineWidth', 3);
                        plot(detectedMarkers{iMarker}.corners([1,3],1), detectedMarkers{iMarker}.corners([1,3],2), 'b', 'LineWidth', 3);
                        midX = (detectedMarkers{iMarker}.corners(firstCorner,1) + detectedMarkers{iMarker}.corners(secondCorner,1)) / 2;
                        midY = (detectedMarkers{iMarker}.corners(firstCorner,2) + detectedMarkers{iMarker}.corners(secondCorner,2)) / 2;
                        text(midX + 5, midY, curResultsData.allDetectedMarkerNames{iMarker}(8:end), 'Color', 'g');
                    end

                    title(sprintf('TestFunction:%s\nCameraExposure:%0.1f\nDistance:%d\nangle:%d\nlight:%d',...
                        testFunctionNames{iTestFunction},...
                        jsonData.Poses{iPose}.Scene.CameraExposure,...
                        jsonData.Poses{iPose}.Scene.Distance,...
                        jsonData.Poses{iPose}.Scene.angle,...
                        jsonData.Poses{iPose}.Scene.light));

                    pause()
                end
            end
        end
    end

function [numTotal, numGood] = compileQuadResults(curResultsData)
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;

    numTotal = length(curResultsData.markerNames);
    numGood = 0;

    for iMarker = 1:numTotal
        curDistance = curResultsData.justQuads_bestDistances_max(iMarker,:);
        curFiducialSize = curResultsData.fiducialSizes(iMarker,:);

        % Are all distances small enough?
        if curDistance(1) <= maxMatchDistance_pixels &&...
           curDistance(2) <= maxMatchDistance_pixels &&...
           curDistance(3) <= maxMatchDistance_pixels &&...
           curDistance(2) <= (curFiducialSize(1) * maxMatchDistance_percent) &&...
           curDistance(3) <= (curFiducialSize(2) * maxMatchDistance_percent)

           numGood = numGood + 1;
        end
    end

function [numCorrect_positionLabelRotation, numCorrect_positionLabel, numCorrect_position, numSpurriousDetections, numUndetected] = compileMarkerResults(curResultsData)
    global maxMatchDistance_pixels;
    global maxMatchDistance_percent;
    
    numCorrect_positionLabelRotation = 0;
    numCorrect_positionLabel = 0;
    numCorrect_position = 0;
    numSpurriousDetections = 0;
    numUndetected = 0;

    % Check all ground truth markers
    for iMarker = 1:length(curResultsData.markerNames)
        curDistance = curResultsData.justQuads_bestDistances_max(iMarker,:);
        curFiducialSize = curResultsData.fiducialSizes(iMarker,:);

        % Are all distances small enough?
        if curDistance(1) <= maxMatchDistance_pixels &&...
           curDistance(2) <= maxMatchDistance_pixels &&...
           curDistance(3) <= maxMatchDistance_pixels &&...
           curDistance(2) <= (curFiducialSize(1) * maxMatchDistance_percent) &&...
           curDistance(3) <= (curFiducialSize(2) * maxMatchDistance_percent)
       
            matchIndex = curResultsData.markers_bestIndexes(iMarker);
            if strcmp(curResultsData.markerNames{iMarker}, curResultsData.allDetectedMarkerNames{matchIndex})
                if curResultsData.markers_areRotationsCorrect(iMarker)
                    numCorrect_positionLabelRotation = numCorrect_positionLabelRotation + 1;
                    numCorrect_positionLabel = numCorrect_positionLabel + 1;
                    numCorrect_position = numCorrect_position + 1;
                else
                    numCorrect_positionLabel = numCorrect_positionLabel + 1;
                    numCorrect_position = numCorrect_position + 1;
                end
            else
                numCorrect_position = numCorrect_position + 1;
            end       
        else
            numUndetected = numUndetected + 1;
        end
    end
    
    % Look for spurrious detections
    allDetectedIndexes = unique(curResultsData.markers_bestIndexes);
    for iMarker = 1:length(curResultsData.allDetectedMarkerNames)
        if isempty(find(iMarker == allDetectedIndexes, 1))
            numSpurriousDetections = numSpurriousDetections + 1;
        end
    end    
    
function [resultsData, testPath, allTestFilenames, testFunctions, testFunctionNames] = computeBasics(markerDirectoryList, testJsonPattern)
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
        @extractMarkers_matlabOriginal_noRefinement,...
        @extractMarkers_matlabOriginal_withRefinement,...
        @extractMarkers_c_noRefinement,...
        @extractMarkers_c_withRefinement};

    testFunctionNames = {...
        'matlab-no',...
        'matlab-with',...
        'C-no  ',...
        'C-with'};

    resultsData = cell(length(allTestFilenames), 1);
    testData = cell(length(allTestFilenames), 1);

    for iTest = 1:length(allTestFilenames)
%     for iTest = 1
        tic;

        jsonData = loadjson(allTestFilenames{iTest});

        if ~iscell(jsonData.Poses)
            jsonData.Poses = { jsonData.Poses };
        end

        resultsData{iTest} = cell(length(jsonData.Poses), 1);
        testData{iTest} = cell(length(jsonData.Poses), 1);

        for iPose = 1:length(jsonData.Poses)
            image = imread([testPath, jsonData.Poses{iPose}.ImageFile]);

            resultsData{iTest}{iPose} = cell(length(testFunctions), 1);

            if ~isfield(jsonData.Poses{iPose}, 'VisionMarkers')
                continue;
            end

            testData{iTest}{iPose}.Scene = jsonData.Poses{iPose}.Scene;
            testData{iTest}{iPose}.ImageFile = jsonData.Poses{iPose}.ImageFile;

            groundTruthQuads = jsonToQuad(jsonData.Poses{iPose}.VisionMarkers);

            for iTestFunction = 1:length(testFunctions)
                [detectedQuads, detectedMarkers] = testFunctions{iTestFunction}(image);

                %check if the quads are in the right places

                [justQuads_bestDistances_mean, justQuads_bestDistances_max, justQuads_bestIndexes, ~] = findClosestMatches(groundTruthQuads, detectedQuads, []);

                [markers_bestDistances_mean, markers_bestDistances_max, markers_bestIndexes, markers_areRotationsCorrect] = findClosestMatches(groundTruthQuads, markersToQuad(detectedMarkers), jsonData.Poses{iPose}.VisionMarkers);

                markerNames = cell(length(groundTruthQuads), 1);
                fiducialSizes = zeros(length(groundTruthQuads), 2);

                for iMarker = 1:length(groundTruthQuads)
                    markerNames{iMarker,1} = jsonData.Poses{iPose}.VisionMarkers{iMarker}.markerType;

                    fiducialSizes(iMarker,:) = [...
                        max(groundTruthQuads{iMarker}(:,1)) - min(groundTruthQuads{iMarker}(:,1)),...
                        max(groundTruthQuads{iMarker}(:,2)) - min(groundTruthQuads{iMarker}(:,2))];

    %                 markerNames{iMarker,1} = detectedMarkers{markers_bestIndexes(iMarker)}.name;
    %                 markerNames{iMarker,2} = jsonData.Poses{iPose}.VisionMarkers{iMarker}.markerType;%
                end

                allDetectedMarkerNames = cell(length(detectedMarkers), 1);
                for iMarker = 1:length(detectedMarkers)
                    allDetectedMarkerNames{iMarker} = detectedMarkers{iMarker}.name;
                end

                resultsData{iTest}{iPose}{iTestFunction}.justQuads_bestDistances_mean = justQuads_bestDistances_mean;
                resultsData{iTest}{iPose}{iTestFunction}.justQuads_bestDistances_max = justQuads_bestDistances_max;
                resultsData{iTest}{iPose}{iTestFunction}.justQuads_bestIndexes = justQuads_bestIndexes;
                resultsData{iTest}{iPose}{iTestFunction}.markers_bestDistances_mean = markers_bestDistances_mean;
                resultsData{iTest}{iPose}{iTestFunction}.markers_bestDistances_max = markers_bestDistances_max;
                resultsData{iTest}{iPose}{iTestFunction}.markers_bestIndexes = markers_bestIndexes;
                resultsData{iTest}{iPose}{iTestFunction}.markers_areRotationsCorrect = markers_areRotationsCorrect;
                resultsData{iTest}{iPose}{iTestFunction}.fiducialSizes = fiducialSizes;
                resultsData{iTest}{iPose}{iTestFunction}.markerNames = markerNames;
                resultsData{iTest}{iPose}{iTestFunction}.allDetectedMarkerNames = allDetectedMarkerNames;

    %             figure(1);
    %             hold off;
    %             imshow(image);
    %             hold on;
    %             for i = 1:length(detectedQuads)
    %                 plot(detectedQuads{i}(:,1), detectedQuads{i}(:,2));
    %                 text(detectedQuads{i}(1,1), detectedQuads{i}(1,2), sprintf('%d', bestIndexes(i)));
    %             end
            end
        end

        disp(sprintf('Finished test %d/%d in %f seconds', iTest, length(allTestFilenames), toc()));
    end % for iTest = 1:length(allTestFilenames)

function [bestDistances_mean, bestDistances_max, bestIndexes, areRotationsCorrect] = findClosestMatches(groundTruthQuads, queryQuads, markerNames)
    global rotationList;
    
    bestDistances_mean = Inf * ones(length(groundTruthQuads), 1);
    bestDistances_max = Inf * ones(length(groundTruthQuads), 3);
    bestIndexes = -1 * ones(length(groundTruthQuads), 1);
    areRotationsCorrect = false * ones(length(groundTruthQuads), 1);
    
    for iGroundTruth = 1:length(groundTruthQuads)
        gtQuad = groundTruthQuads{iGroundTruth};
        
        if isempty(markerNames)
            numRotations = 4;
        else
            curName = markerNames{iGroundTruth}.markerType(8:end);
            
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

%                 figure(iRotation+1); plot(gtQuad(:,1), gtQuad(:,2));

                % rotate the quad
                queryQuad = queryQuad([3,1,4,2],:);
            end
        end
    end

function quads = jsonToQuad(jsonQuads)
    if ~iscell(jsonQuads)
        jsonQuads = { jsonQuads };
    end

    quads = cell(length(jsonQuads), 1);

    for iQuad = 1:length(jsonQuads)
        quads{iQuad} = [...
            jsonQuads{iQuad}.x_imgUpperLeft, jsonQuads{iQuad}.y_imgUpperLeft;
            jsonQuads{iQuad}.x_imgLowerLeft, jsonQuads{iQuad}.y_imgLowerLeft;
            jsonQuads{iQuad}.x_imgUpperRight, jsonQuads{iQuad}.y_imgUpperRight;
            jsonQuads{iQuad}.x_imgLowerRight, jsonQuads{iQuad}.y_imgLowerRight];
    end

function quads = markersToQuad(markers)
    if ~iscell(markers)
        markers = { markers };
    end

    quads = cell(length(markers), 1);

    for iQuad = 1:length(markers)
        quads{iQuad} = markers{iQuad}.corners;
    end
