
% function lucasKanade_interpolateAndDifference_affine()

% templateImage = zeros(16,16); for y=1:size(templateImage,1) templateImage(y,:) = y*(1:size(templateImage,2)); end;
% newImage = zeros(16,16); for y=1:size(newImage,1) newImage(y,:) = .1*y + (1:size(newImage,2)); end;
% templateQuad = [5,4;10,4;10,8;5,8];

% homography = eye(3);
% homography = [.5,0,0;0,1,0;0,0,1];
% homography = [cos(.1), -sin(.1), 0; sin(.1), cos(.1), 0; 0,0,1];

% difference = lucasKanade_interpolateAndDifference_affine(templateImage, templateQuad, newImage, homography, 1);

function differences = lucasKanade_interpolateAndDifference_affine(templateImage, templateQuad, newImage, homography, scale, debugDisplay)

assert(isQuadARectangle(templateQuad))

if ~exist('debugDisplay', 'var')
    debugDisplay = false;
end

minTemplateQuadX = min(templateQuad(:,1));
maxTemplateQuadX = max(templateQuad(:,1));
minTemplateQuadY = min(templateQuad(:,2));
maxTemplateQuadY = max(templateQuad(:,2));

% homography(1,3) = homography(1,3) + minTemplateQuadX;
% homography(2,3) = homography(2,3) + minTemplateQuadY;

% homography = homography * [scale,0,0;0,scale,0;0,0,1];
% homography(1:2,3) = homography(1:2,3)*scale;

homography(1:2,1:3) = homography(1:2,1:3) * scale;

inputImageQuad = [0,0;size(newImage,2),0;size(newImage,2),size(newImage,1);0,size(newImage,1)];

[minIndexes, maxIndexes] = interpolateAffine_rowLimits(inputImageQuad, homography, scale*size(templateImage), debugDisplay);

if debugDisplay
    templateCornerPoints = [minTemplateQuadX, minTemplateQuadY;
                            maxTemplateQuadX+1, minTemplateQuadY;
                            maxTemplateQuadX+1, maxTemplateQuadY+1;
                            minTemplateQuadX, maxTemplateQuadY+1]';

    hold on;
    plot(templateCornerPoints(1,[1:4,1]), -templateCornerPoints(2,[1:4,1]), 'r--');
    hold off;
end

homography(3,:) = [0,0,1];
homographyInv = inv(homography);

newImageCoords_x0y0 = homographyInv*[0;0;1];
newImageCoords_x1y0 = homographyInv*[1;0;1];
% newImageCoords_x0y1 = homographyInv*[0;1;1];

% x_dx is the change in the newImage coordinates x value, when moving one
% pixel right in templateImage coordinates
x_dx = newImageCoords_x1y0(1) - newImageCoords_x0y0(1);

% x_dy is the change in the newImage coordinates y value, when moving one
% pixel right in templateImage coordinates
x_dy = newImageCoords_x1y0(2) - newImageCoords_x0y0(2);

% % y_dx is the change in the newImage coordinates x value, when moving one
% % pixel down in templateImage coordinates
% y_dx = newImageCoords_x0y1(1) - newImageCoords_x0y0(1);
%
% % y_dy is the change in the newImage coordinates y value, when moving one
% % pixel down in templateImage coordinates
% y_dy = newImageCoords_x0y1(2) - newImageCoords_x0y0(2);

numDifferences = 0;
for y = minTemplateQuadY:maxTemplateQuadY
    minX = ceil((max(minIndexes(y+1), minTemplateQuadX+0.5))-0.5) + 0.5;
    maxX = floor(min(maxIndexes(y+1), maxTemplateQuadX+1.0-0.5)-0.5) + 0.5;

    numDifferences = numDifferences + maxX - minX + 1;
end

differences = zeros(numDifferences,1);
curDifference = 1;

for y = minTemplateQuadY:maxTemplateQuadY
    minX = ceil((max(minIndexes(y+1), minTemplateQuadX+0.5))-0.5) + 0.5;
    maxX = floor(min(maxIndexes(y+1), maxTemplateQuadX+1.0-0.5)-0.5) + 0.5;

%     disp(sprintf('y=%d x=[%f,%f]', y, minX, maxX));

    % compute the (x,y) coordinates of the leftmost pixel in the newImage
    % coordinates

    newImageCoords = homographyInv*[minX;y;1];

    for x = (minX:maxX) + 0.5
        templatePixel = templateImage(y+1, x+1);

        pixel00 = newImage(floor(newImageCoords(2)), floor(newImageCoords(1)));
        pixel01 = newImage(ceil(newImageCoords(2)), floor(newImageCoords(1)));
        pixel10 = newImage(floor(newImageCoords(2)), ceil(newImageCoords(1)));
        pixel11 = newImage(ceil(newImageCoords(2)), ceil(newImageCoords(1)));

        alphaY = newImageCoords(2) - floor(newImageCoords(2));
        alphaYinverse = 1 - alphaY;

        alphaX = newImageCoords(1) - floor(newImageCoords(1));
        alphaXinverse = 1 - alphaX;

        interpolatedPixel = interpolate2d(pixel00, pixel01, pixel10, pixel11, alphaY, alphaYinverse, alphaX, alphaXinverse);

        differences(curDifference) = interpolatedPixel - templatePixel;

        curDifference = curDifference + 1;

        newImageCoords = newImageCoords + [x_dx;x_dy;0];
    end % for x = (minX:maxX) + 0.5

%     keyboard
end % for y = minTemplateQuadY:maxTemplateQuadY

% keyboard


end % function interpolateAndDifference_affine()

function interpolatedPixel = interpolate2d(pixel00, pixel01, pixel10, pixel11, alphaY, alphaYinverse, alphaX, alphaXinverse)
    interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
    interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;

    interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;
end
