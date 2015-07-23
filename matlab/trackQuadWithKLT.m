
% quad2 = trackQuadWithKLT(image1, quad1, image2);

function quad2 = trackQuadWithKLT(image1, quad1, image2, varargin)
    
    assert(min(size(quad1) == [4,2]) == 1)
    
    numPointsPerDimension = 10;
    
    xRange = [max(1,min(quad1(:,1))), min(size(image1,2), max(quad1(:,1)))];
    yRange = [max(1,min(quad1(:,2))), min(size(image1,1), max(quad1(:,2)))];
    
    quadLongWidth = max([abs(xRange(2)-xRange(1)), abs(yRange(2)-yRange(1))]);
    
    windowSize = max(3, int32(round(quadLongWidth * 2 / numPointsPerDimension)));
    
    flow_lk_params = struct(...
        'winSize', [windowSize,windowSize],...
        'maxLevel', 4,...
        'criteria', struct('type', 'Count+EPS', 'maxCount', 10, 'epsilon', 0.03),...
        'minEigThreshold', 5e-3);
    
    parseVarargin(varargin{:});
    
    [points0x, points0y] = meshgrid(linspace(xRange(1), xRange(2), numPointsPerDimension), linspace(yRange(1), yRange(2), numPointsPerDimension));
    points0Raw = [points0x(:), points0y(:)];
    points0 = cell(1, size(points0Raw,1));
    for i = 1:size(points0Raw,1)
        points0{i} = points0Raw(i,:);
    end
    
    [points1, curStatus, curErr] = cv.calcOpticalFlowPyrLK(...
        image1, image2, points0,...
        'WinSize', flow_lk_params.winSize,...
        'MaxLevel', flow_lk_params.maxLevel,...
        'Criteria', flow_lk_params.criteria,...
        'MinEigThreshold', flow_lk_params.minEigThreshold,...
        'OnlyUpdateHorizontal', false);
    
    numValidPoints = length(find(curStatus == 1));
    
    points0Good = points0(curStatus == 1);
    points1Good = points1(curStatus == 1);

    if numValidPoints >= 4
    
        homography = cv.findHomography(points0Good, points1Good, 'Method', 'Ransac');

        quad1h = [quad1,ones(4,1)];    
        quad2 = homography * quad1h';
        quad2 = quad2(1:2,:) ./ repmat(quad2(3,:), [2,1]);

        quad2 = quad2';
    else
        quad2 = quad1;
    end
    
%     keyboard
    
%     points0 = []
%     for y in np.linspace(yRange[0], yRange[1], numPointsPerDimension):
%         for x in np.linspace(xRange[0], xRange[1], numPointsPerDimension):
%             points0.append((x,y))
% 
%             
%             
%     points0 = np.array(points0).reshape(-1,1,2).astype('float32')
% 
%     points1, st, err = cv2.calcOpticalFlowPyrLK(startImage, endImage, points0, None, **flow_lk_params)