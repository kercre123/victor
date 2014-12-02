% function simpleLkTest2(varargin)

% simpleLkTest2('useLiveFeed', true);
% simpleLkTest2('useLiveFeed', false, 'runFlowComputation', false, 'filenamePattern','/Users/pbarnum/Documents/datasets/stereo/stereo_%sRectified_%d.png', 'whichImageNumbers', 0:3090, 'pauseForEachFrame', true);

function simpleLkTest2(varargin)
    
    disp('Starting...');
    
    useLiveFeed = true;
    cameraIds = [1,0];
    
    filenamePattern = '/Users/pbarnum/Documents/datasets/stereo/stereo_%sRectified_%d.png';
    whichImageNumbers = 0:3090;
    pauseForEachFrame = false;
    
    useStereoCalibration = false;
    computeStereoBm = false;
    
    runFlowComputation = false;
    runStereoComputation = true;
    
    onlyHorizontalStereoFlow = true;
    
    saveOutputToFile = false;
    outputFilenamePrefix = '/Users/pbarnum/Documents/tmp/output/lkOutput2_';
    
    leftRightCheck = true;
    leftRightCheckThreshold = 7; % TODO: pick a good threshold
    
    stereoDisparityColors128 = [0,0,255;0,7,255;0,16,255;0,24,255;0,32,255;0,41,255;0,49,255;0,57,255;0,65,255;0,73,255;0,82,255;0,90,255;0,99,255;0,107,255;0,115,255;0,124,255;0,132,255;0,140,255;0,148,255;0,156,255;0,165,255;0,173,255;0,182,255;0,190,255;0,198,255;0,207,255;0,215,255;0,223,255;0,231,255;0,239,255;0,248,255;0,255,255;0,255,255;0,255,248;0,255,239;0,255,231;0,255,223;0,255,215;0,255,207;0,255,198;0,255,190;0,255,182;0,255,173;0,255,165;0,255,156;0,255,148;0,255,140;0,255,132;0,255,124;0,255,115;0,255,107;0,255,99;0,255,90;0,255,82;0,255,73;0,255,65;0,255,57;0,255,49;0,255,41;0,255,32;0,255,24;0,255,16;0,255,7;0,255,0;0,255,0;7,255,0;16,255,0;24,255,0;32,255,0;41,255,0;49,255,0;57,255,0;65,255,0;73,255,0;82,255,0;90,255,0;99,255,0;107,255,0;115,255,0;124,255,0;132,255,0;140,255,0;148,255,0;156,255,0;165,255,0;173,255,0;182,255,0;190,255,0;198,255,0;207,255,0;215,255,0;223,255,0;231,255,0;239,255,0;248,255,0;255,255,0;255,255,0;255,248,0;255,239,0;255,231,0;255,223,0;255,215,0;255,207,0;255,198,0;255,190,0;255,182,0;255,173,0;255,165,0;255,156,0;255,148,0;255,140,0;255,132,0;255,124,0;255,115,0;255,107,0;255,99,0;255,90,0;255,82,0;255,73,0;255,65,0;255,57,0;255,49,0;255,41,0;255,32,0;255,24,0;255,16,0;255,7,0;255,0,0];
    stereoDisparityColors96 = [0,1,255;0,12,255;0,22,255;0,34,255;0,45,255;0,56,255;0,67,255;0,78,255;0,89,255;0,100,255;0,111,255;0,122,255;0,133,255;0,144,255;0,155,255;0,166,255;0,177,255;0,188,255;0,199,255;0,211,255;0,221,255;0,233,255;0,243,255;0,254,255;0,255,254;0,255,243;0,255,233;0,255,221;0,255,211;0,255,199;0,255,188;0,255,177;0,255,166;0,255,155;0,255,144;0,255,133;0,255,122;0,255,111;0,255,100;0,255,89;0,255,78;0,255,67;0,255,56;0,255,45;0,255,34;0,255,22;0,255,12;0,255,1;1,255,0;12,255,0;22,255,0;34,255,0;45,255,0;56,255,0;67,255,0;78,255,0;89,255,0;100,255,0;111,255,0;122,255,0;133,255,0;144,255,0;155,255,0;166,255,0;177,255,0;188,255,0;199,255,0;211,255,0;221,255,0;233,255,0;243,255,0;254,255,0;255,254,0;255,243,0;255,233,0;255,221,0;255,211,0;255,199,0;255,188,0;255,177,0;255,166,0;255,155,0;255,144,0;255,133,0;255,122,0;255,111,0;255,100,0;255,89,0;255,78,0;255,67,0;255,56,0;255,45,0;255,34,0;255,22,0;255,12,0;255,1,0];
    stereoDisparityColors64 = [0,4,255;0,20,255;0,36,255;0,53,255;0,70,255;0,86,255;0,103,255;0,119,255;0,136,255;0,152,255;0,169,255;0,185,255;0,202,255;0,220,255;0,235,255;0,251,255;0,255,251;0,255,235;0,255,220;0,255,202;0,255,185;0,255,169;0,255,152;0,255,136;0,255,119;0,255,103;0,255,86;0,255,70;0,255,53;0,255,36;0,255,20;0,255,4;4,255,0;20,255,0;36,255,0;53,255,0;70,255,0;86,255,0;103,255,0;119,255,0;136,255,0;152,255,0;169,255,0;185,255,0;202,255,0;220,255,0;235,255,0;251,255,0;255,251,0;255,235,0;255,220,0;255,202,0;255,185,0;255,169,0;255,152,0;255,136,0;255,119,0;255,103,0;255,86,0;255,70,0;255,53,0;255,36,0;255,20,0;255,4,0];
    
    %windowSizes = {[15,15], [31,31], [51,51]}
    %windowSizes = {[31,31]}
    windowSizes = {[51,51]};
    
    numPointsPerDimension = 50;
    
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
        'maxLevel', 4,...
        'criteria', struct('type', 'Count+EPS', 'maxCount', 10, 'epsilon', 0.03),...
        'minEigThreshold', 1e-4);
    
    stereo_lk_maxAngle = pi/16;
    
    parseVarargin(varargin{:});
    
    if length(cameraIds) == 1
        useStereoCalibration = false;
    end
    
    undistortMaps = [];
    if useStereoCalibration
        [stereoProcessor, undistortMaps, masks] = initStereo(128, 'bm');
    else
        masks = {ones(480,640), ones(480,640)};
    end
    
    iImageNumber = 1;
    
    if useLiveFeed
        videoCaptures = cell(length(cameraIds), 1);
        for i = 1:length(cameraIds)
            videoCaptures{i} = cv.VideoCapture(cameraIds(i));
            disp(sprintf('Opened camera %d (id=%d)', i, cameraIds(i)))
        end
        
        images = captureImages(videoCaptures, undistortMaps);
    else
        
        leftFilename = sprintf(filenamePattern, 'left', whichImageNumbers(iImageNumber));
        rightFilename = sprintf(filenamePattern, 'right', whichImageNumbers(iImageNumber));
        images = {rgb2gray2(imread(leftFilename)), rgb2gray2(imread(rightFilename))};
    end
    
    lastImages = images;
    
    for i = 2:length(images)
        assert(min(size(images{1}) == size(images{i})));
    end
    
    imageSize = size(images{1});
    
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
        else
            leftFilename = sprintf(filenamePattern, 'left', whichImageNumbers(iImageNumber));
            rightFilename = sprintf(filenamePattern, 'right', whichImageNumbers(iImageNumber));
            images = {rgb2gray2(imread(leftFilename)), rgb2gray2(imread(rightFilename))};
            disp(sprintf('Image %d) %s', iImageNumber, leftFilename))
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
                    
                    [points1, status, err, goodPoints0, goodPoints1] = computeFlow(lastImage, curImage, points0, windowSizes{iWindowSize}, flow_lk_params, filterFlowWithErrThreshold, maxErr, filterFlowWithMinimanessThreshold);
                    
                    if leftRightCheck
                        [points0B, statusB, errB, ~, ~] = computeFlow(curImage, lastImage, points1, windowSizes{iWindowSize}, flow_lk_params, filterFlowWithErrThreshold, maxErr, filterFlowWithMinimanessThreshold);
                        
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
        if runStereoComputation
            stereoImagesToShow = {};
            if length(images) == 2
                % Compute the stereo
                allStereoPoints = {};
                for iWindowSize = 1:length(windowSizes)
                    [points1, status, err, goodPoints0, goodPoints1] = computeFlow(images{1}, images{2}, points0, windowSizes{iWindowSize}, stereo_lk_params, false, 0, false, onlyHorizontalStereoFlow);
                    
                    if leftRightCheck
                        [points0B, statusB, errB, ~, ~] = computeFlow(images{2}, images{1}, points1, windowSizes{iWindowSize}, stereo_lk_params, false, 0, false, onlyHorizontalStereoFlow);
                        
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
                for iPoint = 1:length(allStereoPoints)
                    pointsToShow = allStereoPoints{iPoint};
                    keypointImage = drawKeypointMatches(pointsToShow.curImage, pointsToShow.goodPoints0, pointsToShow.goodPoints1, stereoDisparityColors96);
                    stereoImagesToShow{end+1} = keypointImage; %#ok<AGROW>
                    
                    if leftRightCheck
                        keypointImage = drawKeypointMatches(pointsToShow.curImage, pointsToShow.goodPoints0B, pointsToShow.goodPoints1B, stereoDisparityColors96);
                        stereoImagesToShow{end+1} = keypointImage; %#ok<AGROW>
                    end
                end % for iFlow = 1:length(allFlowPoints)
            end % if length(images) == 2
            
            imshows(stereoImagesToShow, 2);
            
            if saveOutputToFile
                outImage = export_fig();                
                imwrite(outImage, [outputFilenamePrefix, sprintf('stereo_%d.png', whichImageNumbers(iImageNumber-1))]);                
            end
        end % if runStereoComputation
        
        if pauseForEachFrame
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

function [points1, status, err, goodPoints0, goodPoints1] = computeFlow(image0, image1, points0, windowSize, lk_params, filterFlowWithErrThreshold, maxErr, filterFlowWithMinimanessThreshold, onlyHorizontalFlow)
    [points1, status, err] = cv.calcOpticalFlowPyrLK(...
        image0, image1, points0,...
        'WinSize', windowSize,...
        'MaxLevel', lk_params.maxLevel,...
        'Criteria', lk_params.criteria,...
        'MinEigThreshold', lk_params.minEigThreshold,...
        'OnlyUpdateHorizontal', onlyHorizontalFlow);
        
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
