function Corners = GetFiducialCorners(imageSize)

[~, padding] = VisionMarkerTrained.GetFiducialPixelSize(imageSize, 'WithPaddedFiducial');

Corners = [padding+1 padding+1;
    padding+1 imageSize-padding-1;
    imageSize-padding-1 padding+1;
    imageSize-padding-1 imageSize-padding+1];
    
end