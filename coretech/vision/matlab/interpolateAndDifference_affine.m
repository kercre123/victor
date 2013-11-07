
% function interpolateAndDifference_affine()

% templateImage = zeros(16,16); for y=1:size(templateImage,1) templateImage(y,:) = y*(1:size(templateImage,2)); end;
% newImage = zeros(16,16); for y=1:size(newImage,1) newImage(y,:) = (1:size(newImage,2)); end;
% templateQuad = [5,4;10,4;10,8;5,8];

% homography = eye(3);
% homography = [.5,0,0;0,1,0;0,0,1];
% homography = [cos(.1), -sin(.1), 0; sin(.1), cos(.1), 0; 0,0,1];

% interpolateAndDifference_affine(templateImage, templateQuad, newImage, homography, 1);

function interpolateAndDifference_affine(templateImage, templateQuad, newImage, homography, scale, debugDisplay)

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

for y = minTemplateQuadY:maxTemplateQuadY
    minX = ceil((max(minIndexes(y+1), minTemplateQuadX+0.5))-0.5) + 0.5;
    maxX = floor(min(maxIndexes(y+1), maxTemplateQuadX+0.5)-0.5) + 0.5;
    
    disp(sprintf('y=%d x=[%f,%f]', y, minX, maxX));
%     keyboard 
end

keyboard 




