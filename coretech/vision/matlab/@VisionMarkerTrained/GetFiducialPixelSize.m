function [squareWidth_pix, padding_pix, cornerRadius_pix] = GetFiducialPixelSize(imageSize, ...
    imageSizeType)

switch(imageSizeType)
    case 'CodeImageOnly'
        
        fiducialWidth_pix = imageSize / (1-2*VisionMarkerTrained.SquareWidthFraction-2*VisionMarkerTrained.FiducialPaddingFraction);
                 
        %squareWidth_pix = VisionMarkerTrained.SquareWidthFraction * imageSize;
        
        %padding_pix     = VisionMarkerTrained.FiducialPaddingFraction*squareWidth_pix;
        
    case 'WithUnpaddedFiducial'
        
        fiducialWidth_pix = imageSize;
        
    case 'WithPaddedFiducial'
        
        fiducialWidth_pix = imageSize / (1 + 2*VisionMarkerTrained.FiducialPaddingFraction);
        
%         codeImageWidth = imageSize / (1 + 2*VisionMarkerTrained.SquareWidthFraction + ...
%             4*VisionMarkerTrained.FiducialPaddingFraction*VisionMarkerTrained.SquareWidthFraction);
%          
%         squareWidth_pix = VisionMarkerTrained.SquareWidthFraction*codeImageWidth;
%         padding_pix = VisionMarkerTrained.FiducialPaddingFraction*squareWidth_pix;
        
    otherwise
        error('Unrecognized imageSizeType "%s"', imageSizeType);
end
   
squareWidth_pix = VisionMarkerTrained.SquareWidthFraction * fiducialWidth_pix;
padding_pix = VisionMarkerTrained.FiducialPaddingFraction * fiducialWidth_pix;
cornerRadius_pix = VisionMarkerTrained.CornerRadiusFraction * fiducialWidth_pix;
         
%squareWidth_pix = round(squareWidth_pix);
%padding_pix = round(padding_pix);

end