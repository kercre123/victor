% function output = lucasKanade_affineRowLimits(inputImageQuad, homography, templateSize, debugDisplay)

% homography can be either 2x3 or 3x3, though only the first two rows are
% used. The x-coordinate comes first.
%
% homography warps the input image coordinates to the template coordinates

% lucasKanade_affineRowLimits([0,0;3,1;2,4;-1,3], [2,1.5,0;.1,2,1.5], [13,11], true);
% for i=1:100 lucasKanade_affineRowLimits(4*[0,0;3,1;2,4;-1,3], 5*(2*rand([2,3])-1), [13,11], true); pause(); end

function [minIndexes, maxIndexes] = lucasKanade_affineRowLimits(inputImageQuad, homography, templateSize, debugDisplay)

numPixelsToReduce = 1.0;

if ~exist('debugDisplay', 'var')
    debugDisplay = false;
end

homography = homography(1:2, :);

% Warp the inputImageQuad corners to the template coordinates
inputImageQuadSqueezed = squeezeQuadrilateral(inputImageQuad, numPixelsToReduce);

inputImageQuadSqueezed(:,3) = 1;
warpedPoints = homography*inputImageQuadSqueezed';

if debugDisplay
    figure(2);
    templateCornerPoints = [0,               0              ;
                            templateSize(2), 0              ;
                            templateSize(2), templateSize(1);
                            0,               templateSize(1)]';

    inputImageQuad(:,3) = 1;
    warpedPoints_nonSqueezed = homography*inputImageQuad';

    hold off;
    plot(templateCornerPoints(1,[1:4,1]), -templateCornerPoints(2,[1:4,1]), 'k--');
    hold on;
    plot(warpedPoints_nonSqueezed(1,[1:4,1]), -warpedPoints_nonSqueezed(2,[1:4,1]), 'y--');
    plot(warpedPoints(1,[1:4,1]), -warpedPoints(2,[1:4,1]), 'b--');
    axis equal
    hold off
end

[~, leftmostIndex] = min(warpedPoints(1,:));
[~, rightmostIndex] = max(warpedPoints(1,:));

minIndexes = Inf * ones(templateSize(1),1);
maxIndexes = -Inf * ones(templateSize(1),1);

%check if the quad is flipped
% TODO: make simpler
centroid = mean(warpedPoints, 2);
anglesFromCenter = atan2(warpedPoints(2,:)'-repmat(centroid(2,:), [1,4])', warpedPoints(1,:)'-repmat(centroid(1,:), [1,4])');

% Find the minimum angle to the left of the current one
inds = (anglesFromCenter<0);
anglesFromCenter(inds) = anglesFromCenter(inds) + 2*pi;

if anglesFromCenter(1) == max(anglesFromCenter)
    [~,ind] = min(anglesFromCenter);
else
    angleDifferences = anglesFromCenter(2:4) - anglesFromCenter(1);
    angleDifferences(angleDifferences<0) = Inf;
    [~,ind] = min(angleDifferences);

    ind = ind + 1;
end

assert(ind ~= 3); % I don't think this should ever happen

if ind == 2
    direction = 1;
else
    direction = -1;
end

if direction == 1
%     disp('normal');
    leftmost_clockwise1 = mod(leftmostIndex, 4) + 1;
    leftmost_clockwise2 = mod(leftmost_clockwise1, 4) + 1;

    leftmost_counter1 = mod(leftmostIndex-2, 4) + 1;
    leftmost_counter2 = mod(leftmost_counter1-2, 4) + 1;

    rightmost_clockwise1 = mod(rightmostIndex, 4) + 1;
    rightmost_clockwise2 = mod(rightmost_clockwise1, 4) + 1;

    rightmost_counter1 = mod(rightmostIndex-2, 4) + 1;
    rightmost_counter2 = mod(rightmost_counter1-2, 4) + 1;
else
%     disp('flipped');
    leftmost_clockwise1 = mod(leftmostIndex-2, 4) + 1;
    leftmost_clockwise2 = mod(leftmost_clockwise1-2, 4) + 1;

    leftmost_counter1 = mod(leftmostIndex, 4) + 1;
    leftmost_counter2 = mod(leftmost_counter1, 4) + 1;

    rightmost_clockwise1 = mod(rightmostIndex-2, 4) + 1;
    rightmost_clockwise2 = mod(rightmost_clockwise1-2, 4) + 1;

    rightmost_counter1 = mod(rightmostIndex, 4) + 1;
    rightmost_counter2 = mod(rightmost_counter1, 4) + 1;
end

% Compute the mins 

minIndexes = updateExtrema(warpedPoints(:,leftmostIndex),...
    warpedPoints(:,leftmost_counter1),...
    minIndexes, templateSize);

if warpedPoints(2,leftmost_counter2) > warpedPoints(2,leftmostIndex)
    minIndexes = updateExtrema(warpedPoints(:,leftmost_counter1),...
        warpedPoints(:,leftmost_counter2),...
        minIndexes, templateSize);
end

minIndexes = updateExtrema(warpedPoints(:,leftmost_clockwise1),...
    warpedPoints(:,leftmostIndex),...
    minIndexes, templateSize);

if warpedPoints(2,leftmostIndex) > warpedPoints(2,leftmost_clockwise2)
    minIndexes = updateExtrema(warpedPoints(:,leftmost_clockwise2),...
        warpedPoints(:,leftmost_clockwise1),...
        minIndexes, templateSize);
end

% Compute the maxes

maxIndexes = updateExtrema(warpedPoints(:,rightmostIndex),...
    warpedPoints(:,rightmost_clockwise1),...
    maxIndexes, templateSize);

if warpedPoints(2,rightmost_clockwise2) > warpedPoints(2,rightmostIndex)
    maxIndexes = updateExtrema(warpedPoints(:,rightmost_clockwise1),...
        warpedPoints(:,rightmost_clockwise2),...
        maxIndexes, templateSize);
end

maxIndexes = updateExtrema(warpedPoints(:,rightmost_counter1),...
    warpedPoints(:,rightmostIndex),...
    maxIndexes, templateSize);

if warpedPoints(2,rightmostIndex) > warpedPoints(2,rightmost_counter2)
    maxIndexes = updateExtrema(warpedPoints(:,rightmost_counter2),...
        warpedPoints(:,rightmost_counter1),...
        maxIndexes, templateSize);
end

assert(length(minIndexes) == length(maxIndexes));


for i = 1:length(minIndexes)
    if minIndexes(i) >= maxIndexes(i)
        minIndexes(i) = Inf;
        maxIndexes(i) = -Inf;
    end
end

minIndexes = max(0, minIndexes);
maxIndexes = min(templateSize(2), maxIndexes);

for i = 1:length(minIndexes)
    if minIndexes(i) >= maxIndexes(i)
        minIndexes(i) = Inf;
        maxIndexes(i) = -Inf;
    end
end

if debugDisplay
    for i = 1:length(minIndexes)
        hold on;
        plot([minIndexes(i),maxIndexes(i)], [-i+0.5,-i+0.5], 'g');
        axis equal
    end
end % if debugDisplay

end % function interpolate_affine()

function extremaIndexes = updateExtrema(topPoint, bottomPoint, extremaIndexes, templateSize)
    if bottomPoint(2) > topPoint(2)
        x = topPoint(1);

        if abs(topPoint(1) - bottomPoint(1)) < .0001
            dx = 0;
        else
            dx = (topPoint(1) - bottomPoint(1)) /...
                 (topPoint(2) - bottomPoint(2));
        end

        minY = topPoint(2);
        minYRounded = min(templateSize(1)-0.5, max(0.5, ceil(minY-0.5)+0.5));

        % I think this is safe as +0.5, versus a ceil, but think about this
%         if ceil(minY) >= templateSize(1)
        if (minY+0.5) >= templateSize(1)
            return;
        end

        maxY = bottomPoint(2);
        maxYRounded = min(templateSize(1)-0.5, max(0.5, floor(maxY-0.5)+0.5));

        if maxY < 0.5
            return;
        end

        % x was first computed at the floating-point coordinate of the
        % corner. Update it to the nearest integer y coordinate
        x = x + dx * (minYRounded-minY);

        for y = minYRounded:maxYRounded
            extremaIndexes(y+0.5) = x;
            x = x + dx;
        end
    end
end % function updateExtrema()

function quad = squeezeQuadrilateral(quad, numPixelsToReduce)

    assert(size(quad,1) == 4);

    centroid = mean(quad, 1);

    for i = 1:4
        offset = quad(i,:) - centroid;
        offsetMagnitude = sqrt(offset(1)^2+offset(2)^2);
        normalizedOffset = offset / offsetMagnitude;

        quad(i,1) = quad(i,1) - numPixelsToReduce*normalizedOffset(1);
        quad(i,2) = quad(i,2) - numPixelsToReduce*normalizedOffset(2);
    end

end % function quad = squeezeQuadrilateral()










