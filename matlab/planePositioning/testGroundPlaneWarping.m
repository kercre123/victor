% function testGroundPlaneWarping()

% Test how accurate feature matching is, with:
% 1. Different amounts of ground plane warping
% 2. Different accuracy of ground warp

% [orb_allResults, rotations, heightRatios, widthRatios] = testGroundPlaneWarping('ORB');
% [sift_allResults, rotations, heightRatios, widthRatios] = testGroundPlaneWarping('SIFT');

function [allResults, rotations, heightRatios, widthRatios] = testGroundPlaneWarping(testFeatureType)
    
    maxMatchDistance = 4; % number of pixels
        
    showIntermediate = false;
    
    image = rgb2gray2(imread('~/Documents/Anki/products-cozmo-large-files/peopleScanned640x480.png'));
    maxDimension = max(size(image));
    
    [paddedImage, ~, ~] = testGroundPlaneWarping_padImage(image, ceil(sqrt(2)*[maxDimension,maxDimension]));
    
    patchSize = 31;
    
    if strcmpi(testFeatureType, 'ORB')
        [baseKeypoints, baseDescriptors] = cv.ORB(paddedImage,...
            'NFeatures', 500,...
            'ScaleFactor', 1.2,...
            'NLevels', 3,...
            'EdgeThreshold', patchSize,...
            'WTA_K', 2,...
            'PatchSize', patchSize);
        
        matcher = cv.DescriptorMatcher('BruteForce-L1');
        matcher.add(baseDescriptors);
        matcher.train();
    elseif strcmpi(testFeatureType, 'SIFT')
        [baseKeypoints, baseDescriptors] = cv.SIFT(paddedImage);
        
        matcher = cv.DescriptorMatcher('FlannBased');
        matcher.add(baseDescriptors);
        matcher.train();
    else
        assert(false);
    end
    
    baseKeypointLocations = ones(3, length(baseKeypoints));
    for i = 1:length(baseKeypoints)
        baseKeypointLocations(1:2, i) = baseKeypoints(i).pt + 1;
    end % for iKeypoint = 1:length(baseKeypoints)
    
    rotations = linspace(0, 2*pi, 17);
    rotations = rotations(1:(end-1));

    heightRatios = 1:-0.1:0.3;
    widthRatios = 1:-0.1:0.6;
        
    allDimensions = [length(rotations), length(heightRatios), length(widthRatios)];
    allResults = struct(...
        'rawImage',     struct('numCorrect', {zeros(allDimensions)}, 'numIncorrect', {zeros(allDimensions)}),...
        'unwarpedImage', struct('numCorrect', {zeros(allDimensions)}, 'numIncorrect', {zeros(allDimensions)}));
       
    for iRotation = 1:length(rotations)
        for iHeight = 1:length(heightRatios)
            for iWidth = 1:length(widthRatios)
                homography = testGroundPlaneWarping_homographyFromParameters(paddedImage, size(image), rotations(iRotation), heightRatios(iHeight), widthRatios(iWidth));
                
                [warpedImage, ~, ~] = warpProjective(paddedImage, homography, size(paddedImage));
                warpedImage = uint8(warpedImage);
                
                [unwarpedImage, ~, ~] = warpProjective(warpedImage, inv(homography), size(paddedImage));
                unwarpedImage = uint8(unwarpedImage);
                
                rawImageMatches = struct();
                [rawImageMatches.keypoints, rawImageMatches.descriptors, rawImageMatches.matches] = computeMatches(warpedImage, testFeatureType, patchSize, matcher);
                
                unwarpedMatches = struct();
                [unwarpedMatches.keypoints, unwarpedMatches.descriptors, unwarpedMatches.matches] = computeMatches(unwarpedImage, testFeatureType, patchSize, matcher);
                
                groundTruthKeypointLocations = homography * baseKeypointLocations;
                groundTruthKeypointLocations = groundTruthKeypointLocations(1:2,:) ./ repmat(groundTruthKeypointLocations(3,:),[2,1]);
                
                [rawImage_numCorrect, rawImage_numIncorrect] = computeAccuracy(rawImageMatches, groundTruthKeypointLocations, maxMatchDistance);
                
                [unwarpedImage_numCorrect, unwarpedImage_numIncorrect] = computeAccuracy(unwarpedMatches, baseKeypointLocations(1:2,:), maxMatchDistance);
                                
                allResults.rawImage.numCorrect(iRotation, iHeight, iWidth) = allResults.rawImage.numCorrect(iRotation, iHeight, iWidth) + rawImage_numCorrect;
                allResults.rawImage.numIncorrect(iRotation, iHeight, iWidth) = allResults.rawImage.numIncorrect(iRotation, iHeight, iWidth) + rawImage_numIncorrect;
                
                allResults.unwarpedImage.numCorrect(iRotation, iHeight, iWidth) = allResults.unwarpedImage.numCorrect(iRotation, iHeight, iWidth) + unwarpedImage_numCorrect;
                allResults.unwarpedImage.numIncorrect(iRotation, iHeight, iWidth) = allResults.unwarpedImage.numIncorrect(iRotation, iHeight, iWidth) + unwarpedImage_numIncorrect;
                                
                disp(sprintf('Rotation:%d widthRatio:%0.2f heightRatio:%0.2f accuracy is %d:%d=%0.2f%% or %d:%d=%0.2f%%', round(rotations(iRotation)*180/pi), widthRatios(iWidth), heightRatios(iHeight), rawImage_numCorrect, rawImage_numIncorrect, 100 * rawImage_numCorrect / (rawImage_numCorrect+rawImage_numIncorrect), unwarpedImage_numCorrect, unwarpedImage_numIncorrect, 100 * unwarpedImage_numCorrect / (unwarpedImage_numCorrect+unwarpedImage_numIncorrect)))

                if showIntermediate
                    imshows(paddedImage,warpedImage,unwarpedImage)
                    pause(0.25)
                end
%                 pause()
            end % for iWidth = 1:length(widthRatios)
        end % for iHeight = 1:length(heightRatios)
    end % for iRotation = 1:length(rotations)
    
%     keyboard
end % function testGroundPlaneWarping()
    
function [keypoints, descriptors, matches] = computeMatches(image, testFeatureType, patchSize, matcher)
    if strcmpi(testFeatureType, 'ORB')
        [keypoints, descriptors] = cv.ORB(image,...
            'NFeatures', 500,...
            'ScaleFactor', 1.2,...
            'NLevels', 3,...
            'EdgeThreshold', patchSize,...
            'WTA_K', 2,...
            'PatchSize', patchSize);

        matches = matcher.match(descriptors);
    elseif strcmpi(testFeatureType, 'SIFT')
        [keypoints, descriptors] = cv.SIFT(image);

        rawMatches = matcher.knnMatch(descriptors, 2);
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
    end % if strcmpi(testFeatureType, 'ORB') ... else
end % function computeMatches
    
function [numCorrect, numIncorrect] = computeAccuracy(matches, groundTruthKeypointLocations, maxMatchDistance)
    curKeypointLocations = ones(3, length(matches.keypoints));
    for i = 1:length(matches.keypoints)
        curKeypointLocations(1:2, i) = matches.keypoints(i).pt;
    end % for iKeypoint = 1:length(curKeypoints)

    numCorrect = 0;
    numIncorrect = 0;
    for iMatch = 1:length(matches.matches)
        curPoint = matches.keypoints(matches.matches(iMatch).queryIdx+1).pt';
        warpedBasePoint = groundTruthKeypointLocations(:, matches.matches(iMatch).trainIdx+1) - 0.5;

        dist = sqrt(sum((curPoint-warpedBasePoint).^2));

        if dist <= maxMatchDistance
            numCorrect = numCorrect + 1;
        else
            numIncorrect = numIncorrect + 1;
        end
    end % for iKeypoint = 1:length(baseKeypoints)
end % function computeAccuracy()
    