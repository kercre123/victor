function marker = matLocalization_step6_findMarker(imgRot, orient, squareWidth, lineWidth, pixPerMM, xcenIndex, ycenIndex, embeddedConversions, DEBUG_DISPLAY)

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

% Make the marker (and it's upAngle) in the coordinate frame of the
% original, unrotated image:
[nrows,ncols] = size(imgRot);
marker = rotate(marker, -orient, [ncols nrows]/2);