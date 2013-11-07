% function lucasKanade_computeUpdate()

% templateImage = zeros(16,16); for y=1:size(templateImage,1) templateImage(y,:) = y*(1:size(templateImage,2)); end;
% warpedTemplateImage = lucasKande_warpGroundTruth(templateImage, [1,0,0.5; 0,1,0.1; 0,0,1], size(templateImage));
% templateQuad = [5,4;10,4;10,8;5,8];
% numScales = 2;
% [A_translationOnly, A_affine] = lucasKanade_init(templateImage, templateQuad, numScales, false, true);
% [update, A, b] = lucasKanade_computeUpdate(templateImage, templateQuad, A_translationOnly{1}, warpedTemplateImage, eye(3), 1, true);

function [update, A, b] = lucasKanade_computeUpdate(templateImage, templateQuad, AFull, newImage, homography, scale, debugDisplay)

% AtW = (A(inBounds,:).*this.W{i_scale}(inBounds,ones(1,size(A,2))))'; % 2x169 or 6x169
% AtWA = AtW*A(inBounds,:); % 2x2 or 6x6
% b = AtW*It(inBounds); % 2x1 or 6x1
% update = AtWA\b; % 2x1 or 6x1

assert(isQuadARectangle(templateQuad))

if ~exist('debugDisplay', 'var')
    debugDisplay = false;
end

numModelParameters = size(AFull,3);

minTemplateQuadX = min(templateQuad(:,1));
maxTemplateQuadX = max(templateQuad(:,1));
minTemplateQuadY = min(templateQuad(:,2));
maxTemplateQuadY = max(templateQuad(:,2));

homography(1:2,1:3) = homography(1:2,1:3) * scale;

inputImageQuad = [0,0;size(newImage,2),0;size(newImage,2),size(newImage,1);0,size(newImage,1)];

[minIndexes, maxIndexes] = lucasKanade_affineRowLimits(inputImageQuad, homography, scale*size(templateImage), debugDisplay);

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

% x_dx is the change in the newImage coordinates x value, when moving one
% pixel right in templateImage coordinates
x_dx = newImageCoords_x1y0(1) - newImageCoords_x0y0(1);

% x_dy is the change in the newImage coordinates y value, when moving one
% pixel right in templateImage coordinates
x_dy = newImageCoords_x1y0(2) - newImageCoords_x0y0(2);

numInBounds = 0;
for y = minTemplateQuadY:maxTemplateQuadY
    minX = ceil((max(minIndexes(y+1), minTemplateQuadX+0.5))-0.5) + 0.5;
    maxX = floor(min(maxIndexes(y+1), maxTemplateQuadX+1.0-0.5)-0.5) + 0.5;

    numInBounds = numInBounds + maxX - minX + 1;
end

% AtW = (A(inBounds,:).*this.W{i_scale}(inBounds,ones(1,size(A,2))))'; % 2x169 or 6x169
% AtWA = AtW*A(inBounds,:); % 2x2 or 6x6 % A is 169x6
% b = AtW*It(inBounds); % b is 2x1 or 6x1 % It is 169x1
% update = AtWA\b; % 2x1 or 6x1

It = zeros(numInBounds,1);
A = zeros(numInBounds, numModelParameters);

curInBoundsElements = 1;

for y = minTemplateQuadY:maxTemplateQuadY
    minX = ceil((max(minIndexes(y+1), minTemplateQuadX+0.5))-0.5) + 0.5;
    maxX = floor(min(maxIndexes(y+1), maxTemplateQuadX+1.0-0.5)-0.5) + 0.5;

    % compute the (x,y) coordinates of the leftmost pixel in the newImage
    % coordinates
    newImageCoords = homographyInv*[minX;y+0.5;1];

    for x = (minX:maxX) - 0.5
        templatePixel = templateImage(y+1, x+1);

        pixel00 = newImage(floor(newImageCoords(2)-0.5)+1, floor(newImageCoords(1)-0.5)+1);
        pixel01 = newImage(ceil(newImageCoords(2)-0.5)+1, floor(newImageCoords(1)-0.5)+1);
        pixel10 = newImage(floor(newImageCoords(2)-0.5)+1, ceil(newImageCoords(1)-0.5)+1);
        pixel11 = newImage(ceil(newImageCoords(2)-0.5)+1, ceil(newImageCoords(1)-0.5)+1);

        alphaY = newImageCoords(2) - (floor(newImageCoords(2)- 0.5)+0.5);
        alphaYinverse = 1 - alphaY;

        alphaX = newImageCoords(1) - (floor(newImageCoords(1)- 0.5)+0.5);
        alphaXinverse = 1 - alphaX;

        interpolatedPixel = interpolate2d(pixel00, pixel01, pixel10, pixel11, alphaY, alphaYinverse, alphaX, alphaXinverse);
        
        disp(sprintf('%f %f', templatePixel, interpolatedPixel));

        It(curInBoundsElements) = interpolatedPixel - templatePixel;
        A(curInBoundsElements,:) = squeeze(AFull(y+1, x+1, :));
        
        curInBoundsElements = curInBoundsElements + 1;

        newImageCoords = newImageCoords + [x_dx;x_dy;0];
    end % for x = (minX:maxX) + 0.5

%     keyboard
end % for y = minTemplateQuadY:maxTemplateQuadY

AtA = A' * A;
b = A' * It;

update = AtA \ b; % TODO: use SVD

keyboard

end % function interpolateAndDifference_affine()

function interpolatedPixel = interpolate2d(pixel00, pixel01, pixel10, pixel11, alphaY, alphaYinverse, alphaX, alphaXinverse)
    interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
    interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;

    interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;
end
