% function testGroundPlaneWarping()

% Test how accurate feature matching is, with:
% 1. Different amounts of ground plane warping
% 2. Different accuracy of ground warp

function testGroundPlaneWarping()
    
    maxMatchDistance = 2; % number of pixels
    
    testFeature = 'SIFT';
    
    image = rgb2gray2(imread('~/Documents/Anki/products-cozmo-large-files/peopleScanned640x480.png'));
    maxDimension = max(size(image));
    
    largeImage = 255 * ones(ceil(sqrt(2)*[maxDimension,maxDimension]), 'uint8');
    
    extraHeight2 = floor((size(largeImage,1) - size(image,1))/2);
    extraWidth2  = floor((size(largeImage,2) - size(image,2))/2);
    
    yRange = [(1+extraHeight2), (extraHeight2+size(image,1))];
    xRange = [(1+extraWidth2),  (extraWidth2+size(image,2))];
    
    largeImage(yRange(1):yRange(2), xRange(1):xRange(2)) = image;
    
    patchSize = 31;
    
    if strcmpi(testFeature, 'ORB')
        [baseKeypoints, baseDescriptors] = cv.ORB(largeImage,...
            'NFeatures', 500,...
            'ScaleFactor', 1.2,...
            'NLevels', 3,...
            'EdgeThreshold', patchSize,...
            'WTA_K', 2,...
            'PatchSize', patchSize);
        
        matcher = cv.DescriptorMatcher('BruteForce-L1');
        matcher.add(baseDescriptors);
        matcher.train();
    elseif strcmpi(testFeature, 'SIFT')
        [baseKeypoints, baseDescriptors] = cv.SIFT(largeImage);
        
        matcher = cv.DescriptorMatcher('FlannBased');
        matcher.add(baseDescriptors);
        matcher.train();
    else
        assert(false);
    end
    
    baseKeypointLocations = ones(3, length(baseKeypoints));
    for i = 1:length(baseKeypoints)
        baseKeypointLocations(1:2, i) = baseKeypoints(i).pt;
    end % for iKeypoint = 1:length(baseKeypoints)
    
    figure(1);
    hold off;
    imshows(largeImage, 1);
    hold on;
    scatter(baseKeypointLocations(1,:)+1, baseKeypointLocations(2,:)+1);
    
    rotations = linspace(0, 2*pi, 17);
    rotations = rotations(1:(end-1));
%     rotations = .9*pi;
    
    for iRotation = 1:length(rotations)
        theta = rotations(iRotation);
        
        rotation = [cos(theta), -sin(theta), 0;
            sin(theta), cos(theta), 0;
            0, 0, 1];
        
        translationF = [1, 0, -size(largeImage,2)/2;
            0, 1, -size(largeImage,1)/2;
            0, 0, 1];
        
        translationB = eye(3);
        translationB(1:2,3) = -translationF(1:2,3);
        
        homography = translationB * rotation * translationF;
        
        [warpedImage, ~, ~] = lucasKande_warpGroundTruth(largeImage, homography, size(largeImage));
        warpedImage = uint8(warpedImage);
        
        %         imshows({largeImage, uint8(warpedImage)});
        if strcmpi(testFeature, 'ORB')
            [curKeypoints, curDescriptors] = cv.ORB(warpedImage,...
                'NFeatures', 500,...
                'ScaleFactor', 1.2,...
                'NLevels', 3,...
                'EdgeThreshold', patchSize,...
                'WTA_K', 2,...
                'PatchSize', patchSize);
            
            matches = matcher.match(curDescriptors);
        elseif strcmpi(testFeature, 'SIFT')
            [curKeypoints, curDescriptors] = cv.SIFT(warpedImage);
            
            rawMatches = matcher.knnMatch(curDescriptors, 2);
            matches = [];
            for iMatch = 1:length(rawMatches)
                if rawMatches{iMatch}(1).distance * 0.8 < rawMatches{iMatch}(2).distance
                    if isempty(matches)
                        matches = rawMatches{iMatch}(1);
                    else
                        matches(end+1) = rawMatches{iMatch}(1); %#ok<AGROW>
                    end                    
                end
            end % for iMatch = 1:length(matches)
            
%             keyboard
        end
        
        curKeypointLocations = ones(3, length(curKeypoints));
        for i = 1:length(curKeypoints)
            curKeypointLocations(1:2, i) = curKeypoints(i).pt;
        end % for iKeypoint = 1:length(curKeypoints)
        
        figure(2);
        hold off;
        imshows(warpedImage, 2);
        hold on;
        scatter(curKeypointLocations(1,:)+1, curKeypointLocations(2,:)+1);
        
        
        
        warpedKeypointLocations = homography * baseKeypointLocations;
        warpedKeypointLocations = warpedKeypointLocations(1:2,:) ./ repmat(warpedKeypointLocations(3,:),[2,1]);
        
        numCorrect = 0;
        numIncorrect = 0;
        for iMatch = 1:length(matches)
            %             if matches(iMatch).distance < 100
            curPoint = curKeypoints(matches(iMatch).queryIdx+1).pt';
            basePoint = warpedKeypointLocations(:, matches(iMatch).trainIdx+1);
            
            dist = sqrt(sum((curPoint-basePoint).^2));
            
            if dist <= maxMatchDistance
                numCorrect = numCorrect + 1;
            else
                numIncorrect = numIncorrect + 1;
            end
            %             end % if matches(iMatch).distance <
        end % for iKeypoint = 1:length(baseKeypoints)
        
        disp(sprintf('Rotation %0.2f accuracy is %d:%d = %0.2f%%', theta*180/pi, numCorrect, numIncorrect, 100 * numCorrect / (numCorrect+numIncorrect)))
        pause()
    end
    
    keyboard
    
    
    
    