function Corners = GetFiducialCorners(imageSize, isPadded)

if isPadded
    [~, padding] = VisionMarkerTrained.GetFiducialPixelSize(imageSize, 'WithPaddedFiducial');
    
    Corners = [padding+1 padding+1;
        padding+1 imageSize-padding-1;
        imageSize-padding-1 padding+1;
        imageSize-padding-1 imageSize-padding-1];
else
    Corners = [1 1; 1 imageSize; imageSize 1; imageSize imageSize];
end
    
end