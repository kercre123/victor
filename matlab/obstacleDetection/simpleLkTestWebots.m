% function simpleLkTestWebots(varargin)

% simpleLkTestWebots();

function simpleLkTestWebots(varargin)
    
    disp('Starting...');
    
    windowSize = [15,15];
    
    imageSize = [240, 320];
    
    numPointsPerDimension = 25;
    
    pauseForEachFrame = false;
    
    % Parameters for lucas kanade optical flow
    flow_lk_params = struct(...
        'winSize', [31,31],...
        'maxLevel', 4,...
        'criteria', struct('type', 'Count+EPS', 'maxCount', 10, 'epsilon', 0.03),...
        'minEigThreshold', 5e-3);
    
    stereoDisparityColors196 = [0,0,0;0,3,255;0,9,255;0,14,255;0,19,255;0,24,255;0,30,255;0,36,255;0,41,255;0,46,255;0,52,255;0,57,255;0,63,255;0,68,255;0,73,255;0,78,255;0,84,255;0,90,255;0,95,255;0,100,255;0,106,255;0,111,255;0,117,255;0,122,255;0,128,255;0,133,255;0,138,255;0,144,255;0,149,255;0,155,255;0,160,255;0,165,255;0,171,255;0,177,255;0,182,255;0,187,255;0,192,255;0,198,255;0,204,255;0,209,255;0,215,255;0,220,255;0,225,255;0,231,255;0,236,255;0,241,255;0,246,255;0,252,255;0,255,255;0,255,255;0,255,252;0,255,246;0,255,241;0,255,236;0,255,231;0,255,225;0,255,220;0,255,215;0,255,209;0,255,204;0,255,198;0,255,192;0,255,187;0,255,182;0,255,177;0,255,171;0,255,165;0,255,160;0,255,155;0,255,149;0,255,144;0,255,138;0,255,133;0,255,128;0,255,122;0,255,117;0,255,111;0,255,106;0,255,100;0,255,95;0,255,90;0,255,84;0,255,78;0,255,73;0,255,68;0,255,63;0,255,57;0,255,52;0,255,46;0,255,41;0,255,36;0,255,30;0,255,24;0,255,19;0,255,14;0,255,9;0,255,3;0,255,0;0,255,0;3,255,0;9,255,0;14,255,0;19,255,0;24,255,0;30,255,0;36,255,0;41,255,0;46,255,0;52,255,0;57,255,0;63,255,0;68,255,0;73,255,0;78,255,0;84,255,0;90,255,0;95,255,0;100,255,0;106,255,0;111,255,0;117,255,0;122,255,0;128,255,0;133,255,0;138,255,0;144,255,0;149,255,0;155,255,0;160,255,0;165,255,0;171,255,0;177,255,0;182,255,0;187,255,0;192,255,0;198,255,0;204,255,0;209,255,0;215,255,0;220,255,0;225,255,0;231,255,0;236,255,0;241,255,0;246,255,0;252,255,0;255,255,0;255,255,0;255,252,0;255,246,0;255,241,0;255,236,0;255,231,0;255,225,0;255,220,0;255,215,0;255,209,0;255,204,0;255,198,0;255,192,0;255,187,0;255,182,0;255,177,0;255,171,0;255,165,0;255,160,0;255,155,0;255,149,0;255,144,0;255,138,0;255,133,0;255,128,0;255,122,0;255,117,0;255,111,0;255,106,0;255,100,0;255,95,0;255,90,0;255,84,0;255,78,0;255,73,0;255,68,0;255,63,0;255,57,0;255,52,0;255,46,0;255,41,0;255,36,0;255,30,0;255,24,0;255,19,0;255,14,0;255,9,0;255,3,0;255,0,0];
    stereoDisparityColors128 = [0,0,0;0,7,255;0,16,255;0,24,255;0,32,255;0,41,255;0,49,255;0,57,255;0,65,255;0,73,255;0,82,255;0,90,255;0,99,255;0,107,255;0,115,255;0,124,255;0,132,255;0,140,255;0,148,255;0,156,255;0,165,255;0,173,255;0,182,255;0,190,255;0,198,255;0,207,255;0,215,255;0,223,255;0,231,255;0,239,255;0,248,255;0,255,255;0,255,255;0,255,248;0,255,239;0,255,231;0,255,223;0,255,215;0,255,207;0,255,198;0,255,190;0,255,182;0,255,173;0,255,165;0,255,156;0,255,148;0,255,140;0,255,132;0,255,124;0,255,115;0,255,107;0,255,99;0,255,90;0,255,82;0,255,73;0,255,65;0,255,57;0,255,49;0,255,41;0,255,32;0,255,24;0,255,16;0,255,7;0,255,0;0,255,0;7,255,0;16,255,0;24,255,0;32,255,0;41,255,0;49,255,0;57,255,0;65,255,0;73,255,0;82,255,0;90,255,0;99,255,0;107,255,0;115,255,0;124,255,0;132,255,0;140,255,0;148,255,0;156,255,0;165,255,0;173,255,0;182,255,0;190,255,0;198,255,0;207,255,0;215,255,0;223,255,0;231,255,0;239,255,0;248,255,0;255,255,0;255,255,0;255,248,0;255,239,0;255,231,0;255,223,0;255,215,0;255,207,0;255,198,0;255,190,0;255,182,0;255,173,0;255,165,0;255,156,0;255,148,0;255,140,0;255,132,0;255,124,0;255,115,0;255,107,0;255,99,0;255,90,0;255,82,0;255,73,0;255,65,0;255,57,0;255,49,0;255,41,0;255,32,0;255,24,0;255,16,0;255,7,0;255,0,0];
    stereoDisparityColors96 = [0,0,0;0,12,255;0,22,255;0,34,255;0,45,255;0,56,255;0,67,255;0,78,255;0,89,255;0,100,255;0,111,255;0,122,255;0,133,255;0,144,255;0,155,255;0,166,255;0,177,255;0,188,255;0,199,255;0,211,255;0,221,255;0,233,255;0,243,255;0,254,255;0,255,254;0,255,243;0,255,233;0,255,221;0,255,211;0,255,199;0,255,188;0,255,177;0,255,166;0,255,155;0,255,144;0,255,133;0,255,122;0,255,111;0,255,100;0,255,89;0,255,78;0,255,67;0,255,56;0,255,45;0,255,34;0,255,22;0,255,12;0,255,1;1,255,0;12,255,0;22,255,0;34,255,0;45,255,0;56,255,0;67,255,0;78,255,0;89,255,0;100,255,0;111,255,0;122,255,0;133,255,0;144,255,0;155,255,0;166,255,0;177,255,0;188,255,0;199,255,0;211,255,0;221,255,0;233,255,0;243,255,0;254,255,0;255,254,0;255,243,0;255,233,0;255,221,0;255,211,0;255,199,0;255,188,0;255,177,0;255,166,0;255,155,0;255,144,0;255,133,0;255,122,0;255,111,0;255,100,0;255,89,0;255,78,0;255,67,0;255,56,0;255,45,0;255,34,0;255,22,0;255,12,0;255,1,0];
    stereoDisparityColors64 = [0,0,0;0,20,255;0,36,255;0,53,255;0,70,255;0,86,255;0,103,255;0,119,255;0,136,255;0,152,255;0,169,255;0,185,255;0,202,255;0,220,255;0,235,255;0,251,255;0,255,251;0,255,235;0,255,220;0,255,202;0,255,185;0,255,169;0,255,152;0,255,136;0,255,119;0,255,103;0,255,86;0,255,70;0,255,53;0,255,36;0,255,20;0,255,4;4,255,0;20,255,0;36,255,0;53,255,0;70,255,0;86,255,0;103,255,0;119,255,0;136,255,0;152,255,0;169,255,0;185,255,0;202,255,0;220,255,0;235,255,0;251,255,0;255,251,0;255,235,0;255,220,0;255,202,0;255,185,0;255,169,0;255,152,0;255,136,0;255,119,0;255,103,0;255,86,0;255,70,0;255,53,0;255,36,0;255,20,0;255,4,0];
    stereoDisparityColors32 = [0,0,0;0,45,255;0,78,255;0,111,255;0,144,255;0,177,255;0,210,255;0,242,255;0,255,242;0,255,210;0,255,177;0,255,144;0,255,111;0,255,78;0,255,45;0,255,13;13,255,0;45,255,0;78,255,0;111,255,0;144,255,0;177,255,0;210,255,0;242,255,0;255,242,0;255,210,0;255,177,0;255,144,0;255,111,0;255,78,0;255,45,0;255,13,0];
    stereoDisparityColors16 = [0,0,0;0,94,255;0,161,255;0,225,254;0,254,225;0,255,161;0,255,94;1,255,30;30,255,1;94,255,0;161,255,0;225,254,0;254,225,0;255,161,0;255,94,0;255,30,0];
    
    parseVarargin(varargin{:});
    
    [points0x, points0y] = meshgrid(linspace(25, imageSize(2)-26, numPointsPerDimension),linspace(25, imageSize(1)-26, numPointsPerDimension));
    points0Raw = [points0x(:), points0y(:)];
    points0 = cell(1, size(points0Raw,1));
    for i = 1:size(points0Raw,1)
        points0{i} = points0Raw(i,:);
    end
    
    lastImage = webotsCameraCapture();
    while true       
        image = webotsCameraCapture();
        
        [points1, status, err, goodPoints0, goodPoints1] =...
            computeFlow(lastImage, image, points0, windowSize, flow_lk_params, false, Inf, false, {[0,0]}, false);
        
        flowPoints = struct(...
            'points1', {points1},...
            'status', {status},...
            'err', {err},...
            'goodPoints0', {goodPoints0},...
            'goodPoints1', {goodPoints1},...
            'image', {image},...
            'lastImage', {lastImage},...
            'windowSize', windowSize);
        
        % Draw the flow       
        toShow = drawKeypointMatches(flowPoints.image, flowPoints.goodPoints0, flowPoints.goodPoints1, stereoDisparityColors128);
        
        imshows(toShow, 1);
        
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
        lastImage = image;
    end % while true
end % function simpleLkTestWebots()

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
        
    % Select good points
    goodPoints0 = points0(status==1);
    goodPoints1 = points1(status==1);
end % function computeFlow()
