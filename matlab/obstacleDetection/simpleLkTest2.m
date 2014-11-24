% function simpleLkTest2()

function simpleLkTest2()
    
    disp('Starting...');
    
    %cameraIds = [1];
    cameraIds = [1,2];
    
    useStereoCalibration = true;
    undistortMaps = [];
    computeStereoBm = false;
    
    stereoDisparityColors128 = [255, 0, 0; 255, 7, 0; 255, 16, 0; 255, 24, 0; 255, 32, 0; 255, 41, 0; 255, 49, 0; 255, 57, 0; 255, 65, 0; 255, 73, 0; 255, 82, 0; 255, 90, 0; 255, 99, 0; 255, 107, 0; 255, 115, 0; 255, 124, 0; 255, 132, 0; 255, 140, 0; 255, 148, 0; 255, 156, 0; 255, 165, 0; 255, 173, 0; 255, 182, 0; 255, 190, 0; 255, 198, 0; 255, 207, 0; 255, 215, 0; 255, 223, 0; 255, 231, 0; 255, 239, 0; 255, 248, 0; 255, 255, 0; 255, 255, 0; 248, 255, 0; 239, 255, 0; 231, 255, 0; 223, 255, 0; 215, 255, 0; 207, 255, 0; 198, 255, 0; 190, 255, 0; 182, 255, 0; 173, 255, 0; 165, 255, 0; 156, 255, 0; 148, 255, 0; 140, 255, 0; 132, 255, 0; 124, 255, 0; 115, 255, 0; 107, 255, 0; 99, 255, 0; 90, 255, 0; 82, 255, 0; 73, 255, 0; 65, 255, 0; 57, 255, 0; 49, 255, 0; 41, 255, 0; 32, 255, 0; 24, 255, 0; 16, 255, 0; 7, 255, 0; 0, 255, 0; 0, 255, 0; 0, 255, 7; 0, 255, 16; 0, 255, 24; 0, 255, 32; 0, 255, 41; 0, 255, 49; 0, 255, 57; 0, 255, 65; 0, 255, 73; 0, 255, 82; 0, 255, 90; 0, 255, 99; 0, 255, 107; 0, 255, 115; 0, 255, 124; 0, 255, 132; 0, 255, 140; 0, 255, 148; 0, 255, 156; 0, 255, 165; 0, 255, 173; 0, 255, 182; 0, 255, 190; 0, 255, 198; 0, 255, 207; 0, 255, 215; 0, 255, 223; 0, 255, 231; 0, 255, 239; 0, 255, 248; 0, 255, 255; 0, 255, 255; 0, 248, 255; 0, 239, 255; 0, 231, 255; 0, 223, 255; 0, 215, 255; 0, 207, 255; 0, 198, 255; 0, 190, 255; 0, 182, 255; 0, 173, 255; 0, 165, 255; 0, 156, 255; 0, 148, 255; 0, 140, 255; 0, 132, 255; 0, 124, 255; 0, 115, 255; 0, 107, 255; 0, 99, 255; 0, 90, 255; 0, 82, 255; 0, 73, 255; 0, 65, 255; 0, 57, 255; 0, 49, 255; 0, 41, 255; 0, 32, 255; 0, 24, 255; 0, 16, 255; 0, 7, 255; 0, 0, 255];
    stereoDisparityColors64 = [255, 4, 0; 255, 20, 0; 255, 36, 0; 255, 53, 0; 255, 70, 0; 255, 86, 0; 255, 103, 0; 255, 119, 0; 255, 136, 0; 255, 152, 0; 255, 169, 0; 255, 185, 0; 255, 202, 0; 255, 220, 0; 255, 235, 0; 255, 251, 0; 251, 255, 0; 235, 255, 0; 220, 255, 0; 202, 255, 0; 185, 255, 0; 169, 255, 0; 152, 255, 0; 136, 255, 0; 119, 255, 0; 103, 255, 0; 86, 255, 0; 70, 255, 0; 53, 255, 0; 36, 255, 0; 20, 255, 0; 4, 255, 0; 0, 255, 4; 0, 255, 20; 0, 255, 36; 0, 255, 53; 0, 255, 70; 0, 255, 86; 0, 255, 103; 0, 255, 119; 0, 255, 136; 0, 255, 152; 0, 255, 169; 0, 255, 185; 0, 255, 202; 0, 255, 220; 0, 255, 235; 0, 255, 251; 0, 251, 255; 0, 235, 255; 0, 220, 255; 0, 202, 255; 0, 185, 255; 0, 169, 255; 0, 152, 255; 0, 136, 255; 0, 119, 255; 0, 103, 255; 0, 86, 255; 0, 70, 255; 0, 53, 255; 0, 36, 255; 0, 20, 255; 0, 4, 255];
    
    %windowSizes = {[15,15], [31,31], [51,51]}
    %windowSizes = {[31,31]}
    windowSizes = {[51,51]};
    
    numPointsPerDimension = 50;
    
    % Parameters for lucas kanade optical flow
    flow_lk_params = struct(...
        'winSize', [31,31],...
        'maxLevel', 4,...
        'criteria', struct('type', 'Count+EPS', 'maxCount', 10, 'epsilon', 0.03),...
        'minEigThreshold', 5e-3);
    
    stereo_lk_params = struct(...
        'winSize' , [31,31],...
        'maxLevel', 5,...
        'criteria', struct('type', 'Count+EPS', 'maxCount', 10, 'epsilon', 0.03),...
        'minEigThreshold', 1e-4);
    
    stereo_lk_maxAngle = pi/16;
    
    if length(cameraIds) == 1
        useStereoCalibration = false;
    end
    
    undistortMaps = [];
    if useStereoCalibration
        [stereoProcessor, undistortMaps] = initStereo(128, 'bm');
    end
    
    videoCaptures = cell(length(cameraIds), 1);
    for i = 1:length(cameraIds)
        videoCaptures{i} = cv.VideoCapture(cameraIds(i));
        disp(sprintf('Opened camera %d (id=%d)', i, cameraIds(i)))
    end
    
    images = captureImages(videoCaptures, undistortMaps);
    lastImages = images;
    
    for i = 2:length(images)
        assert(min(size(images{1}) == size(images{i})));
    end
    
    imageSize = size(images{1});
    
    %[allPoints0x, allPoints0y] = meshgrid(0:(imageSize(2)-1), 0:(imageSize(1)-1));
    [points0x, points0y] = meshgrid(linspace(0, imageSize(2)-1, numPointsPerDimension),linspace(0, imageSize(1)-1, numPointsPerDimension));
    points0Raw = [points0x(:), points0y(:)];
    points0 = cell(1, size(points0Raw,1));
    for i = 1:size(points0Raw,1)
        points0{i} = points0Raw(i,:);
    end
    
    while true
        images = captureImages(videoCaptures, undistortMaps);
        
        % Compute the flow
        allFlowPoints = {};
        for iImage = 1:length(images)
            curImage = images{iImage};
            lastImage = lastImages{iImage};
            
            curFlowPoints = {};
            
            for iWindowSize = 1:length(windowSizes)
                [points1, status, err] = cv.calcOpticalFlowPyrLK(...
                    lastImage, curImage, points0,...
                    'WinSize', windowSizes{iWindowSize},...
                    'MaxLevel', flow_lk_params.maxLevel,...
                    'Criteria', flow_lk_params.criteria,...
                    'MinEigThreshold', flow_lk_params.minEigThreshold);
                
                % Select good points
                goodPoints0 = points0(status==1);
                goodPoints1 = points1(status==1);
                
                curFlowPoints{end+1} = struct(...
                    'points1', {points1},...
                    'status', {status},...
                    'err', {err},...
                    'goodPoints0', {goodPoints0},...
                    'goodPoints1', {goodPoints1},...
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
            end % for jFlow in allFlowPoints{iFlow}
        end % for iFlow = 1:length(allFlowPoints)
        
        % Stereo
        stereoImagesToShow = {};
        if length(images) == 2
            % Compute the stereo
            allStereoPoints = {};
            for iWindowSize = 1:length(windowSizes)
                [points1, status, err] = cv.calcOpticalFlowPyrLK(...
                    images{1}, images{2}, points0,...
                    'WinSize', windowSizes{iWindowSize},...
                    'MaxLevel', flow_lk_params.maxLevel,...
                    'Criteria', flow_lk_params.criteria,...
                    'MinEigThreshold', flow_lk_params.minEigThreshold);
                
                % Select good points
                goodPoints0 = points0(status==1);
                goodPoints1 = points1(status==1);
                
                allStereoPoints{end+1} = struct(...
                    'points1', {points1},...
                    'status', {status},...
                    'err', {err},...
                    'goodPoints0', {goodPoints0},...
                    'goodPoints1', {goodPoints1},...
                    'curImage', {curImage},...
                    'lastImage', {lastImage},...
                    'windowSize', windowSizes(iWindowSize),...
                    'iImage', iImage); %#ok<AGROW>
            end % for iWindowSize = 1:length(windowSizes)
            
            % Draw the stereo matches
            for iPoint = 1:length(allStereoPoints)
                pointsToShow = allStereoPoints{iPoint};
                keypointImage = drawKeypointMatches(pointsToShow.curImage, pointsToShow.goodPoints0, pointsToShow.goodPoints1, stereoDisparityColors128);
                stereoImagesToShow{end+1} = keypointImage; %#ok<AGROW>
            end % for iFlow = 1:length(allFlowPoints)
        end % if length(images) == 2
        
        imshows(flowImagesToShow, stereoImagesToShow);
        
        pause(0.03);
%         c = getkeywait(0.03);        
%         if c == 'q'
%             disp('Quitting...');
%             return
%         end
        
        % Now update the previous frame
        lastImages = images;
        
        %         keypointImage = drawKeypointMatches(flowPoints['curImage'], flowPoints['goodPoints0'], flowPoints['goodPoints1'], stereoDisparityColors128)
        %         cv2.imshow('flow_' + str(flowPoints['iImage']) + '_' + str(flowPoints['windowSize']), keypointImage)
        
    end % while true    
end % function simpleLkTest2()

%function images = captureImages(videoCaptures, undistortMaps, rightImageWarpHomography)
function images = captureImages(videoCaptures, undistortMaps)
    images =  {};
    for i = 1:length(videoCaptures)
        image = videoCaptures{i}.read;
        image = rgb2gray2(image);
        
        if exist('undistortMaps', 'var') && ~isempty(undistortMaps)
            image = cv.remap(image, undistortMaps{i}.mapX, undistortMaps{i}.mapY);
            
            %             if (i == 1) && (rightImageWarpHomography is not None)
            %                 image = cv2.warpPerspective(image, rightImageWarpHomography, image.shape[::-1])
            %             end
        end
        
        images{end+1} = image; %#ok<AGROW>
    end % for i = 1:length(videoCaptures)
end % function images = captureImages()

% draw the tracks
function keypointImage = drawKeypointMatches(image, points0, points1, distanceColormap)
    
    assert(length(points0) == length(points1));
    
    if ~exist('distanceColormap', 'var')
        distanceColormap = [];
    end
    
    keypointImage = repmat(image,[1,1,3]);
    
    colors = cell(length(points0), 1);
    
    for iPoint = 1:length(points0)
        x0 = points0{iPoint}(1);
        y0 = points0{iPoint}(2);
        
        x1 = points1{iPoint}(1);
        y1 = points1{iPoint}(2);
        
        if isempty(distanceColormap)
            colors{iPoint} = [255,0,0];
        else
            dist = sqrt((x0-x1)^2 + (y0-y1)^2);
            dist = round(dist);
            dist = dist + 1; % 0 is actually index 1
            
            if dist > length(distanceColormap)
                dist = length(distanceColormap);
            end
            
            colors{iPoint} = distanceColormap(dist, :);
        end
    end % for iPoint = 1:length(points0)
    
    keypointImage = cv.line(keypointImage, points0, points1, 'Colors', colors, 'Thickness', 1);
end % function drawKeypointMatches(image, points0, points1, distanceColormap)

function [stereoProcessor, undistortMaps] = initStereo(numDisparities, disparityType)
    
    if strcmpi(disparityType, 'bm')
        disparityWindowSize = 51;
        
        % TODO: add window size
        stereoProcessor = cv.StereoBM('Preset', 'Basic', 'NDisparities', numDisparities);
    else
        assert(false);
    end
    
    % Calibration for the Spynet stereo pair
    
    distCoeffs1 = [0.190152, -0.472407, 0.004230, -0.006271, 0.000000];
    distCoeffs2 = [0.169615, -0.418723, 0.000641, -0.012438, 0.000000];
    
    cameraMatrix1 = ...
        [724.938597, 0.000000, 319.709242;
        0.000000, 722.297095, 291.756454;
        0.000000, 0.000000, 1.000000];
    
    cameraMatrix2 = ...
        [742.089046, 0.000000, 305.986048;
        0.000000, 739.837304, 248.962878;
        0.000000, 0.000000, 1.000000];
    
    R = [0.994009, -0.022323, 0.106996;
        0.019794, 0.999500, 0.024641;
        -0.107492, -0.022376, 0.993954];
    
    T = [-13.001413, -0.695831, 0.988237]';
    
    imageSize = [640, 480];
    
    rectifyStruct = cv.stereoRectify(cameraMatrix1, distCoeffs1, cameraMatrix2, distCoeffs2, imageSize, R, T);
    
    [leftUndistortMapX, leftUndistortMapY] = cv.initUndistortRectifyMap(cameraMatrix1, distCoeffs1, rectifyStruct.P1, imageSize, 'R', rectifyStruct.R1);
    
    [rightUndistortMapX, rightUndistortMapY] = cv.initUndistortRectifyMap(cameraMatrix2, distCoeffs2, rectifyStruct.P2, imageSize, 'R', rectifyStruct.R2);
    
    undistortMaps = cell(2,1);
    undistortMaps{1} = struct('mapX', leftUndistortMapX, 'mapY', leftUndistortMapY);
    undistortMaps{2} = struct('mapX', rightUndistortMapX, 'mapY', rightUndistortMapY);
end % function undistortMaps = computeUndistortMaps()