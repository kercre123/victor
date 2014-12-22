function homography = testGroundPlaneWarping_homographyFromParameters(paddedImage, originalImageSize, rotation, heightRatio, widthRatio)
    extraHeight2 = floor((size(paddedImage,1) - originalImageSize(1))/2);
    extraWidth2  = floor((size(paddedImage,2) - originalImageSize(2))/2);
    
    rotation = [cos(rotation), -sin(rotation), 0;
        sin(rotation), cos(rotation), 0;
        0, 0, 1];
    
    translationF = [1, 0, -size(paddedImage,2)/2;
        0, 1, -size(paddedImage,1)/2;
        0, 0, 1];
    
    translationB = eye(3);
    translationB(1:2,3) = -translationF(1:2,3);
    
    bottomPoints = [extraWidth2,size(paddedImage,1)-extraHeight2; size(paddedImage,2)-extraWidth2,size(paddedImage,1)-extraHeight2];
    originalTopPoints = [extraWidth2,extraHeight2; size(paddedImage,2)-extraWidth2,extraHeight2];

    warpedY = originalImageSize(1) - (heightRatio * originalImageSize(1)) + extraHeight2;
    warpedDx = (originalImageSize(2) - (widthRatio * originalImageSize(2))) / 2;
    warpedTopPoints = [warpedDx+extraWidth2,warpedY; size(paddedImage,2)-warpedDx-extraWidth2,warpedY];

    allOriginalPoints = [originalTopPoints; bottomPoints];
    allWarpedPoints = [warpedTopPoints; bottomPoints];

    tform = fitgeotrans(allOriginalPoints, allWarpedPoints, 'projective');

    homography = tform.T' * translationB * rotation * translationF;
    homography = homography / homography(3,3);
    
end % function testGroundPlaneWarping_homographyFromParameters()