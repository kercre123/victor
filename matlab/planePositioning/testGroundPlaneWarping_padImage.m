function [paddedImage, extraHeight2, extraWidth2] = testGroundPlaneWarping_padImage(image, paddedImageSize)
    
    paddedImage = nan * ones(paddedImageSize, 'uint8');
    
    extraHeight2 = floor((size(paddedImage,1) - size(image,1))/2);
    extraWidth2  = floor((size(paddedImage,2) - size(image,2))/2);
    
    yRange = [(1+extraHeight2), (extraHeight2+size(image,1))];
    xRange = [(1+extraWidth2),  (extraWidth2+size(image,2))];
    
    paddedImage(yRange(1):yRange(2), xRange(1):xRange(2)) = image;
    
end % function testGroundPlaneWarping_padImage)