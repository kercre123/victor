% function run_testTrackers(allTestsFilename)

% run_testTrackers('C:/Anki/systemTestImages/allDockerTests.json');

function run_testTrackers(allTestsFilename)

resolutions = {[480,640], [240,320], [120,160], [60,80], [30,40]};
maxPyramidLevels = [5, 4, 3, 1, 1, 1];
maxIterations = 50;
convergenceTolerance = 0.25;

allTests = loadjson(allTestsFilename);

slashIndexes = strfind(allTestsFilename, '/');
dataPath = allTestsFilename(1:(slashIndexes(end)));

for iTest = 1:length(allTests)
    jsonData = loadjson([dataPath, allTests.tests{iTest}]);
    
    if ~iscell(jsonData.sequences)
        jsonData.sequences = { jsonData.sequences };
    end
            
    allImages = {};
    
    for iSequence = 1:length(jsonData.sequences)
        filenamePattern = jsonData.sequences{iSequence}.filenamePattern;
        for iFrame = 1:length(jsonData.sequences{iSequence}.frameNumbers)
            frameNumber = jsonData.sequences{iSequence}.frameNumbers(iFrame);
            allImages{end+1} = imread([dataPath, sprintf(filenamePattern, frameNumber)]);
        end
    end
    
    frameIndex = findFrameNumberIndex(jsonData, 1, 1);
    
    templateCorners = jsonData.sequences{1}.groundTruth{frameIndex}.templateCorners;
    xCoordinates = [templateCorners{1}.x, templateCorners{2}.x, templateCorners{3}.x, templateCorners{4}.x];
    yCoordinates = [templateCorners{1}.y, templateCorners{2}.y, templateCorners{3}.y, templateCorners{4}.y];
    templateQuad = [xCoordinates', yCoordinates'];
    
    mask = zeros(size(allImages{1}));
    mask = roipoly(mask, xCoordinates, yCoordinates);

    for iOriginalResolution = 1:length(resolutions)
        originalResolution = resolutions{iOriginalResolution};
        
        originalImageResized = imresize(allImages{1}, originalResolution);
        maskResized = imresize(mask, originalResolution);
        
        scale = originalResolution(1) / 480;
        
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
                
                for iFrame = 2:length(allImages)
                    newImageResized = imresize(allImages{iFrame}, originalResolution);
    
                    lkTracker_projective.track(newImageResized, 'MaxIterations', maxIterations, 'ConvergenceTolerance', convergenceTolerance);
                    
%                     disp('Matlab LK projective:');
%                     disp(lkTracker_projective.tform);
                    plotResults(originalImageResized, newImageResized, templateQuad*scale, lkTracker_projective.tform);
                    
                    pause(.01);
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
    
function plotResults(im1, im2, corners, H)

    cen = mean(corners,1);

    order = [1,2,3,4,1];

%     subplot(1,2,1);
%     hold off;
%     imshow(im1)

%     subplot(1,2,2);
    hold off;
    imshow(im2);
    hold on;
    plot(corners(order,1), corners(order,2), 'b--', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');

    tempx = H(1,1)*(corners(order,1)-cen(1)) + ...
        H(1,2)*(corners(order,2)-cen(2)) + ...
        H(1,3) + cen(1);

    tempy = H(2,1)*(corners(order,1)-cen(1)) + ...
        H(2,2)*(corners(order,2)-cen(2)) + ...
        H(2,3) + cen(2);

    plot(tempx, tempy, 'r', ...
                    'LineWidth', 2, 'Tag', 'TrackRect');
    