
% function binaryTracker_computeUpdateFromBinary()

% updateType in {'translation', 'projective'}

function [updatedHomography, numTemplatePixelsMatched] = binaryTracker_computeUpdateFromBinary(...
    xDecreasingTemplate, xIncreasingTemplate, yDecreasingTemplate, yIncreasingTemplate,...
    xDecreasingNew, xIncreasingNew, yDecreasingNew, yIncreasingNew,...
    initialHomography, scale,...
    maxMatchingDistance, updateType)

verifyRange = 1;

imageHeight = size(xDecreasingTemplate, 1);
imageWidth = size(xDecreasingTemplate, 2);

imageResizedHeight = imageHeight * scale;
imageResizedWidth = imageWidth * scale;

homographyOffset = [(imageWidth-1)/2, (imageHeight-1)/2];

[xGrid, yGrid] = meshgrid(...
    linspace(0, imageWidth-1, imageResizedWidth), ...
    linspace(0, imageHeight-1, imageResizedHeight));

xDecreasingTemplateInds = find(xDecreasingTemplate' ~= 0);
yDecreasingTemplateInds = find(yDecreasingTemplate ~= 0);
xIncreasingTemplateInds = find(xIncreasingTemplate' ~= 0);
yIncreasingTemplateInds = find(yIncreasingTemplate ~= 0);

xGridTransposed = xGrid';
yGridTransposed = yGrid';

xDecreasingTemplateList_x = xGridTransposed(xDecreasingTemplateInds) - homographyOffset(1);
xDecreasingTemplateList_y = yGridTransposed(xDecreasingTemplateInds) - homographyOffset(2);

yDecreasingTemplateList_x = xGrid(yDecreasingTemplateInds) - homographyOffset(1);
yDecreasingTemplateList_y = yGrid(yDecreasingTemplateInds) - homographyOffset(2);

xIncreasingTemplateList_x = xGridTransposed(xIncreasingTemplateInds) - homographyOffset(1);
xIncreasingTemplateList_y = yGridTransposed(xIncreasingTemplateInds) - homographyOffset(2);

yIncreasingTemplateList_x = xGrid(yIncreasingTemplateInds) - homographyOffset(1);
yIncreasingTemplateList_y = yGrid(yIncreasingTemplateInds) - homographyOffset(2);

correspondences_xDecreasing = findCorrespondences(xDecreasingTemplateList_x, xDecreasingTemplateList_y, xDecreasingNew, initialHomography, homographyOffset, maxMatchingDistance, false);
correspondences_xIncreasing = findCorrespondences(xIncreasingTemplateList_x, xIncreasingTemplateList_y, xIncreasingNew, initialHomography, homographyOffset, maxMatchingDistance, false);
correspondences_yDecreasing = findCorrespondences(yDecreasingTemplateList_x, yDecreasingTemplateList_y, yDecreasingNew, initialHomography, homographyOffset, maxMatchingDistance, true);
correspondences_yIncreasing = findCorrespondences(yIncreasingTemplateList_x, yIncreasingTemplateList_y, yIncreasingNew, initialHomography, homographyOffset, maxMatchingDistance, true);

allCorrespondences = [correspondences_xDecreasing, correspondences_xIncreasing, correspondences_yDecreasing, correspondences_yIncreasing];

updatedHomography = updateHomography(initialHomography, allCorrespondences, updateType);

numTemplatePixelsMatched = binaryTracker_verifyTrack(...
  updatedHomography, verifyRange,...
  xDecreasingTemplateList_x, xDecreasingTemplateList_y, yDecreasingTemplateList_x, yDecreasingTemplateList_y, xIncreasingTemplateList_x, xIncreasingTemplateList_y, yIncreasingTemplateList_x, yIncreasingTemplateList_y,...
  xDecreasingNew, xIncreasingNew, yDecreasingNew, yIncreasingNew);
% keyboard

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

        if size(A,1) < 8
            updatedHomography = initialHomography;
            return;
        end
        
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






