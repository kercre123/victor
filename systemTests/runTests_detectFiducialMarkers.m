% function [resultsData, resultsString] = runTests_detectFiducialMarkers(testJsonPattern, outputFilename)

% [resultsData, resultsString] = runTests_detectFiducialMarkers('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_*.json', 'C:/Anki/systemTestImages/results_runTests_detectFiducialMarkers.mat');

function [resultsData, resultsString] = runTests_detectFiducialMarkers(testJsonPattern, outputFilename)

% useUndistortion = false;
maxMatchDistance = 10; % If all corners of a quad are not detected within this threshold, it is considered a complete failure

showExtractedQuads = true;

assert(exist('testJsonPattern', 'var') == 1);
assert(exist('outputFilename', 'var') == 1);

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
resultsString = '';



    
    
for iTest = 1:length(allTestFilenames)
    jsonData = loadjson(allTestFilenames{iTest});

    if ~iscell(jsonData.Poses)
        jsonData.Poses = { jsonData.Poses };
    end
    
    resultsData{iTest} = cell(length(jsonData.Poses), 1);
    
    for iPose = 1:length(jsonData.Poses)
        image = imread([testPath, jsonData.Poses{1}.ImageFile]);
        
        resultsData{iTest}{iPose} = cell(length(testFunctions), 1);
        
        groundTruthQuads = jsonToQuad(jsonData.Poses{iPose}.VisionMarkers);
        
        for iTestFunction = 1:length(testFunctions)
            [detectedQuads, detectedMarkers] = testFunctions{iTestFunction}(image);
            
            [bestDistances_mean, bestDistances_max, bestIndexes, areRotationsCorrect] = findClosestMatches(groundTruthQuads, detectedQuads)
            
            figure(1); 
            hold off;
            imshow(image);
            hold on;
            for i = 1:length(detectedQuads)
                plot(detectedQuads{i}(:,1), detectedQuads{i}(:,2));
                text(detectedQuads{i}(1,1), detectedQuads{i}(1,2), sprintf('%d', bestIndexes(i)));
            end
            
            keyboard
        end
    end
    
    keyboard

%     % Init code here
% 
%     resultsData{iTest} = cell(length(testingResolutions), 1);
% 
%     for iTestingResolution = 1:length(testingResolutions)
%         testingResolution = testingResolutions{iTestingResolution};
%         scale = testingResolution(1) / originalResolution(1);
% 
%         resultsData{iTest}{iTestingResolution} = cell(size(allImages,1), 1);
% 
%         for iFrame = 1:size(allImages,1)
%             imageResized = imresize(allImages{iFrame,1}, testingResolution);
% 
%             resultsData{iTest}{iTestingResolution}{iFrame} = cell(length(testFunctions), 1);
% 
%             % For every type of quad extraction function
%             for iTestFunction = 1:length(testFunctions)
%                 tic
% 
%                 curTestFunction = testFunctions{iTestFunction};
%                 extractedQuads = curTestFunction(imageResized);
% 
%                 groundTruthCorners = jsonData.sequences{allImages{iFrame,2}}.groundTruth{allImages{iFrame,3}}.fiducialMarkerCorners;
%                 if ~iscell(groundTruthCorners)
%                     groundTruthCorners = {groundTruthCorners};
%                 end
% 
%                 groundTruthCorners = groundTruthCorners(1:(end-1));
% 
% %                 resultsData{iTest}{iTestingResolution}{iFrame}{iTestFunction} = Inf * ones(length(groundTruthCorners), 1);
%                 resultsData{iTest}{iTestingResolution}{iFrame}{iTestFunction} = cell(length(groundTruthCorners), 1);
% 
%                 % For all labeled quads, find if any of the extracted quads
%                 % match within maxMatchDistance pixels
%                 isExtractedQuadsMatched = zeros(length(extractedQuads),1);
%                 for iGroundTruth = 1:length(groundTruthCorners)
%                     curGroundTruthCorners = groundTruthCorners{iGroundTruth};
% 
%                     assert(length(curGroundTruthCorners) == 4);
% 
%                     % Find the closest matching quad to the current ground
%                     % truth quad
%                     bestDistances = Inf * ones(4,1);
%                     for iQuad = 1:length(extractedQuads)
%                         curExtractedQuad = extractedQuads{iQuad};
% 
%                         assert(size(curExtractedQuad,1) == 4);
% 
%                         numMatched = 0;
%                         dists = Inf * ones(4,1);
%                         for iGrCorner = 1:4
%                             for iExCorner = 1:4
%                                 dist = sqrt( (curExtractedQuad(iExCorner,1) - curGroundTruthCorners{iGrCorner}.x*scale)^2 + (curExtractedQuad(iExCorner,2) - curGroundTruthCorners{iGrCorner}.y*scale)^2) / scale;
%                                 if dist <= maxMatchDistance
%                                     dists(iGrCorner) = dist;
%                                     numMatched = numMatched + 1;
%                                     break;
%                                 end
%                             end
%                         end
% 
%                         if numMatched == 4
%                             isExtractedQuadsMatched(iQuad) = 1;
%                             if mean(dists) < mean(bestDistances)
%                                 bestDistances = dists;
%                             end
%                         end
%                     end % for iQuad = 1:length(extractedQuads)
% 
%                     % bestDistances is infinity if no good match was found
%                     resultsData{iTest}{iTestingResolution}{iFrame}{iTestFunction}{iGroundTruth} = bestDistances;
%                 end % for iGroundTruth = 1:length(groundTruthCorners)
% 
%                 someDistances = zeros(4,length(groundTruthCorners));
%                 for iGroundTruth = 1:length(groundTruthCorners)
%                     someDistances(:,iGroundTruth) = resultsData{iTest}{iTestingResolution}{iFrame}{iTestFunction}{iGroundTruth};
%                 end
% 
%                 numMatched = sum(~isinf(someDistances(1,:)));
% 
%                 inds = ~isinf(someDistances(1,:));
%                 someDistances = someDistances(:,inds);
% 
%                 curResultsString = sprintf('(%s) Test:%d/%d Resolution:%dx%d Frame:%d/%d TestFunction:%d/%d time:%fs numMatched=%d/%d mean=%f median=%f std=%f max=%f',...
%                     testFunctionNames{iTestFunction},...
%                     iTest, length(allTests),...
%                     testingResolutions{iTestingResolution}(2), testingResolutions{iTestingResolution}(1),...
%                     iFrame, size(allImages,1),...
%                     iTestFunction, length(testFunctions),...
%                     toc(),...
%                     numMatched, length(groundTruthCorners),...
%                     mean(someDistances(:)), median(someDistances(:)), std(someDistances(:)), max(someDistances(:)) );
% 
%                 resultsString = [resultsString, sprintf('\n'), curResultsString];
% 
%                 curDateAndTime = round(clock());
%                 save(outputFilename, 'resultsData', 'resultsString', 'curDateAndTime');
% 
%                 disp(curResultsString);
% 
%                 if showExtractedQuads
%                     figure(iTest*100 + iTestingResolution*10 + iFrame);
%                     subplot(1,length(testFunctions),iTestFunction);
%                     plotQuads(img, extractedQuads, isExtractedQuadsMatched, groundTruthCorners, scale, testFunctionNames{iTestFunction})
%                 end
%             end % for iTestFunction = 1:length(testFunctions)
%             pause(.01);
%         end % for iFrame = 1:size(allImages,1)
%     end % for iTestingResolution = 1:length(testingResolutions)
end % for iTest = 1:length(allTests)

% keyboard

function [bestDistances_mean, bestDistances_max, bestIndexes, areRotationsCorrect] = findClosestMatches(groundTruthQuads, queryQuads)
    bestDistances_mean = Inf * ones(length(queryQuads), 1);
    bestDistances_max = Inf * ones(length(queryQuads), 1);
    bestIndexes = -1 * ones(length(queryQuads), 1);
    areRotationsCorrect = false * ones(length(queryQuads), 1);
        
    for iQuery = 1:length(queryQuads)
        queryQuad = queryQuads{iQuery};
        
        for iGroundTruth = 1:length(groundTruthQuads)
            gtQuad = groundTruthQuads{iGroundTruth};

            for iRotation = 0:3
                distances = sqrt(sum((queryQuad - gtQuad).^2, 2));
                
                % Compute the closest match based on the mean distance
                distance_mean = mean(distances);
                
                if distance_mean < bestDistances_mean(iQuery)
                    bestDistances_mean(iQuery) = distance_mean;
                    bestDistances_max(iQuery) = max(distances);
                    bestIndexes(iQuery) = iGroundTruth;
                    
                    if iRotation == 0
                        areRotationsCorrect(iQuery) = true;
                    else
                        areRotationsCorrect(iQuery) = false;
                    end                    
                end
                
%                 figure(iRotation+1); plot(gtQuad(:,1), gtQuad(:,2));
                
                % rotate the quad
                gtQuad = gtQuad([3,1,4,2],:);
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

function plotQuads(image, extractedQuads, isExtractedQuadsMatched, groundTruthCorners, scale, titleString)
    hold off;
    imshow(image);
    hold on;
    
    title(titleString);
    
    for i=1:length(extractedQuads)
        if isExtractedQuadsMatched(i)
            plotString = 'b';
        else
            plotString = 'r';
        end
        plot((1/scale)*extractedQuads{i}([1:4,1],1), (1/scale)*extractedQuads{i}([1:4,1],2), plotString);
    end
    
    for i=1:length(groundTruthCorners)
        curQuadX = [groundTruthCorners{i}{1}.x, groundTruthCorners{i}{2}.x, groundTruthCorners{i}{3}.x, groundTruthCorners{i}{4}.x, groundTruthCorners{i}{1}.x];
        curQuadY = [groundTruthCorners{i}{1}.y, groundTruthCorners{i}{2}.y, groundTruthCorners{i}{3}.y, groundTruthCorners{i}{4}.y, groundTruthCorners{i}{1}.y];
        plot(curQuadX, curQuadY, 'g--');
    end

function [allQuads, markers] = extractMarkers_matlabOriginal_noRefinement(image)
    allQuads = simpleDetector(image, 'decodeMarkers', false, 'quadRefinementIterations', 0);
    markers = simpleDetector(image, 'decodeMarkers', true, 'quadRefinementIterations', 0);        
    
function [allQuads, markers] = extractMarkers_matlabOriginal_withRefinement(image)
    allQuads = simpleDetector(image, 'decodeMarkers', false, 'quadRefinementIterations', 25);
    markers = simpleDetector(image, 'decodeMarkers', true, 'quadRefinementIterations', 25);
    
function [allQuads, markers] = extractMarkers_c_noRefinement(image)
    [allQuads, markers] = extractMarkers_c_total(image, false);
    
function [allQuads, markers] = extractMarkers_c_withRefinement(image)
    [allQuads, markers] = extractMarkers_c_total(image, true);    
    
function [allQuads, markers] = extractMarkers_c_total(image, useRefinement)    
    imageSize = size(image);
    scaleImage_thresholdMultiplier = 1.0;
    scaleImage_numPyramidLevels = 3;
    component1d_minComponentWidth = 0;
    component1d_maxSkipDistance = 0;
    minSideLength = round(0.03*max(imageSize(1),imageSize(2)));
    maxSideLength = round(0.97*min(imageSize(1),imageSize(2)));
    component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
    component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
    component_sparseMultiplyThreshold = 1000.0;
    component_solidMultiplyThreshold = 2.0;
    component_minHollowRatio = 1.0;
    quads_minQuadArea = 100 / 4;
    quads_quadSymmetryThreshold = 2.0;
    quads_minDistanceFromImageEdge = 2;
    decode_minContrastRatio = 1.25;
    numRefinementSamples = 100;
    
    if useRefinement        
        quadRefinementIterations = 25;
    else
        quadRefinementIterations = 0;
    end
    
    returnInvalidMarkers = 0;
    [allQuads, ~, ~] = mexDetectFiducialMarkers(image, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, returnInvalidMarkers);
    
    returnInvalidMarkers = 1;
    [goodQuads, ~, markerNames] = mexDetectFiducialMarkers(image, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, returnInvalidMarkers);

    markers = cell(length(markerNames), 1);
    
    for i = 1:length(markerNames)
        markers{i}.name = markerNames{i};
        markers{i}.corners = goodQuads{i};
    end
    
    % Convert from c to matlab coordinates
    for i = 1:length(allQuads)
        allQuads{i} = allQuads{i} + 1;
    end
    
    for i = 1:length(markers)
        markers{i}.corners = markers{i}.corners + 1;
    end
    
    