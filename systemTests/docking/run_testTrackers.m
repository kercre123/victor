% function run_testTrackers(allTestsFilename)

% run_testTrackers('C:/Anki/systemTestImages/allDockerTests.json');

function run_testTrackers(allTestsFilename)

mainResolution = [480,640];
downsampleFactor = 1;

resolutions = {[480,640], [240,320], [120,160], [60,80], [30,40]};
maxPyramidLevels = [5, 4, 3, 1, 1, 1];
maxIterations = 50;
convergenceTolerance = 0.1;
useUndistortion = false;

if useUndistortion
    load('Z:\Documents\Box Documents\Cozmo SE\calibCozmoProto1_head.mat');
    cam = Camera('calibration', calibCozmoProto1_head);
end

allTests = loadjson(allTestsFilename);

slashIndexes = strfind(allTestsFilename, '/');
dataPath = allTestsFilename(1:(slashIndexes(end)));

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

    for iOriginalResolution = 2:length(resolutions)
        originalResolution = resolutions{iOriginalResolution};
        
        originalImageResized = imresize(allImages{1,1}, originalResolution);
        maskResized = imresize(mask, originalResolution);
        
        scale = originalResolution(1) / mainResolution(1) * downsampleFactor;
        
%         for iTrackingResolution = iOriginalResolution:length(resolutions)
%             trackingResolution = resolutions{iTrackingResolution};
            
            for numPyramidLevels = 1:maxPyramidLevels(iOriginalResolution)
                disp(sprintf('originalRes:(%d,%d) numPyramidLevels:%d', originalResolution(2), originalResolution(1), numPyramidLevels));
                
                lkTracker_projective = LucasKanadeTracker(originalImageResized, maskResized, ...
                    'Type', 'homography', 'DebugDisplay', false, ...
                    'UseBlurring', false, 'UseNormalization', false, ...
                    'NumScales', numPyramidLevels, 'ApproximateGradientMargins', false);
%                     'TrackingResolution', trackingResolution(2:-1:1));
                
                warning off
                
                for iFrame = 2:size(allImages,1)
                    newImageResized = imresize(allImages{iFrame,1}, originalResolution);
    
                    lkTracker_projective.track(newImageResized, 'MaxIterations', maxIterations, 'ConvergenceTolerance', convergenceTolerance);
                    
%                     disp('Matlab LK projective:');
%                     disp(lkTracker_projective.tform);
                    
                    if (allImages{iFrame,3} > 0) && isfield(jsonData.sequences{allImages{iFrame,2}}.groundTruth{allImages{iFrame,3}}, 'errorSignalCorners') 
                        bottomLineGroundTruth = jsonData.sequences{allImages{iFrame,2}}.groundTruth{allImages{iFrame,3}}.errorSignalCorners;
                        esX = [bottomLineGroundTruth{1}.x, bottomLineGroundTruth{2}.x] / downsampleFactor;
                        esY = [bottomLineGroundTruth{1}.y, bottomLineGroundTruth{2}.y] / downsampleFactor;
                        bottomLineGroundTruth = [esX', esY'];
                    else
                        bottomLineGroundTruth = [];
                    end

                    plotResults(newImageResized, templateQuad*scale, lkTracker_projective.tform, bottomLine*scale, bottomLineGroundTruth*scale);
                    
                    pause(.1);
%                     pause();
                end
                
                warning on
            end % for numPyramidLevels = 1:maxPyramidLevels
%         end % for iTrackingResolution = iOriginalResolution:length(resolutions)
    end % for iOriginalResolution = 1:length(resolutions)
    
    
    keyboard
end

% [trackingImages, centeredImage, tiltedImage, shiftedImage] = loadTestImages();

% test various template sizes

keyboard

function index = findFrameNumberIndex(jsonData, sequenceNumberIndex, frameNumberIndex)
    index = -1;

    if ~isfield(jsonData.sequences{sequenceNumberIndex}, 'groundTruth')
        return;
    end
    
    for i = 1:length(jsonData.sequences{sequenceNumberIndex}.groundTruth)
        curFrameNumber = jsonData.sequences{sequenceNumberIndex}.groundTruth{i}.frameNumber;
        queryFrameNumber = jsonData.sequences{sequenceNumberIndex}.frameNumbers(frameNumberIndex);

    %     disp(sprintf('%d) %d %d', i, curFrameNumber, queryFrameNumber));

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
                
    bottomLineWarpedX = ...
        H(1,1)*(bottomLine(:,1)-cen(1)) + ...
        H(1,2)*(bottomLine(:,2)-cen(2)) + ...
        H(1,3);
    
    bottomLineWarpedY = ...
        H(2,1)*(bottomLine(:,1)-cen(1)) + ...
        H(2,2)*(bottomLine(:,2)-cen(2)) + ...
        H(2,3);                
    
    bottomLineWarpedW = ...
        H(3,1)*(bottomLine(:,1)-cen(1)) + ...
        H(3,2)*(bottomLine(:,2)-cen(2)) + ...
        H(3,3);
    
    plot((bottomLineWarpedX./bottomLineWarpedW) + cen(1),...
         (bottomLineWarpedY./bottomLineWarpedW) + cen(2), 'r', ...
         'LineWidth', 2, 'Tag', 'TrackRect');
     
     scatter((bottomLineWarpedX./bottomLineWarpedW) + cen(1),...
         (bottomLineWarpedY./bottomLineWarpedW) + cen(2), 'ro', ...
         'LineWidth', 2, 'Tag', 'TrackRect');
    
    if exist('bottomLineGroundTruth', 'var') && ~isempty(bottomLineGroundTruth)
        scatter(bottomLineGroundTruth(:,1), bottomLineGroundTruth(:,2),...
            'g+', 'LineWidth', 2, 'Tag', 'TrackRect');
    end
     
     