function [squareWidth_pix, padding_pix] = GetFiducialPixelSize(imageSize)

imageWidthFraction = 1 - 2*VisionMarkerTrained.SquareWidthFraction - ...
    2*VisionMarkerTrained.FiducialPaddingFraction*VisionMarkerTrained.SquareWidthFraction;

pixScale = imageSize/imageWidthFraction;

squareWidth_pix = VisionMarkerTrained.SquareWidthFraction * pixScale;

padding_pix     = VisionMarkerTrained.FiducialPaddingFraction*squareWidth_pix;

squareWidth_pix = round(squareWidth_pix);
padding_pix = round(padding_pix);

end