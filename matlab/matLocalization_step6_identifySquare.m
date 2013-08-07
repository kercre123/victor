function [xMat, yMat, xvecRot, yvecRot, orient1] = matLocalization_step6_identifySquare(imgRot, orient1, squareWidth, lineWidth, pixPerMM, imgCen, xcenIndex, ycenIndex, embeddedConversions, DEBUG_DISPLAY)


insideSquareWidth = squareWidth - lineWidth;
 
% s = diag(insideSquareWidth * pixPerMM * [1 1]);
% R = [cos(orient1) sin(orient1); -sin(orient1) cos(orient1)];
% t = [xcen; ycen] - R*diag(s)/2;
% tform = maketform('affine', [s*R t; 0 0 1]');
% 
% corners = [0 0; ...
%            0 1; ...
%            1 0; ...
%            1 1];
%         
% corners = tformfwd(tform, corners);
%     
% marker = MatMarker2D(img, corners, maketform('affine', tform.tdata.Tinv));
s = diag(insideSquareWidth * pixPerMM * [1 1]);
R = eye(2);
t = [xcenIndex; ycenIndex] - diag(s)/2;
tform = maketform('affine', [s*R t; 0 0 1]');

corners = [0 0; ...
           0 1; ...
           1 0; ...
           1 1];
        
corners = tformfwd(tform, corners);

% Note we're using the rotated, undistorted image for identifying the code
marker = MatMarker2D(imgRot, corners, maketform('affine', tform.tdata.Tinv));

if marker.isValid
    % Get camera/image position on mat, in mm:
    xvec = imgCen(1)-xcenIndex;
    yvec = imgCen(2)-ycenIndex;
    xvecRot =  xvec*cos(-marker.upAngle) + yvec*sin(-marker.upAngle);
    yvecRot = -xvec*sin(-marker.upAngle) + yvec*cos(-marker.upAngle);
    xMat = xvecRot/pixPerMM + marker.X*squareWidth - squareWidth/2;
    yMat = yvecRot/pixPerMM + marker.Y*squareWidth - squareWidth/2;

    orient1 = orient1 + marker.upAngle;
    
else
    xMat = -1;
    yMat = -1;
    xvecRot = -1;
    yvecRot = -1;
end
