function [xMat, yMat, xvecRot, yvecRot, orient1] = matLocalization_step7_localizeOnMat(marker,  squareWidth, pixPerMM, imgCen, xcenIndex, ycenIndex, matSize, zDirection, embeddedConversions, DEBUG_DISPLAY)

if marker.isValid
    % Get camera/image position on mat, in mm:
    
    switch(zDirection)
        case 'up'
            xvec = imgCen(1)-xcenIndex;
            yvec = imgCen(2)-ycenIndex;
            
        case 'down'
            xvec = xcenIndex - imgCen(1);
            yvec = ycenIndex - imgCen(2);
            
        otherwise
            error('Unrecognized Zdirection "%s".', zDirection);
    end
    
    xvecRot =  xvec*cos(-marker.upAngle) + yvec*sin(-marker.upAngle);
    yvecRot = -xvec*sin(-marker.upAngle) + yvec*cos(-marker.upAngle);
            
    xMat = xvecRot/pixPerMM + marker.X*squareWidth - squareWidth/2 - matSize(1)/2;
    if strcmp(zDirection, 'down')
        xMat = -xMat;
    end
    yMat = yvecRot/pixPerMM + marker.Y*squareWidth - squareWidth/2 - matSize(2)/2;
    orient1 = marker.upAngle;
else
    xMat = -1;
    yMat = -1;
    xvecRot = -1;
    yvecRot = -1;
    orient1 = -1;
end