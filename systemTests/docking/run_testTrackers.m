% function run_testTrackers(allTestsFilename)

% run_testTrackers('C:/Anki/systemTestImages/allDockerTests.json', 'C:/Anki/systemTestImages/results.mat');

function run_testTrackers(allTestsFilename, outputFilename)

mainResolution = [480,640];
downsampleFactor = 1;

% resolutions = {[480,640], [240,320], [120,160], [60,80], [30,40]};
resolutions = {[240,320], [120,160], [60,80]};
% maxPyramidLevels = [5, 4, 3, 1, 1, 1];
% maxPyramidLevels = [4, 3, 1, 1];
maxPyramidLevels = [3, 2, 1];
maxIterations = 10;
convergenceTolerance = 0.1;
useUndistortion = false;

binaryLK_extremaFilterWidth = 3;
binaryLK_extremaFilterSigma = 1.0;
binaryLK_extremaDerivativeThreshold = 255;
binaryLK_matchingFilterWidth = 7;
binaryLK_matchingFilterSigma = 2.0;
% binaryLK_matchingFilterWidth = 3;
% binaryLK_matchingFilterSigma = 1.0;
% binaryLK_matchingFilterWidth = 15;
% binaryLK_matchingFilterSigma = 5.0;

assert(exist('allTestsFilename', 'var') == 1);
assert(exist('outputFilename', 'var') == 1);

if useUndistortion
    load('Z:\Documents\Box Documents\Cozmo SE\calibCozmoProto1_head.mat');
    cam = Camera('calibration', calibCozmoProto1_head);
end

allTests = loadjson(allTestsFilename);

slashIndexes = strfind(allTestsFilename, '/');
dataPath = allTestsFilename(1:(slashIndexes(end)));

pointErrors_projectiveLK = cell(length(allTests), 1);
pointErrors_binaryProjectiveLK = cell(length(allTests), 1);
pointErrors_splitBinaryProjectiveLK = cell(length(allTests), 1);

fullResultsString = '';

for iTest = 1:length(allTests)
    jsonData = loadjson([dataPath, allTests.tests{iTest}]);

    if ~iscell(jsonData.sequences)
        jsonData.sequences = { jsonData.sequences };
    end

    allImages = cell(0,3);

    for iSequence = 1:length(jsonData.sequences)
        if ~iscell(jsonData.sequences{iSequence}.groundTruth)
            jsonData.sequences{iSequence}.groundTruth = { jsonData.sequences{iSequence}.groundTruth };
        end

        filenamePattern = jsonData.sequences{iSequence}.filenamePattern;
        for iFrame = 1:length(jsonData.sequences{iSequence}.frameNumbers)
            frameNumber = jsonData.sequences{iSequence}.frameNumbers(iFrame);
            img = imresize(imread([dataPath, sprintf(filenamePattern, frameNumber)]), mainResolution/downsampleFactor);
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
    end

    pointErrors_projectiveLK{iTest} = cell(length(resolutions), 1);
    pointErrors_binaryProjectiveLK{iTest} = cell(length(resolutions), 1);
    pointErrors_splitBinaryProjectiveLK{iTest} = cell(length(resolutions), 1);    

    frameIndex = findFrameNumberIndex(jsonData, 1, 1);

    templateCorners = jsonData.sequences{1}.groundTruth{frameIndex}.templateCorners;
    xCoordinates = [templateCorners{1}.x, templateCorners{2}.x, templateCorners{3}.x, templateCorners{4}.x] / downsampleFactor;
    yCoordinates = [templateCorners{1}.y, templateCorners{2}.y, templateCorners{3}.y, templateCorners{4}.y] / downsampleFactor;
    templateQuad = [xCoordinates', yCoordinates'];

    errorSignalCorners = jsonData.sequences{1}.groundTruth{frameIndex}.errorSignalCorners;
    esX = [errorSignalCorners{1}.x, errorSignalCorners{2}.x] / downsampleFactor;
    esY = [errorSignalCorners{1}.y, errorSignalCorners{2}.y] / downsampleFactor;
    bottomLine = [esX', esY'];

    mask = zeros(size(allImages{1,1}));
    mask = roipoly(mask, xCoordinates, yCoordinates);

    for iOriginalResolution = 1:length(resolutions)
        originalResolution = resolutions{iOriginalResolution};

        originalImageResized = imresize(allImages{1,1}, originalResolution);
        maskResized = imresize(mask, originalResolution);

        scale = originalResolution(1) / mainResolution(1) * downsampleFactor;

        pointErrors_projectiveLK{iTest}{iOriginalResolution} = cell(maxPyramidLevels(iOriginalResolution), 1);
        pointErrors_binaryProjectiveLK{iTest}{iOriginalResolution} = cell(maxPyramidLevels(iOriginalResolution), 1);
        pointErrors_splitBinaryProjectiveLK{iTest}{iOriginalResolution} = cell(maxPyramidLevels(iOriginalResolution), 1);        

%         for iTrackingResolution = iOriginalResolution:length(resolutions)
%             trackingResolution = resolutions{iTrackingResolution};

            for numPyramidLevels = 1:maxPyramidLevels(iOriginalResolution)
%                 disp(sprintf('originalRes:(%d,%d) numPyramidLevels:%d', originalResolution(2), originalResolution(1), numPyramidLevels));

                lkTracker_projective = LucasKanadeTrackerBinary({originalImageResized,originalImageResized}, maskResized, ...
                    'Type', 'homography', 'DebugDisplay', false, ...%                     'UseBlurring', false, ... , 'ApproximateGradientMargins', false
                    'UseNormalization', false, ...
                    'NumScales', numPyramidLevels);
%                     'TrackingResolution', trackingResolution(2:-1:1));

                binaryLK_originalImageResized = computeBinaryExtrema(originalImageResized, binaryLK_extremaFilterWidth, binaryLK_extremaFilterSigma, binaryLK_extremaDerivativeThreshold, true, true);
                lkTracker_binary_projective = LucasKanadeTrackerBinary({binaryLK_originalImageResized,binaryLK_originalImageResized}, maskResized, ...
                    'Type', 'homography', 'DebugDisplay', false, ... %                     'UseBlurring', false, ... , 'ApproximateGradientMargins', false
                    'UseNormalization', false, ...
                    'NumScales', numPyramidLevels);
                
                binaryLK_originalImageResized = computeBinaryExtrema(originalImageResized, binaryLK_extremaFilterWidth, binaryLK_extremaFilterSigma, binaryLK_extremaDerivativeThreshold, true, true);
                lkTracker_split_binary_projective = LucasKanadeTrackerBinary({binaryLK_originalImageResized,binaryLK_originalImageResized}, maskResized, ...
                    'Type', 'homography', 'DebugDisplay', false, ... %                     'UseBlurring', false, ... , 'ApproximateGradientMargins', false
                    'UseNormalization', false, ...
                    'NumScales', numPyramidLevels);

                pointErrors_projectiveLK{iTest}{iOriginalResolution}{numPyramidLevels} = zeros(0, 2);
                pointErrors_binaryProjectiveLK{iTest}{iOriginalResolution}{numPyramidLevels} = zeros(0, 2);
                pointErrors_splitBinaryProjectiveLK{iTest}{iOriginalResolution}{numPyramidLevels} = zeros(0, 2);             
                
%                 warning off

                for iFrame = 2:size(allImages,1)
                    newImageResized = imresize(allImages{iFrame,1}, originalResolution);

%                     disp('normal LK');
                    lkTracker_projective.track({newImageResized,newImageResized}, 'MaxIterations', maxIterations, 'ConvergenceTolerance', convergenceTolerance);
                    
                    binaryLK_newImageResized = computeBinaryExtrema(newImageResized, binaryLK_extremaFilterWidth, binaryLK_extremaFilterSigma, binaryLK_extremaDerivativeThreshold, true, true, true, binaryLK_matchingFilterWidth, binaryLK_matchingFilterSigma);
%                     disp('binary LK');
                    lkTracker_binary_projective.track({binaryLK_newImageResized,binaryLK_newImageResized}, 'MaxIterations', maxIterations, 'ConvergenceTolerance', convergenceTolerance);
                    
                    binarySplitLK_newImageResized = computeBinaryExtrema(newImageResized, binaryLK_extremaFilterWidth, binaryLK_extremaFilterSigma, binaryLK_extremaDerivativeThreshold, true, true, true, binaryLK_matchingFilterWidth, binaryLK_matchingFilterSigma);
%                     disp('binary LK');
                    lkTracker_split_binary_projective.track({binarySplitLK_newImageResized,binarySplitLK_newImageResized}, 'MaxIterations', maxIterations, 'ConvergenceTolerance', convergenceTolerance);

                    if (allImages{iFrame,3} > 0) && isfield(jsonData.sequences{allImages{iFrame,2}}.groundTruth{allImages{iFrame,3}}, 'errorSignalCorners')
                        bottomLineGroundTruth = jsonData.sequences{allImages{iFrame,2}}.groundTruth{allImages{iFrame,3}}.errorSignalCorners;
                        esX = [bottomLineGroundTruth{1}.x, bottomLineGroundTruth{2}.x] / downsampleFactor * scale;
                        esY = [bottomLineGroundTruth{1}.y, bottomLineGroundTruth{2}.y] / downsampleFactor * scale;
                        bottomLineGroundTruth = [esX', esY'];
                        
                        pointErrors_projectiveLK{iTest}{iOriginalResolution}{numPyramidLevels}(end+1, :) = computeGroundTruthLineDifference(bottomLine, lkTracker_projective.tform, templateQuad, scale, bottomLineGroundTruth);
                        pointErrors_binaryProjectiveLK{iTest}{iOriginalResolution}{numPyramidLevels}(end+1, :) = computeGroundTruthLineDifference(bottomLine, lkTracker_binary_projective.tform, templateQuad, scale, bottomLineGroundTruth);
                        pointErrors_splitBinaryProjectiveLK{iTest}{iOriginalResolution}{numPyramidLevels}(end+1, :) = computeGroundTruthLineDifference(bottomLine, lkTracker_split_binary_projective.tform, templateQuad, scale, bottomLineGroundTruth);                        
                        
                    else
                        bottomLineGroundTruth = [];
                    end

                    figure(1); plotResults(newImageResized, templateQuad*scale, lkTracker_projective.tform, bottomLine*scale, bottomLineGroundTruth); title(sprintf('lkTracker projective %dx%d %d %d', originalResolution(2), originalResolution(1), numPyramidLevels, iFrame));
                    figure(2); plotResults(newImageResized, templateQuad*scale, lkTracker_binary_projective.tform, bottomLine*scale, bottomLineGroundTruth); title(sprintf('lkTracker binary projective %dx%d %d %d', originalResolution(2), originalResolution(1), numPyramidLevels, iFrame));
                    figure(3); plotResults(newImageResized, templateQuad*scale, lkTracker_split_binary_projective.tform, bottomLine*scale, bottomLineGroundTruth); title(sprintf('lkTracker split binary projective %dx%d %d %d', originalResolution(2), originalResolution(1), numPyramidLevels, iFrame));                    

                    pause(.03);
%                     pause();
                end
                
                totalString_projectiveLK = createOutputString('projectiveLK', iTest, originalResolution, numPyramidLevels, pointErrors_projectiveLK{iTest}{iOriginalResolution}{numPyramidLevels});
                totalString_binaryProjectiveLK = createOutputString('binaryProjectiveLK', iTest, originalResolution, numPyramidLevels, pointErrors_binaryProjectiveLK{iTest}{iOriginalResolution}{numPyramidLevels});
                totalString_splitBinaryProjectiveLK = createOutputString('binaryProjectiveLK', iTest, originalResolution, numPyramidLevels, pointErrors_splitBinaryProjectiveLK{iTest}{iOriginalResolution}{numPyramidLevels});
                                
                fullResultsString = [fullResultsString, totalString_projectiveLK, sprintf('\n'), totalString_binaryProjectiveLK, sprintf('\n'), totalString_splitBinaryProjectiveLK, sprintf('\n')];
                
                save(outputFilename, 'pointErrors_*', 'fullResultsString');
                
                disp([totalString_projectiveLK, totalString_binaryProjectiveLK]);

%                 warning on
            end % for numPyramidLevels = 1:maxPyramidLevels
%         end % for iTrackingResolution = iOriginalResolution:length(resolutions)
    end % for iOriginalResolution = 1:length(resolutions)

    keyboard
end

% [trackingImages, centeredImage, tiltedImage, shiftedImage] = loadTestImages();

% test various template sizes

keyboard

function warpedLine = warpLine(originalLine, H, centerPoint)
    warpedLineX = ...
        H(1,1)*(originalLine(:,1)-centerPoint(1)) + ...
        H(1,2)*(originalLine(:,2)-centerPoint(2)) + ...
        H(1,3);

    warpedLineY = ...
        H(2,1)*(originalLine(:,1)-centerPoint(1)) + ...
        H(2,2)*(originalLine(:,2)-centerPoint(2)) + ...
        H(2,3);

    warpedLineW = ...
        H(3,1)*(originalLine(:,1)-centerPoint(1)) + ...
        H(3,2)*(originalLine(:,2)-centerPoint(2)) + ...
        H(3,3);

    warpedLineX = (warpedLineX./warpedLineW) + centerPoint(1);
    warpedLineY = (warpedLineY./warpedLineW) + centerPoint(2);

    warpedLine = [warpedLineX, warpedLineY];

    return;

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

    return;

function plotResults(im, corners, H, bottomLine, bottomLineGroundTruth)
    cen = mean(corners,1);

    order = [1,2,3,4,1];

    hold off;
    imshow(im);
    hold on;
    plot(corners(order,1), corners(order,2), 'k--', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');

    tempx = ...
        H(1,1)*(corners(order,1)-cen(1)) + ...
        H(1,2)*(corners(order,2)-cen(2)) + ...
        H(1,3);

    tempy = ...
        H(2,1)*(corners(order,1)-cen(1)) + ...
        H(2,2)*(corners(order,2)-cen(2)) + ...
        H(2,3);

    tempw = ...
        H(3,1)*(corners(order,1)-cen(1)) + ...
        H(3,2)*(corners(order,2)-cen(2)) + ...
        H(3,3);

    plot((tempx./tempw) + cen(1), (tempy./tempw) + cen(2), 'b', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');

    bottomLineWarped = warpLine(bottomLine, H, cen);

    plot(bottomLineWarped(:,1), bottomLineWarped(:,2), 'r', 'LineWidth', 2, 'Tag', 'TrackRect');
    scatter(bottomLineWarped(:,1), bottomLineWarped(:,2), 'ro', 'LineWidth', 2, 'Tag', 'TrackRect');

    if exist('bottomLineGroundTruth', 'var') && ~isempty(bottomLineGroundTruth)
        scatter(bottomLineGroundTruth(:,1), bottomLineGroundTruth(:,2),...
            'g+', 'LineWidth', 2, 'Tag', 'TrackRect');
    end

function distance = computeGroundTruthLineDifference(bottomLine, homography, templateQuad, scale, bottomLineGroundTruth)
    warpedBottomLine = warpLine(bottomLine*scale, homography, mean(templateQuad*scale, 1));                        
    bottomLineGroundTruthScaled = bottomLineGroundTruth / scale;
    warpedBottomLineScaled = warpedBottomLine / scale;                        
    distance = sqrt((bottomLineGroundTruthScaled(:,1) - warpedBottomLineScaled(:,1)).^2 + (bottomLineGroundTruthScaled(:,2) - warpedBottomLineScaled(:,2)).^2);
    
function totalString = createOutputString(testName, iTest, originalResolution, numPyramidLevels, pointErrors)
    totalString = sprintf('Results for Test %s:%d originalResolution:(%d,%d) numPyramidLevels:%d = [', testName, iTest, originalResolution(2), originalResolution(1), numPyramidLevels);
    maxValues = max(pointErrors, [], 2);
    for ii = 1:length(maxValues)
        totalString = [totalString, num2str(maxValues(ii)), ', '];
    end
    totalString = [totalString(1:(end-2)), ']'];
    
    
    