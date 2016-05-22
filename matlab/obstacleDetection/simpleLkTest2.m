% function simpleLkTest2(varargin)

% simpleLkTest2('useLiveFeed', true);
% simpleLkTest2('useLiveFeed', false, 'runFlowComputation', false, 'filenamePattern','/Users/pbarnum/Documents/datasets/stereo/stereo_%sRectified_%d.png', 'whichImageNumbers', 0:3090, 'pauseForEachFrame', true);

% simpleLkTest2('useLiveFeed', false, 'runFlowComputation', false, 'runStereoComputation', false, 'runStereoBmComputation', true, 'filenamePattern','/Users/pbarnum/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_%sRectified_%d.png', 'whichImageNumbers', 0:50, 'pauseForEachFrame', false, 'saveOutputToFile', true, 'outputFilenamePrefix', '/Users/pbarnum/Documents/tmp/output2/lkOutput3twoPass_');


% Show voxels
% simpleLkTest2('useLiveFeed', false, 'runFlowComputation', false, 'runStereoComputation', false, 'runStereoBmComputation', true, 'filenamePattern','/Users/pbarnum/Documents/Anki/products-cozmo-large-files/stereo/stereoDataset/stereo_%sRectified_%d.png', 'whichImageNumbers', 0:50, 'pauseForEachFrame', false, 'drawVoxels', true);

% simpleLkTest2('useLiveFeed', true, 'pauseForEachFrame', false, 'drawVoxels', true);

function simpleLkTest2(varargin)
    
    disp('Starting...');
    
    useLiveFeed = true;
    cameraIds = [0,1];
    
    filenamePattern = '/Users/pbarnum/Documents/datasets/stereo/stereo_%sRectified_%d.png';
    whichImageNumbers = 0:3090;
    pauseForEachFrame = false;
    
    useStereoCalibration = true;
    computeStereoBm = false;
    drawVoxels = false;
    voxelsUseDepthColor = true;
    
    runFlowComputation = false;
    runStereoComputation = false;
    runStereoBmComputation = true;
    
    onlyHorizontalStereoFlow = true;
    
    useBlur = false;
    binarizeImage = false;
    
    flowOffsets = {[0,0]};
    
    stereoOffsets = {[0,0]};
    %     stereoOffsets = {[0,0], [-32,0], [-64,0], [-96,0], [-128,0]};
    %     stereoOffsets = {[0,0], [-32,0], [-64,0], [-96,0], [-128,0]};
    %     stereoOffsets = {[0,0], [-16,0], [-32,0], [-48,0], [-64,0], [-80,0], [-96,0], [-112,0], [-128,0]};
    
    saveOutputToFile = false;
    outputFilenamePrefix = '/Users/pbarnum/Documents/tmp/output/lkOutput2_';
    
    leftRightCheck = true;
    leftRightCheckThreshold = 7; % TODO: pick a good threshold
    
    stereoDisparityColors196 = [0,0,0;0,3,255;0,9,255;0,14,255;0,19,255;0,24,255;0,30,255;0,36,255;0,41,255;0,46,255;0,52,255;0,57,255;0,63,255;0,68,255;0,73,255;0,78,255;0,84,255;0,90,255;0,95,255;0,100,255;0,106,255;0,111,255;0,117,255;0,122,255;0,128,255;0,133,255;0,138,255;0,144,255;0,149,255;0,155,255;0,160,255;0,165,255;0,171,255;0,177,255;0,182,255;0,187,255;0,192,255;0,198,255;0,204,255;0,209,255;0,215,255;0,220,255;0,225,255;0,231,255;0,236,255;0,241,255;0,246,255;0,252,255;0,255,255;0,255,255;0,255,252;0,255,246;0,255,241;0,255,236;0,255,231;0,255,225;0,255,220;0,255,215;0,255,209;0,255,204;0,255,198;0,255,192;0,255,187;0,255,182;0,255,177;0,255,171;0,255,165;0,255,160;0,255,155;0,255,149;0,255,144;0,255,138;0,255,133;0,255,128;0,255,122;0,255,117;0,255,111;0,255,106;0,255,100;0,255,95;0,255,90;0,255,84;0,255,78;0,255,73;0,255,68;0,255,63;0,255,57;0,255,52;0,255,46;0,255,41;0,255,36;0,255,30;0,255,24;0,255,19;0,255,14;0,255,9;0,255,3;0,255,0;0,255,0;3,255,0;9,255,0;14,255,0;19,255,0;24,255,0;30,255,0;36,255,0;41,255,0;46,255,0;52,255,0;57,255,0;63,255,0;68,255,0;73,255,0;78,255,0;84,255,0;90,255,0;95,255,0;100,255,0;106,255,0;111,255,0;117,255,0;122,255,0;128,255,0;133,255,0;138,255,0;144,255,0;149,255,0;155,255,0;160,255,0;165,255,0;171,255,0;177,255,0;182,255,0;187,255,0;192,255,0;198,255,0;204,255,0;209,255,0;215,255,0;220,255,0;225,255,0;231,255,0;236,255,0;241,255,0;246,255,0;252,255,0;255,255,0;255,255,0;255,252,0;255,246,0;255,241,0;255,236,0;255,231,0;255,225,0;255,220,0;255,215,0;255,209,0;255,204,0;255,198,0;255,192,0;255,187,0;255,182,0;255,177,0;255,171,0;255,165,0;255,160,0;255,155,0;255,149,0;255,144,0;255,138,0;255,133,0;255,128,0;255,122,0;255,117,0;255,111,0;255,106,0;255,100,0;255,95,0;255,90,0;255,84,0;255,78,0;255,73,0;255,68,0;255,63,0;255,57,0;255,52,0;255,46,0;255,41,0;255,36,0;255,30,0;255,24,0;255,19,0;255,14,0;255,9,0;255,3,0;255,0,0];
    stereoDisparityColors128 = [0,0,0;0,7,255;0,16,255;0,24,255;0,32,255;0,41,255;0,49,255;0,57,255;0,65,255;0,73,255;0,82,255;0,90,255;0,99,255;0,107,255;0,115,255;0,124,255;0,132,255;0,140,255;0,148,255;0,156,255;0,165,255;0,173,255;0,182,255;0,190,255;0,198,255;0,207,255;0,215,255;0,223,255;0,231,255;0,239,255;0,248,255;0,255,255;0,255,255;0,255,248;0,255,239;0,255,231;0,255,223;0,255,215;0,255,207;0,255,198;0,255,190;0,255,182;0,255,173;0,255,165;0,255,156;0,255,148;0,255,140;0,255,132;0,255,124;0,255,115;0,255,107;0,255,99;0,255,90;0,255,82;0,255,73;0,255,65;0,255,57;0,255,49;0,255,41;0,255,32;0,255,24;0,255,16;0,255,7;0,255,0;0,255,0;7,255,0;16,255,0;24,255,0;32,255,0;41,255,0;49,255,0;57,255,0;65,255,0;73,255,0;82,255,0;90,255,0;99,255,0;107,255,0;115,255,0;124,255,0;132,255,0;140,255,0;148,255,0;156,255,0;165,255,0;173,255,0;182,255,0;190,255,0;198,255,0;207,255,0;215,255,0;223,255,0;231,255,0;239,255,0;248,255,0;255,255,0;255,255,0;255,248,0;255,239,0;255,231,0;255,223,0;255,215,0;255,207,0;255,198,0;255,190,0;255,182,0;255,173,0;255,165,0;255,156,0;255,148,0;255,140,0;255,132,0;255,124,0;255,115,0;255,107,0;255,99,0;255,90,0;255,82,0;255,73,0;255,65,0;255,57,0;255,49,0;255,41,0;255,32,0;255,24,0;255,16,0;255,7,0;255,0,0];
    stereoDisparityColors96 = [0,0,0;0,12,255;0,22,255;0,34,255;0,45,255;0,56,255;0,67,255;0,78,255;0,89,255;0,100,255;0,111,255;0,122,255;0,133,255;0,144,255;0,155,255;0,166,255;0,177,255;0,188,255;0,199,255;0,211,255;0,221,255;0,233,255;0,243,255;0,254,255;0,255,254;0,255,243;0,255,233;0,255,221;0,255,211;0,255,199;0,255,188;0,255,177;0,255,166;0,255,155;0,255,144;0,255,133;0,255,122;0,255,111;0,255,100;0,255,89;0,255,78;0,255,67;0,255,56;0,255,45;0,255,34;0,255,22;0,255,12;0,255,1;1,255,0;12,255,0;22,255,0;34,255,0;45,255,0;56,255,0;67,255,0;78,255,0;89,255,0;100,255,0;111,255,0;122,255,0;133,255,0;144,255,0;155,255,0;166,255,0;177,255,0;188,255,0;199,255,0;211,255,0;221,255,0;233,255,0;243,255,0;254,255,0;255,254,0;255,243,0;255,233,0;255,221,0;255,211,0;255,199,0;255,188,0;255,177,0;255,166,0;255,155,0;255,144,0;255,133,0;255,122,0;255,111,0;255,100,0;255,89,0;255,78,0;255,67,0;255,56,0;255,45,0;255,34,0;255,22,0;255,12,0;255,1,0];
    stereoDisparityColors64 = [0,0,0;0,20,255;0,36,255;0,53,255;0,70,255;0,86,255;0,103,255;0,119,255;0,136,255;0,152,255;0,169,255;0,185,255;0,202,255;0,220,255;0,235,255;0,251,255;0,255,251;0,255,235;0,255,220;0,255,202;0,255,185;0,255,169;0,255,152;0,255,136;0,255,119;0,255,103;0,255,86;0,255,70;0,255,53;0,255,36;0,255,20;0,255,4;4,255,0;20,255,0;36,255,0;53,255,0;70,255,0;86,255,0;103,255,0;119,255,0;136,255,0;152,255,0;169,255,0;185,255,0;202,255,0;220,255,0;235,255,0;251,255,0;255,251,0;255,235,0;255,220,0;255,202,0;255,185,0;255,169,0;255,152,0;255,136,0;255,119,0;255,103,0;255,86,0;255,70,0;255,53,0;255,36,0;255,20,0;255,4,0];
    stereoDisparityColors32 = [0,0,0;0,45,255;0,78,255;0,111,255;0,144,255;0,177,255;0,210,255;0,242,255;0,255,242;0,255,210;0,255,177;0,255,144;0,255,111;0,255,78;0,255,45;0,255,13;13,255,0;45,255,0;78,255,0;111,255,0;144,255,0;177,255,0;210,255,0;242,255,0;255,242,0;255,210,0;255,177,0;255,144,0;255,111,0;255,78,0;255,45,0;255,13,0];
    stereoDisparityColors16 = [0,0,0;0,94,255;0,161,255;0,225,254;0,254,225;0,255,161;0,255,94;1,255,30;30,255,1;94,255,0;161,255,0;225,254,0;254,225,0;255,161,0;255,94,0;255,30,0];
    
    %windowSizes = {[15,15], [31,31], [51,51]};
    %     windowSizes = {[31,31]};
    windowSizes = {[51,51]};
    
    imageSize = [240, 320];
    
    numPointsPerDimension = 25;
    
    filterFlowWithMinimanessThreshold = false;
    minimanessThreshold = 0.5;
    
    filterFlowWithErrThreshold = true;
    maxErr = 0.2;
    
    % Parameters for lucas kanade optical flow
    flow_lk_params = struct(...
        'winSize', [31,31],...
        'maxLevel', 4,...
        'criteria', struct('type', 'Count+EPS', 'maxCount', 10, 'epsilon', 0.03),...
        'minEigThreshold', 5e-3);
    
    stereo_lk_params = struct(...
        'winSize' , [31,31],...
        'maxLevel', 3,...
        'criteria', struct('type', 'Count+EPS', 'maxCount', 10, 'epsilon', 0.03),...
        'minEigThreshold', 1e-4);
    
    stereo_lk_maxAngle = pi/16;
    
    if runStereoBmComputation
        numDisparities = 128;
        disparityType = 'sgbm';
        interpolateMissing = true;
        
        if strcmpi(disparityType, 'bm')
            disparityWindowSize = 51;
            stereo = cv.StereoBM('Preset', 'Basic', 'NDisparities', 0, 'SADWindowSize', disparityWindowSize);
        elseif strcmpi(disparityType, 'sgbm')
            stereo = cv.StereoSGBM(...
                'MinDisparity', 0,...
                'NumDisparities', numDisparities,...
                'SADWindowSize', 5,...
                'P1', 600,...
                'P2', 2400,...
                'Disp12MaxDiff', 2,...
                'PreFilterCap', 16,...
                'UniquenessRatio', 5,...
                'SpeckleWindowSize', 1000,...
                'SpeckleRange', 4,...
                'FullDP', false);
        end
    end % if runStereoBmComputation
    
    parseVarargin(varargin{:});
    
    if length(cameraIds) == 1
        useStereoCalibration = false;
    end
    
    undistortMaps = [];
    if useStereoCalibration
        [stereoProcessor, undistortMaps, masks] = initStereo(128, 'bm');
    else
        masks = {ones(imageSize), ones(imageSize)};
    end
    
    reverseFlowOffsets = cell(size(flowOffsets));
    for i = 1:length(flowOffsets)
        reverseFlowOffsets{i} = -flowOffsets{i};
    end
    
    reverseStereoOffsets = cell(size(flowOffsets));
    for i = 1:length(stereoOffsets)
        reverseStereoOffsets{i} = -stereoOffsets{i};
    end
    
    iImageNumber = 0;
    
    if useLiveFeed
        videoCaptures = cell(length(cameraIds), 1);
        for i = 1:length(cameraIds)
            videoCaptures{i} = cv.VideoCapture(cameraIds(i));
            disp(sprintf('Opened camera %d (id=%d)', i, cameraIds(i)))
        end
    end
    
    %     [points0x, points0y] = meshgrid(linspace(0, imageSize(2)-1, numPointsPerDimension),linspace(0, imageSize(1)-1, numPointsPerDimension));
    [points0x, points0y] = meshgrid(linspace(25, imageSize(2)-26, numPointsPerDimension),linspace(25, imageSize(1)-26, numPointsPerDimension));
    points0Raw = [points0x(:), points0y(:)];
    points0 = cell(1, size(points0Raw,1));
    for i = 1:size(points0Raw,1)
        points0{i} = points0Raw(i,:);
    end
    
    while true
        iImageNumber = iImageNumber + 1;
        
        if useLiveFeed
            images = captureImages(videoCaptures, undistortMaps);
            
            for i = 2:length(images)
                assert(min(size(images{1}) == size(images{i})));
            end
            
            for i = 1:length(images)
                images{i} = imresize(images{i}, imageSize);
            end
            
            if useBlur
                %                 filt = fspecial('gaussian',[21,21],5);
                filt = fspecial('gaussian',[5,5],1);
                for i = 1:length(images)
                    images{i} = imfilter(images{i}, filt);
                    images{i} = cv.equalizeHist(images{i});
                end
            end
            
            if binarizeImage
                for i = 1:length(images)
                    images{i} = uint8(255*simpleDetector_step1_computeCharacteristicScale(0.025, size(images{i},1), size(images{i},2), 2, im2double(images{i}), 1, 1, EmbeddedConversionsManager(), 0, 0));
                end
            end
        else
            if iImageNumber > length(whichImageNumbers)
                break;
            end
            
            leftFilename = sprintf(filenamePattern, 'left', whichImageNumbers(iImageNumber));
            rightFilename = sprintf(filenamePattern, 'right', whichImageNumbers(iImageNumber));
            images = {rgb2gray2(imread(leftFilename)), rgb2gray2(imread(rightFilename))};
            disp(sprintf('Image %d) %s', iImageNumber, leftFilename))
        end
        
        if iImageNumber == 1
            lastImages = images;
            continue
        end
        
        % Compute the flow
        if runFlowComputation
            allFlowPoints = {};
            for iImage = 1:length(images)
                curImage = images{iImage};
                mask = masks{iImage};
                lastImage = lastImages{iImage};
                
                curFlowPoints = {};
                
                for iWindowSize = 1:length(windowSizes)
                    [points1, status, err, goodPoints0, goodPoints1] = computeFlow(lastImage, curImage, points0, windowSizes{iWindowSize}, flow_lk_params, filterFlowWithErrThreshold, maxErr, filterFlowWithMinimanessThreshold, flowOffsets, false);
                    
                    if leftRightCheck
                        [points0B, statusB, errB, ~, ~] = computeFlow(curImage, lastImage, points1, windowSizes{iWindowSize}, flow_lk_params, filterFlowWithErrThreshold, maxErr, filterFlowWithMinimanessThreshold, reverseFlowOffsets, false);
                        
                        for iPoint = 1:length(points0)
                            dist = sqrt(sum((points0{iPoint} - points0B{iPoint}).^2));
                            if dist > leftRightCheckThreshold
                                statusB(iPoint) = 0;
                            end
                        end % for iPoint = 1:length(points0)
                        
                        goodPoints0B = points0B(statusB==1);
                        goodPoints1B = points1(statusB==1);
                    else
                        points0B = [];
                        statusB = [];
                        errB = [];
                        goodPoints0B = [];
                        goodPoints1B = [];
                    end
                    
                    curFlowPoints{end+1} = struct(...
                        'points1', {points1},...
                        'status', {status},...
                        'err', {err},...
                        'goodPoints0', {goodPoints0},...
                        'goodPoints1', {goodPoints1},...
                        'points0B', {points0B}, ...
                        'statusB', {statusB},...
                        'errB', {errB},...
                        'goodPoints0B', {goodPoints0B},...
                        'goodPoints1B', {goodPoints1B},...
                        'curImage', {curImage},...
                        'lastImage', {lastImage},...
                        'windowSize', windowSizes(iWindowSize),...
                        'iImage', iImage); %#ok<AGROW>
                end % for windowSize = 1:length(windowSizes)
                
                allFlowPoints{end+1} = curFlowPoints; %#ok<AGROW>
            end % for iImage = 1:length(images)
            
            % Draw the flow
            flowImagesToShow = {};
            for iPoint = 1:length(allFlowPoints)
                for jPoint = 1:length(allFlowPoints{iPoint})
                    pointsToShow = allFlowPoints{iPoint}{jPoint};
                    keypointImage = drawKeypointMatches(pointsToShow.curImage, pointsToShow.goodPoints0, pointsToShow.goodPoints1, stereoDisparityColors128);
                    flowImagesToShow{end+1} = keypointImage; %#ok<AGROW>
                    
                    if leftRightCheck
                        keypointImage = drawKeypointMatches(pointsToShow.curImage, pointsToShow.goodPoints0B, pointsToShow.goodPoints1B, stereoDisparityColors128);
                        flowImagesToShow{end+1} = keypointImage; %#ok<AGROW>
                    end
                end % for jFlow in allFlowPoints{iFlow}
            end % for iFlow = 1:length(allFlowPoints)
            
            imshows(flowImagesToShow, 1);
            
            if saveOutputToFile
                outImage = export_fig();
                imwrite(outImage, [outputFilenamePrefix, sprintf('flow_%d.png', whichImageNumbers(iImageNumber-1))]);
            end
        end % if runFlowComputation
        
        % Stereo
        if runStereoComputation && length(images) == 2
            stereoImagesToShow = {};
            
            % Compute the stereo
            allStereoPoints = {};
            for iWindowSize = 1:length(windowSizes)
                [points1, status, err, goodPoints0, goodPoints1] = computeFlow(images{1}, images{2}, points0, windowSizes{iWindowSize}, stereo_lk_params, false, 0, false, stereoOffsets, onlyHorizontalStereoFlow);
                
                if leftRightCheck
                    [points0B, statusB, errB, ~, ~] = computeFlow(images{2}, images{1}, points1, windowSizes{iWindowSize}, stereo_lk_params, false, 0, false, reverseStereoOffsets, onlyHorizontalStereoFlow);
                    
                    for iPoint = 1:length(points0)
                        dist = sqrt(sum((points0{iPoint} - points0B{iPoint}).^2));
                        if dist > leftRightCheckThreshold
                            statusB(iPoint) = 0;
                        end
                    end % for iPoint = 1:length(points0)
                    
                    goodPoints0B = points0B(statusB==1);
                    goodPoints1B = points1(statusB==1);
                else
                    points0B = [];
                    statusB = [];
                    errB = [];
                    goodPoints0B = [];
                    goodPoints1B = [];
                end
                
                allStereoPoints{end+1} = struct(...
                    'points1', {points1},...
                    'status', {status},...
                    'err', {err},...
                    'goodPoints0', {goodPoints0},...
                    'goodPoints1', {goodPoints1},...
                    'points0B', {points0B}, ...
                    'statusB', {statusB},...
                    'errB', {errB},...
                    'goodPoints0B', {goodPoints0B},...
                    'goodPoints1B', {goodPoints1B},...
                    'curImage', {images{1}},...
                    'lastImage', {images{2}},...
                    'windowSize', windowSizes(iWindowSize)); %#ok<AGROW>
            end % for iWindowSize = 1:length(windowSizes)
            
            % Draw the stereo matches
            for iPoints = 1:length(allStereoPoints)
                pointsToShow = allStereoPoints{iPoints};
                
                
                keypointImage = drawKeypointMatches(pointsToShow.lastImage, pointsToShow.goodPoints0, pointsToShow.goodPoints1, stereoDisparityColors96, false);
                stereoImagesToShow{end+1} = keypointImage; %#ok<AGROW>
                
                if leftRightCheck
                    keypointImage = drawKeypointMatches(pointsToShow.curImage, pointsToShow.goodPoints0B, pointsToShow.goodPoints1B, stereoDisparityColors96, false);
                    stereoImagesToShow{end+1} = keypointImage; %#ok<AGROW>
                end
            end % for iFlow = 1:length(allFlowPoints)
            
            imshows(stereoImagesToShow, 2);
            
            if saveOutputToFile
                outImage = export_fig();
                imwrite(outImage, [outputFilenamePrefix, sprintf('stereo_%d.png', whichImageNumbers(iImageNumber-1))]);
            end
        end % if runStereoComputation && length(images) == 2
        
        if runStereoBmComputation && length(images) == 2
            imageScale = size(images{1},1) / imageSize(1);
            
            leftIm = images{1};
            rightIm = images{2};
            
            leftIm = medfilt2(leftIm, [3,3]);
            rightIm = medfilt2(rightIm, [3,3]);
            
            filt = fspecial('gaussian',[9,9],2);
            leftIm = imfilter(leftIm, filt);
            rightIm = imfilter(rightIm, filt);
            
            if imageScale == 2
                leftIm = uint8((double(leftIm(1:2:end,1:2:end)) + double(leftIm(2:2:end,1:2:end)) + double(leftIm(1:2:end,2:2:end)) + double(leftIm(2:2:end,2:2:end))) / 4);
                rightIm = uint8((double(rightIm(1:2:end,1:2:end)) + double(rightIm(2:2:end,1:2:end)) + double(rightIm(1:2:end,2:2:end)) + double(rightIm(2:2:end,2:2:end))) / 4);
                stereoColormap = stereoDisparityColors32;
            elseif imageScale < 1
                keyboard
            else
                leftIm = imresize(leftIm, imageSize);
                rightIm = imresize(rightIm, imageSize);
                stereoColormap = stereoDisparityColors64;
            end
            
            tic
            disparity = stereo.compute(leftIm, rightIm);
            toc
            
            % TODO: add mask
            disparity(:, [1:20, (end-20:end)]) = 0;
            disparity([1:20, (end-20):end], :) = 0;
            
            if interpolateMissing
                maxInterpolationGap = 5;
                minInterpolationGapRatio = 0.85;
                
                tic
                for y = 1:size(disparity,1)
                    validXIndexes = find(disparity(y,:) > 0);
                    
                    for iValidX = 2:length(validXIndexes)
                        startX = validXIndexes(iValidX-1);
                        endX = validXIndexes(iValidX);
                        
                        startDisparity = double(disparity(y, startX));
                        endDisparity = double(disparity(y, endX));
                        
                        gap = abs(endDisparity-startDisparity);
                        gapRatio = min(endDisparity/startDisparity, startDisparity/endDisparity);
                        
                        if (gap <= maxInterpolationGap) || (gapRatio > minInterpolationGapRatio)
                            alphas = linspace(0, 1, endX-startX+1);
                            
                            alphas = alphas(2:(end-1));
                            
                            if ~isempty(alphas)
                                interpolatedValues = (1-alphas) .* startDisparity + alphas .*endDisparity;
                                
                                disparity(y, (startX+1):(endX-1)) = interpolatedValues;
                            end
                        end
                    end % for iValidX = 2:(length(validXIndexes)-1)
                end % for y = 1:size(disparity,1)
                toc
            end
            
            % TODO: add mask
            disparity(:, [1:20, (end-20:end)]) = 0;
            disparity([1:20, (end-20):end], :) = 0;
            
            disparityToShow = uint8(bitshift(disparity, -5));
            
            colorDisparityToShow = zeros([size(disparityToShow), 3], 'uint8');
            disparityToShow = disparityToShow + 1;
            disparityToShow(disparityToShow > length(stereoColormap)) = length(stereoColormap);
            for y = 1:size(disparityToShow,1)
                colorDisparityToShow(y,1:size(disparityToShow,2),:) = stereoColormap(disparityToShow(y,1:size(disparityToShow,2)),:);
            end
            
            disparityToShowBoth = [leftIm, disparityToShow];
            colorDisparityToShowBoth = [repmat(leftIm,[1,1,3]), colorDisparityToShow];
            
            imshows({disparityToShowBoth,0,colorDisparityToShowBoth}, 3);
            
            if drawVoxels
                validInds = find(disparity > 0);
                [xs, ys] = meshgrid(1:size(disparity,2), 1:size(disparity,1));
                xs = xs - mean(xs(:));
                ys = ys - mean(ys(:));
                
                voxels = zeros([6,length(validInds)], 'single');
                voxels(1,:) = -xs(validInds);
                voxels(2,:) = -ys(validInds);
                %scaledDepths = 1000000 * (1 ./ single(disparity(validInds)));
                %scaledDepths(scaledDepths>5000) = 5000;
                %scaledDepths = scaledDepths - median(scaledDepths(:));
                %scaledDepths(scaledDepths>5000) = 5000;
                %                 scaledDepths = scaledDepths - median(scaledDepths(:));
                scaledDepths = -single(disparity(validInds));
                voxels(3,:) = scaledDepths;
                
                if voxelsUseDepthColor
                    rc = colorDisparityToShow(:,:,1);
                    gc = colorDisparityToShow(:,:,2);
                    bc = colorDisparityToShow(:,:,3);
                    
                    voxels(4,:) = single(rc(validInds)') / 255;
                    voxels(5,:) = single(gc(validInds)') / 255;
                    voxels(6,:) = single(bc(validInds)') / 255;
                else
                    voxels(4:6,:) = repmat(single(leftIm(validInds)')*(1/255), [3,1]);
                end
                
                if pauseForEachFrame
                    secondsToWait = 0;
                else
                    secondsToWait = 0.5;
                end
                
                mexShowVoxels(voxels, secondsToWait);
            end
            
            if saveOutputToFile
                imwrite(disparityToShowBoth, [outputFilenamePrefix, sprintf('stereoBm_%d.png', whichImageNumbers(iImageNumber-1))]);
                imwrite(colorDisparityToShowBoth, [outputFilenamePrefix, sprintf('stereoBmColor_%d.png', whichImageNumbers(iImageNumber-1))]);
            end
        end % if runStereoBmComputation && length(images) == 2
        
        if pauseForEachFrame && ~drawVoxels
            [~,~,c] = ginput(1);
            if c == 'q'
                disp('Quitting...');
                return
            end
        else
            pause(0.03);
        end
        
        %         c = getkeywait(0.03);
        %         if c == 'q'
        %             disp('Quitting...');
        %             return
        %         end
        
        % Now update the previous frame
        lastImages = images;
    end % while true
end % function simpleLkTest2()

function [points1, status, err, goodPoints0, goodPoints1] = computeFlow(image0, image1, points0, windowSize, lk_params, filterFlowWithErrThreshold, maxErr, filterFlowWithMinimanessThreshold, offsets, onlyHorizontalFlow)
    
    halfWindowSize = floor(windowSize/2);
    
    diffs = Inf*ones(length(points0), 1);
    for iOffset = 1:length(offsets)
        offsetPoints0 = cell(size(points0));
        for iPoint = 1:length(offsetPoints0)
            offsetPoints0{iPoint} = points0{iPoint} + offsets{iOffset};
        end
        
        [curPoints1, curStatus, curErr] = cv.calcOpticalFlowPyrLK(...
            image0, image1, offsetPoints0,...
            'WinSize', windowSize,...
            'MaxLevel', lk_params.maxLevel,...
            'Criteria', lk_params.criteria,...
            'MinEigThreshold', lk_params.minEigThreshold,...
            'OnlyUpdateHorizontal', onlyHorizontalFlow);
        
        if iOffset == 1
            points1 = curPoints1;
            status = zeros(size(curStatus));
            err = curErr;
        end
        
        for iPoint = 1:length(curPoints1)
            if ~curStatus(iPoint)
                continue;
            end
            
            x0Range = round([points0{iPoint}(1)-halfWindowSize(1), points0{iPoint}(1)+halfWindowSize(1)] + 1);
            y0Range = round([points0{iPoint}(2)-halfWindowSize(2), points0{iPoint}(2)+halfWindowSize(2)] + 1);
            
            x1Range = round([curPoints1{iPoint}(1)-halfWindowSize(1), curPoints1{iPoint}(1)+halfWindowSize(1)] + 1);
            y1Range = round([curPoints1{iPoint}(2)-halfWindowSize(2), curPoints1{iPoint}(2)+halfWindowSize(2)] + 1);
            
            if min(x0Range) < 1 || max(x0Range) > size(image0,2) || min(y0Range) < 1 || max(y0Range) > size(image0,1) ||...
                    min(x1Range) < 1 || max(x1Range) > size(image1,2) || min(y1Range) < 1 || max(y1Range) > size(image1,1)
                
                continue;
            end
            
            block0 = image0(y0Range(1):y0Range(2), x0Range(1):x0Range(2));
            block1 = image1(y1Range(1):y1Range(2), x1Range(1):x1Range(2));
            
            block0 = block0(:);
            block1 = block1(:);
            
            diff = mean(abs(block0-block1));
            
            if diff < diffs(iPoint)
                diffs(iPoint) = diff;
                points1{iPoint} = curPoints1{iPoint};
                status(iPoint) = curStatus(iPoint);
                err(iPoint) = curErr(iPoint);
            end
        end
        
        %             inds = curStatus == 1;
        %
        %             for ind
        %             & curErr < err;
        %             points1(inds) = curPoints1(inds);
        %             status(inds) = curStatus(inds);
        %             err(inds) = curErr(inds);
        
        %         keyboard
    end
    
    if filterFlowWithErrThreshold
        status(:) = 1;
        status(err > maxErr) = 0;
    end % if filterFlowWithErrThreshold
    
    if filterFlowWithMinimanessThreshold
        curLocalMinimaness = computeLocalMinimaness(curImage, mask, windowSize);
        lastLocalMinimaness = computeLocalMinimaness(lastImage, mask, windowSize);
        
        status(:) = 1;
        for iPoint = 1:length(points0)
            if status(iPoint)
                p0 = [round(points0{iPoint}(2))+1, round(points0{iPoint}(1))+1];
                p1 = [round(points1{iPoint}(2))+1, round(points1{iPoint}(1))+1];
                
                if p1(1) < 1 || p1(1) > size(image0,1) || p1(2) < 1 || p1(2) > size(image0,2)
                    status(iPoint) = 0;
                    continue
                end
                
                if lastLocalMinimaness(p0(1), p0(2)) < minimanessThreshold
                    status(iPoint) = 0;
                    continue
                end
                
                if curLocalMinimaness(p1(1), p1(2)) < minimanessThreshold
                    status(iPoint) = 0;
                    continue
                end
            end % if status(iPoint)
        end
    end % if filterFlowWithMinimanessThreshold
    
    % Select good points
    goodPoints0 = points0(status==1);
    goodPoints1 = points1(status==1);
end % function computeFlow()
