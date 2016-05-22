
% function binaryTracker_computeUpdate()

% updateType in {'translation', 'projective'}

% real images
% im1 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_0.png');
% im2 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_5.png');
% mask1 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_0_mask.png');
% extremaDerivativeThreshold = 255.0;

% im1 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_10.png');
% im2 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_12.png');
% mask1 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_12_mask.png');
% extremaDerivativeThreshold = 255.0;

% Horizontal-only shift
% im1 = zeros([480,640]); im1(50:100,50:150) = 1; im1(60:90,60:140) = 0;
% im2 = zeros([480,640]); im2(50:100,5+(50:150)) = 1; im2(60:90,5+(60:140)) = 0;
% mask1 = ones(size(im1));
% extremaDerivativeThreshold = 0.5;

% im1 = imresize(im1, [240,320]);
% im2 = imresize(im2, [240,320]);
% mask1 = imresize(mask1, [240,320]);

% im1 = imresize(im1, [120,160]);
% im2 = imresize(im2, [120,160]);
% mask1 = imresize(mask1, [120,160]);

% updatedHomography = binaryTracker_computeUpdate(im1, mask1, im2, eye(3), 1.0, 5, 1.0, extremaDerivativeThreshold, 15, 'translation')
% updatedHomography = binaryTracker_computeUpdate(im1, mask1, im2, eye(3), 1.0, 5, 1.0, extremaDerivativeThreshold, 15, 'projective')

% homography = eye(3);
% for i = 1:10
%   maxMatchingDistance = floor(14/i);
%   maxMatchingDistance = maxMatchingDistance + (1-mod(maxMatchingDistance,2));
%   homography = binaryTracker_computeUpdate(im1, mask1, im2, homography, 1.0, 5, 1.0, extremaDerivativeThreshold, maxMatchingDistance, 'translation')
% end

% homography1 = eye(3);
% im1w1 = binaryTracker_warpWithHomography(im1, homography1);
% homography2 = binaryTracker_computeUpdate(im1, mask1, im2, homography1, 1.0, 5, 1.0, extremaDerivativeThreshold, 7, 'translation')
% im1w2 = binaryTracker_warpWithHomography(im1, homography2);
% homography3 = binaryTracker_computeUpdate(im1, mask1, im2, homography2, 1.0, 5, 1.0, extremaDerivativeThreshold, 3, 'translation')
% im1w3 = binaryTracker_warpWithHomography(im1, homography3);
% homography4 = binaryTracker_computeUpdate(im1, mask1, im2, homography3, 1.0, 5, 1.0, extremaDerivativeThreshold, 7, 'projective')
% im1w4 = binaryTracker_warpWithHomography(im1, homography4);
% homography5 = binaryTracker_computeUpdate(im1, mask1, im2, homography4, 1.0, 5, 1.0, extremaDerivativeThreshold, 5, 'projective')
% im1w5 = binaryTracker_warpWithHomography(im1, homography5); 
% imshows(im1, im2, uint8(im1w2), uint8(im1w3), uint8(im1w4), uint8(im1w5), 'maximize');

% homography1 = eye(3);
% homography2 = binaryTracker_computeUpdate(im1, mask1, im2, homography1, 1.0, 5, 1.0, extremaDerivativeThreshold, 7, 'translation')
% im1w2a = binaryTracker_warpWithHomography(im1, homography2);
% homography3 = binaryTracker_computeUpdate(im1, mask1, im2, homography2, 1.0, 5, 1.0, extremaDerivativeThreshold, 7, 'projective')
% im1w3a = binaryTracker_warpWithHomography(im1, homography3);
% imshows(im1, im2, uint8(im1w2a), uint8(im1w3a), 'maximize');

function updatedHomography = binaryTracker_computeUpdate(...
    templateImage, templateMask,...
    newImage,...
    initialHomography, scale,...
    extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold,...
    maxMatchingDistance,...
    updateType, binarizeWithAutomata)

if ~exist('binarizeWithAutomata', 'var')
    binarizeWithAutomata = true;
end

grayvalueThreshold = 100;
minComponentWidth = 2;

assert(size(templateImage,1) == size(newImage,1));
assert(size(templateImage,2) == size(newImage,2));
assert(size(templateImage,1) == size(templateMask,1));
assert(size(templateImage,2) == size(templateMask,2));
assert(size(templateImage,3) == 1);
assert(size(templateMask,3) == 1);
assert(size(newImage,3) == 1);
assert(mod(maxMatchingDistance,2) == 1);

imageHeight = size(templateImage, 1);
imageWidth = size(templateImage, 2);

imageResizedHeight = imageHeight * scale;
imageResizedWidth = imageWidth * scale;

% Make the mask its bounding rectangle
[indsY, indsX] = find(templateMask ~= 0);
templateMask(:,:) = 0;
maskLimits = [min(indsX(:)), max(indsX(:)), min(indsY(:)), max(indsY(:))];
templateMask(maskLimits(3):maskLimits(4), maskLimits(1):maskLimits(2)) = 1;

templateImageResized = imresize(templateImage, [imageResizedHeight, imageResizedWidth]);
newImageResized = imresize(newImage, [imageResizedHeight, imageResizedWidth]);

if binarizeWithAutomata
    [xMinima1Image, xMaxima1Image, yMinima1Image, yMaxima1Image] = binaryTracker_binarizeWithAutomata(templateImageResized, grayvalueThreshold, minComponentWidth);
else
    [~, xMinima1Image, yMinima1Image] = computeBinaryExtrema(...
        templateImageResized, extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, true, false);

    [~, xMaxima1Image, yMaxima1Image] = computeBinaryExtrema(...
        templateImageResized, extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, false, true);
end

xMinima1Image(~templateMask) = 0;
yMinima1Image(~templateMask) = 0;
xMaxima1Image(~templateMask) = 0;
yMaxima1Image(~templateMask) = 0;

if binarizeWithAutomata
    [xMinima2Image, xMaxima2Image, yMinima2Image, yMaxima2Image] = binaryTracker_binarizeWithAutomata(newImageResized, grayvalueThreshold, minComponentWidth);
else
    [~, xMinima2Image, yMinima2Image] = computeBinaryExtrema(...
        newImageResized, extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, true, false);

    [~, xMaxima2Image, yMaxima2Image] = computeBinaryExtrema(...
        newImageResized, extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, false, true);
end

homographyOffset = [(imageWidth-1)/2, (imageHeight-1)/2];

% [xGrid, yGrid] = meshgrid(...
%     linspace(-homographyOffset(1), homographyOffset(1), imageResizedWidth), ...
%     linspace(-homographyOffset(2), homographyOffset(2), imageResizedHeight));

[xGrid, yGrid] = meshgrid(...
    linspace(0, 319, imageResizedWidth), ...
    linspace(0, 239, imageResizedHeight));

xMinima1Inds = find(xMinima1Image' ~= 0);
yMinima1Inds = find(yMinima1Image ~= 0);
xMaxima1Inds = find(xMaxima1Image' ~= 0);
yMaxima1Inds = find(yMaxima1Image ~= 0);

xGridTransposed = xGrid';
yGridTransposed = yGrid';

xMinima1List_x = xGridTransposed(xMinima1Inds) - homographyOffset(1);
xMinima1List_y = yGridTransposed(xMinima1Inds) - homographyOffset(2);

yMinima1List_x = xGrid(yMinima1Inds) - homographyOffset(1);
yMinima1List_y = yGrid(yMinima1Inds) - homographyOffset(2);

xMaxima1List_x = xGridTransposed(xMaxima1Inds) - homographyOffset(1);
xMaxima1List_y = yGridTransposed(xMaxima1Inds) - homographyOffset(2);

yMaxima1List_x = xGrid(yMaxima1Inds) - homographyOffset(1);
yMaxima1List_y = yGrid(yMaxima1Inds) - homographyOffset(2);

% xMinima2Image(5,5) = 1;
% figure(1);
% hold off;
% imshow(xMinima2Image);
% hold on;
% scatter(xMinima2List_x+imageWidth/2+0.5, xMinima2List_y+imageHeight/2+0.5);
% scatter(5,5, 'r+')

correspondences_xMinima = findCorrespondences(xMinima1List_x, xMinima1List_y, xMinima2Image, initialHomography, homographyOffset, maxMatchingDistance, false);
correspondences_xMaxima = findCorrespondences(xMaxima1List_x, xMaxima1List_y, xMaxima2Image, initialHomography, homographyOffset, maxMatchingDistance, false);
correspondences_yMinima = findCorrespondences(yMinima1List_x, yMinima1List_y, yMinima2Image, initialHomography, homographyOffset, maxMatchingDistance, true);
correspondences_yMaxima = findCorrespondences(yMaxima1List_x, yMaxima1List_y, yMaxima2Image, initialHomography, homographyOffset, maxMatchingDistance, true);

allCorrespondences = [correspondences_xMinima, correspondences_xMaxima, correspondences_yMinima, correspondences_yMaxima];
% allCorrespondences = [correspondences_yMinima, correspondences_yMaxima];
% allCorrespondences = [correspondences_xMinima, correspondences_xMaxima];

%allCorrespondences = allCorrespondences(:,1:527);

updatedHomography = updateHomography(initialHomography, allCorrespondences, updateType);

% keyboard
% allCorrespondences =

function correspondences = findCorrespondences(templateExtremaList_x, templateExtremaList_y, newExtremaImage, homography, homographyOffset, maxMatchingDistance, isVerticalSearch)
    % TODO: bilinear interpolation, instead of nearest neighbor

%     matchingFilterHalfWidth = floor(maxMatchingDistance / 2);

    offsets = (-maxMatchingDistance):maxMatchingDistance;

    points = [templateExtremaList_x, templateExtremaList_y, ones(length(templateExtremaList_y),1)]';

    warpedPoints = homography*points;
    warpedPoints = warpedPoints(1:2,:) ./ repmat(warpedPoints(3,:), [2,1]);

    correspondences = zeros(7,0);
    
    for iPoint = 1:size(warpedPoints,2)
        x = points(1, iPoint);
        y = points(2, iPoint);

        warpedX = warpedPoints(1, iPoint);
        warpedY = warpedPoints(2, iPoint);
        
        warpedXrounded = round(warpedX + homographyOffset(1) + 0.5);
        warpedYrounded = round(warpedY + homographyOffset(2) + 0.5);

        if warpedXrounded > maxMatchingDistance && warpedXrounded <= (size(newExtremaImage,2)-maxMatchingDistance) &&...
           warpedYrounded > maxMatchingDistance && warpedYrounded <= (size(newExtremaImage,1)-maxMatchingDistance)

            if isVerticalSearch
                for iOffset = 1:length(offsets)
                    xp = warpedX;
                    yp = warpedY + offsets(iOffset);
                    
                    xpRounded = warpedXrounded;
                    ypRounded = warpedYrounded + offsets(iOffset);

                    if newExtremaImage(ypRounded,xpRounded) ~= 0
                        correspondences(:,end+1) = [x, y, warpedX, warpedY, xp, yp, true];
                    end
                end
            else
                for iOffset = 1:length(offsets)
                    xp = warpedX + offsets(iOffset);
                    yp = warpedY;
                    
                    xpRounded = warpedXrounded + offsets(iOffset);
                    ypRounded = warpedYrounded;
                    
                    if newExtremaImage(ypRounded,xpRounded) ~= 0
                        correspondences(:,end+1) = [x, y, warpedX, warpedY, xp, yp, false];
                    end
                end

            end
        end % if point is in bounds
    end % for iPoint = 1:size(warpedPoints,2)

function updatedHomography = updateHomography(initialHomography, correspondences, updateType)

    if strcmpi(updateType, 'translation')
        verticalInds = correspondences(7,:) == true;
        horizontalInds = correspondences(7,:) == false;
        
        update = [mean(correspondences(5,horizontalInds) - correspondences(3,horizontalInds)), mean(correspondences(6,verticalInds) - correspondences(4,verticalInds))];
        
        updatedHomography = initialHomography;
        updatedHomography(1:2,3) = updatedHomography(1:2,3) + [update(1); update(2)];
    else
        if strcmpi(updateType, 'projective')
            numPoints = size(correspondences,2);

            A = zeros(numPoints, 8);
            b = zeros(8, 1);

            for i = 1:numPoints
                xi = correspondences(1,i);
                yi = correspondences(2,i);

                if correspondences(7,i) == true
                    yp = correspondences(6,i);
                    A(i, :) = [ 0,  0, 0, -xi, -yi, -1,  xi*yp,  yi*yp];
                    b(i) = -yp;
                else
                    xp = correspondences(5,i);
                    A(i, :) = [xi, yi, 1,   0,  0,   0, -xi*xp, -yi*xp];
                    b(i) = xp;
                end
            end
        elseif strcmpi(updateType, 'affine')
            numPoints = size(correspondences,2);

            A = zeros(numPoints, 6);
            b = zeros(6, 1);

            for i = 1:numPoints
                xi = correspondences(1,i);
                yi = correspondences(2,i);

                if correspondences(7,i) == true
                    yp = correspondences(6,i);
                    A(i, :) = [ 0,  0, 0, -xi, -yi, -1];
                    b(i) = -yp;
                else
                    xp = correspondences(5,i);
                    A(i, :) = [xi, yi, 1,   0,  0,   0];
                    b(i) = xp;
                end
            end
        end
        
        % The direct solution
        % homography = A \ b;

        % The cholesky solution
        AtA = A'*A;
        Atb = A'*b;

        L = chol(AtA);

        homography = L \ (L' \ Atb);
        homography(9) = 1;

        updatedHomography = reshape(homography, [3,3])';
    end
    
%     updatedHomography

%     keyboard






