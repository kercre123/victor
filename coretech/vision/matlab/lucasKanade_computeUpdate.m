% function lucasKanade_computeUpdate()

% templateImage = zeros(16,16); for y=1:size(templateImage,1) templateImage(y,:) = y*(1:size(templateImage,2)); end;
% warpedTemplateImage = warpProjective(templateImage, [1,0,0.5; 0,1,0.1; 0,0,1], size(templateImage));
% templateQuad = [5,4;10,4;10,8;5,8];
% numScales = 2;

% templateImage = zeros(120,160); for y=1:size(templateImage,1) templateImage(y,:) = y*(1:size(templateImage,2)); end;
% warpedTemplateImage = warpProjective(templateImage, [1,0,0.5; 0,1,0.1; 0,0,1], size(templateImage));
% templateQuad = [50,50;50,100;100,100;100,50];
% numScales = 4;

% templateImage = zeros(120,160); for y=1:size(templateImage,1) templateImage(y,:) = y*(1:size(templateImage,2)); end;
% templateImage = templateImage / max(templateImage(:)); templateImage = round(50*templateImage);
% templateQuad = [3,3;3,152;142,152;142,3];
% [warpedTemplateImage, mask, warpedMask] = warpProjective(templateImage, [1,0,1; 0,1,0; 0,0,1], size(templateImage), templateQuad);
% warpedTemplateImage(isnan(warpedTemplateImage)) = 0;
% templateQuad = [5,5;5,150;140,150;140,5];
% numScales = 4;

% [A_translationOnly, A_affine, templateImagePyramid] = lucasKanade_init(templateImage, templateQuad, numScales, false, true);
% [update, A, b] = lucasKanade_computeUpdate(templateImagePyramid{1}, templateQuad, A_translationOnly{1}, warpedTemplateImage, eye(3), 1, false);
% [update, A, b] = lucasKanade_computeUpdate(templateImagePyramid{2}, templateQuad, A_translationOnly{2}, imresize(warpedTemplateImage, size(warpedTemplateImage)/2), eye(3), 2, false);
% [update, A, b] = lucasKanade_computeUpdate(templateImagePyramid{3}, templateQuad, A_translationOnly{3}, imresize(warpedTemplateImage, size(warpedTemplateImage)/(2^2)), eye(3), 2^2, false);
% [update, A, b] = lucasKanade_computeUpdate(templateImagePyramid{4}, templateQuad, A_translationOnly{4}, imresize(warpedTemplateImage, size(warpedTemplateImage)/(2^3)), eye(3), 2^3, false);

% H = eye(3); for i=1:5 [update, A, b] = lucasKanade_computeUpdate(templateImagePyramid{1}, templateQuad, A_translationOnly{1}, warpedTemplateImage, H, 1, false); H(1:2,3) = H(1:2,3) - update(1:2); disp(H); end;

function [update, A, b] = lucasKanade_computeUpdate(templateImage, templateQuad, AFull, newImage, homography, scale, debugDisplay)

assert(isQuadARectangle(templateQuad))

if ~exist('debugDisplay', 'var')
    debugDisplay = false;
end

numModelParameters = size(AFull,3);

minTemplateQuadX = round(min(templateQuad(:,1)) / scale);
maxTemplateQuadX = round(max(templateQuad(:,1)) / scale);
minTemplateQuadY = round(min(templateQuad(:,2)) / scale);
maxTemplateQuadY = round(max(templateQuad(:,2)) / scale);

inputImageQuad = [0,0;size(newImage,2),0;size(newImage,2),size(newImage,1);0,size(newImage,1)];

smallHomography = homography;
smallHomography(1:2,3) = smallHomography(1:2,3) / scale;
smallHomography(3,:) = [0,0,1];

[minIndexes, maxIndexes] = lucasKanade_affineRowLimits(inputImageQuad, smallHomography, size(templateImage), debugDisplay);

if debugDisplay
    templateCornerPoints = [minTemplateQuadX, minTemplateQuadY;
                            maxTemplateQuadX+1, minTemplateQuadY;
                            maxTemplateQuadX+1, maxTemplateQuadY+1;
                            minTemplateQuadX, maxTemplateQuadY+1]';

    hold on;
    plot(templateCornerPoints(1,[1:4,1]), -templateCornerPoints(2,[1:4,1]), 'r--');
    hold off;
end

newImageCoords_x0y0 = smallHomography*[0;0;1];
newImageCoords_x1y0 = smallHomography*[1;0;1];

% x_dx is the change in the newImage coordinates x value, when moving one
% pixel right in templateImage coordinates
x_dx = newImageCoords_x1y0(1) - newImageCoords_x0y0(1);

% x_dy is the change in the newImage coordinates y value, when moving one
% pixel right in templateImage coordinates
x_dy = newImageCoords_x1y0(2) - newImageCoords_x0y0(2);

numInBounds = 0;
for y = minTemplateQuadY:maxTemplateQuadY
    if isinf(minIndexes(y+1)) || isinf(maxIndexes(y+1))
        continue;
    end

    minX = ceil((max(minIndexes(y+1), minTemplateQuadX+0.5))-0.5) + 0.5;
    maxX = floor(min(maxIndexes(y+1), maxTemplateQuadX+1.0-0.5)-0.5) + 0.5;

    if maxX < minX % should the happen if there's no bugs?
        keyboard
        continue;
    end

    numInBounds = numInBounds + maxX - minX + 1 - 2;
end

It = zeros(numInBounds,1);
A = zeros(numInBounds, numModelParameters);

curInBoundsElements = 1;

interpolatedFrom = zeros(size(newImage));
interpolatedSelection = zeros(size(templateImage));

for y = minTemplateQuadY:maxTemplateQuadY
    if isinf(minIndexes(y+1)) || isinf(maxIndexes(y+1))
        continue;
    end

    minX = ceil((max(minIndexes(y+1), minTemplateQuadX+0.5))-0.5) + 0.5 + 1;
    maxX = floor(min(maxIndexes(y+1), maxTemplateQuadX+1.0-0.5)-0.5) + 0.5 - 1;

    % compute the (x,y) coordinates of the leftmost pixel in the newImage
    % coordinates at this row
    newImageCoords = smallHomography*[minX;y+0.5;1];

    for x = (minX:maxX) - 0.5
        templatePixel = templateImage(y+1, x+1);

        % Because of the additions performed every loop, newImageCoords
        % should be equal to the following expression:
        % newImageCoords = smallHomography * [x+0.5;y+0.5;1];

        x0 = floor(newImageCoords(1)-0.5)+1;
        x1 = ceil(newImageCoords(1)-0.5)+1;

        y0 = floor(newImageCoords(2)-0.5)+1;
        y1 = ceil(newImageCoords(2)-0.5)+1;

        interpolatedFrom(y0:y1, x0:x1) = 1;

        pixelTL = newImage(y0, x0);
        pixelTR = newImage(y0, x1);
        pixelBL = newImage(y1, x0);
        pixelBR = newImage(y1, x1);

        alphaY = newImageCoords(2) - (floor(newImageCoords(2)- 0.5)+0.5);
        alphaYinverse = 1 - alphaY;

        alphaX = newImageCoords(1) - (floor(newImageCoords(1)- 0.5)+0.5);
        alphaXinverse = 1 - alphaX;

        interpolatedPixel = interpolate2d(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

        interpolatedSelection(y+1,x+1) = interpolatedPixel;

%         disp(sprintf('%f %f', templatePixel, interpolatedPixel));

        It(curInBoundsElements) = interpolatedPixel - templatePixel;
        A(curInBoundsElements,:) = squeeze(AFull(y+1, x+1, :));

        curInBoundsElements = curInBoundsElements + 1;

        newImageCoords = newImageCoords + [x_dx;x_dy;0];
    end % for x = (minX:maxX) + 0.5
end % for y = minTemplateQuadY:maxTemplateQuadY

% figure(); imshow(interpolatedSelection);

AtA = A' * A;
b = A' * It;

update = AtA \ b; % TODO: use SVD

end % function interpolateAndDifference_affine()

function interpolatedPixel = interpolate2d(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse)
    interpolatedTop = alphaXinverse*pixelTL + alphaX*pixelTR;
    interpolatedBottom = alphaXinverse*pixelBL + alphaX*pixelBR;

    interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;
end
