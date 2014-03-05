% function [resultsData, resultsString] = runTests_quadExtraction(allTestsFilename, outputFilename)

% [resultsData, resultsString] = runTests_quadExtraction('C:/Anki/products-cozmo/systemTests/tests/allQuadExtractionTests.json', 'C:/Anki/systemTestImages/results_quadExtraction.mat');

function [resultsData, resultsString] = runTests_quadExtraction(allTestsFilename, outputFilename)

originalResolution = [480,640];
testingResolutions = {[480,640], [240,320], [120,160]};
useUndistortion = false;
maxMatchDistance = 8; % If all corners of a quad are not detected within this threshold, it is considered a complete failure

showExtractedQuads = true;

assert(exist('allTestsFilename', 'var') == 1);
assert(exist('outputFilename', 'var') == 1);

if useUndistortion
    load('Z:\Documents\Box Documents\Cozmo SE\calibCozmoProto1_head.mat');
    cam = Camera('calibration', calibCozmoProto1_head);
end

allTests = loadjson(allTestsFilename);

slashIndexes = strfind(allTestsFilename, '/');
dataPath = allTestsFilename(1:(slashIndexes(end)));

testFunctions = {@extractQuads_matlabOriginal, @extractQuads_c};

resultsData = cell(length(allTests), 1);
resultsString = '';

for iTest = 1:length(allTests)
    jsonData = loadjson([dataPath, allTests.tests{iTest}]);

    if ~iscell(jsonData.sequences)
        jsonData.sequences = { jsonData.sequences };
    end

    allImages = cell(0,3);

    % At the start, load all the images into memory, and undistort as required
    for iSequence = 1:length(jsonData.sequences)
        if ~iscell(jsonData.sequences{iSequence}.groundTruth)
            jsonData.sequences{iSequence}.groundTruth = { jsonData.sequences{iSequence}.groundTruth };
        end

        filenamePattern = jsonData.sequences{iSequence}.filenamePattern;
        for iFrame = 1:length(jsonData.sequences{iSequence}.frameNumbers)
            frameNumber = jsonData.sequences{iSequence}.frameNumbers(iFrame);
            img = imresize(imread([dataPath, sprintf(filenamePattern, frameNumber)]), originalResolution);

            if useUndistortion
                imgUndistorted = cam.undistort(img);
                allImages{end+1,1} = imgUndistorted;
            else
                allImages{end+1,1} = img;
            end

            allImages{end,2} = iSequence;

            frameIndex = findFrameNumberIndex(jsonData, iSequence, iFrame);
            allImages{end,3} = frameIndex;
        end
    end % for iSequence = 1:length(jsonData.sequences)

    % Init code here

    resultsData{iTest} = cell(length(testingResolutions), 1);

    for iTestingResolution = 1:length(testingResolutions)
        testingResolution = testingResolutions{iTestingResolution};
        scale = testingResolution(1) / originalResolution(1);

        resultsData{iTest}{iTestingResolution} = cell(size(allImages,1), 1);

        for iFrame = 1:size(allImages,1)
            imageResized = imresize(allImages{iFrame,1}, testingResolution);

            resultsData{iTest}{iTestingResolution}{iFrame} = cell(length(testFunctions), 1);

            % For every type of quad extraction function
            for iTestFunction = 1:length(testFunctions)
                tic

                curTestFunction = testFunctions{iTestFunction};
                extractedQuads = curTestFunction(imageResized);

                groundTruthCorners = jsonData.sequences{allImages{iFrame,2}}.groundTruth{allImages{iFrame,3}}.fiducialMarkerCorners;
                if ~iscell(groundTruthCorners)
                    groundTruthCorners = {groundTruthCorners};
                end

                groundTruthCorners = groundTruthCorners(1:(end-1));

%                 resultsData{iTest}{iTestingResolution}{iFrame}{iTestFunction} = Inf * ones(length(groundTruthCorners), 1);
                resultsData{iTest}{iTestingResolution}{iFrame}{iTestFunction} = cell(length(groundTruthCorners), 1);

                % For all labeled quads, find if any of the extracted quads
                % match within maxMatchDistance pixels
                for iGroundTruth = 1:length(groundTruthCorners)
                    curGroundTruthCorners = groundTruthCorners{iGroundTruth};

                    assert(length(curGroundTruthCorners) == 4);

                    % Find the closest matching quad to the current ground
                    % truth quad
                    bestDistances = Inf * ones(4,1);
                    for iQuad = 1:length(extractedQuads)
                        curExtractedQuad = extractedQuads{iQuad};

                        assert(size(curExtractedQuad,1) == 4);

                        numMatched = 0;
                        dists = Inf * ones(4,1);
                        for iGrCorner = 1:4
                            for iExCorner = 1:4
                                dist = sqrt( (curExtractedQuad(iExCorner,1) - curGroundTruthCorners{iGrCorner}.x*scale)^2 + (curExtractedQuad(iExCorner,2) - curGroundTruthCorners{iGrCorner}.y*scale)^2) / scale;
                                if dist <= maxMatchDistance
                                    dists(iGrCorner) = dist;
                                    numMatched = numMatched + 1;
                                    break;
                                end
                            end
                        end

                        if numMatched == 4
                            if mean(dists) < mean(bestDistances)
                                bestDistances = dists;
                            end
                        end
                    end % for iQuad = 1:length(extractedQuads)

                    % bestDistances is infinity if no good match was found
                    resultsData{iTest}{iTestingResolution}{iFrame}{iTestFunction}{iGroundTruth} = bestDistances;
                end % for iGroundTruth = 1:length(groundTruthCorners)

                someDistances = zeros(4,length(groundTruthCorners));
                for iGroundTruth = 1:length(groundTruthCorners)
                    someDistances(:,iGroundTruth) = resultsData{iTest}{iTestingResolution}{iFrame}{iTestFunction}{iGroundTruth};
                end

                numMatched = sum(~isinf(someDistances(1,:)));

                inds = ~isinf(someDistances(1,:));
                someDistances = someDistances(:,inds);

                curResultsString = sprintf('Test:%d/%d Resolution:%dx%d Frame:%d/%d TestFunction:%d/%d time:%fs numMatched=%d/%d mean=%f median=%f std=%f max=%f',...
                    iTest, length(allTests),...
                    testingResolutions{iTestingResolution}(2), testingResolutions{iTestingResolution}(1),...
                    iFrame, size(allImages,1),...
                    iTestFunction, length(testFunctions),...
                    toc(),...
                    numMatched, length(groundTruthCorners),...
                    mean(someDistances(:)), median(someDistances(:)), std(someDistances(:)), max(someDistances(:)) );

                resultsString = [resultsString, sprintf('\n'), curResultsString];

                curDateAndTime = round(clock());
                save(outputFilename, 'resultsData', 'resultsString', 'curDateAndTime');

                disp(curResultsString);

                if showExtractedQuads
                    figure(iTest*100 + iTestingResolution*10 + iFrame);
                    subplot(1,length(testFunctions),iTestFunction);
                    plotQuads(img, extractedQuads, groundTruthCorners, scale)
%                     pause();
                end
            end % for iTestFunction = 1:length(testFunctions)
            pause(.01);
        end % for iFrame = 1:size(allImages,1)
    end % for iTestingResolution = 1:length(testingResolutions)
end % for iTest = 1:length(allTests)

% keyboard

function index = findFrameNumberIndex(jsonData, sequenceNumberIndex, frameNumberIndex)
    index = -1;

    if ~isfield(jsonData.sequences{sequenceNumberIndex}, 'groundTruth')
        return;
    end

    for i = 1:length(jsonData.sequences{sequenceNumberIndex}.groundTruth)
        curFrameNumber = jsonData.sequences{sequenceNumberIndex}.groundTruth{i}.frameNumber;
        queryFrameNumber = jsonData.sequences{sequenceNumberIndex}.frameNumbers(frameNumberIndex);

        if curFrameNumber == queryFrameNumber
            index = i;
            return;
        end
    end

function plotQuads(image, extractedQuads, groundTruthCorners, scale)
    hold off;
    imshow(image);
    hold on;
    
    for i=1:length(extractedQuads)
        plot((1/scale)*extractedQuads{i}([1:4,1],1), (1/scale)*extractedQuads{i}([1:4,1],2), 'b');
    end
    
    for i=1:length(groundTruthCorners)
        curQuadX = [groundTruthCorners{i}{1}.x, groundTruthCorners{i}{2}.x, groundTruthCorners{i}{3}.x, groundTruthCorners{i}{4}.x, groundTruthCorners{i}{1}.x];
        curQuadY = [groundTruthCorners{i}{1}.y, groundTruthCorners{i}{2}.y, groundTruthCorners{i}{3}.y, groundTruthCorners{i}{4}.y, groundTruthCorners{i}{1}.y];
        plot(curQuadX, curQuadY, 'g--');
    end

function detectedQuads = extractQuads_matlabOriginal(image)
    detectedQuads = simpleDetector(image, 'decodeMarkers', false, 'showComponents', true);

function detectedQuads = extractQuads_c(image)
    imageSize = size(image);

%     if imageSize(1) > 240 || imageSize(2) > 320
%         detectedQuads = {};
%         return;
%     end

    if imageSize(2) == 640
        scaleImage_numPyramidLevels = 4;
        quads_minQuadArea = round(100);
    elseif imageSize(2) == 320
        scaleImage_numPyramidLevels = 3;
        quads_minQuadArea = round(100 / 4);
    elseif imageSize(2) == 160
        scaleImage_numPyramidLevels = 2;
        quads_minQuadArea = round(100 / (4^2));
    elseif imageSize(2) == 80
        scaleImage_numPyramidLevels = 1;
        quads_minQuadArea = round(100 / (4^3));
    else
        assert(false);
    end

%     scaleImage_thresholdMultiplier = .75;
    scaleImage_thresholdMultiplier = 1.0;
    component1d_minComponentWidth = 0;
    component1d_maxSkipDistance = 0;
    minSideLength = round(0.03*max(imageSize(1),imageSize(2)));
    maxSideLength = round(0.97*min(imageSize(1),imageSize(2)));
    component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
    component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
    component_sparseMultiplyThreshold = 1000.0;
    component_solidMultiplyThreshold = 2.0;
    component_percentHorizontal = 0.5;
    component_percentVertical = 0.5;
    quads_quadSymmetryThreshold = 2.0;
    quads_minDistanceFromImageEdge = 2;
    decode_minContrastRatio = 1.25;
    returnInvalidMarkers = 1;

    [detectedQuads, ~, ~, ~] = mexDetectFiducialMarkers(image, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_percentHorizontal, component_percentVertical, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, returnInvalidMarkers);

%     keyboard

