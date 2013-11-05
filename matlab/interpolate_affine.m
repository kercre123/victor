% function output = interpolate_affine(inputImage, homography, outputImageSize)

% homography can be either 2x3 or 3x3, though only the first two rows are
% used. The x-coordinate comes first.
% 
% homography warps the input image coordinates to the template coordinates

% inputImage = zeros([5,5]); for y=1:5 inputImage(y,:)=y*(1:5); end
% output = interpolate_affine(inputImage, [2,1.5,0;.1,2,1.5], zeros(13,11));

function interpolate_affine(inputImage, homography, template)
DISPLAY_PLOTS = true;
% DISPLAY_PLOTS = false;

homography = homography(1:2, :)

% Warp the inputImage corners to the template coordinates
% Clockwise from the upper-left
inputCornerPoints = [0,                  0,                  1;
                     size(inputImage,2), 0,                  1;
                     size(inputImage,2), size(inputImage,1), 1;
                     0,                  size(inputImage,1), 1]';

warpedPoints = homography*inputCornerPoints;

if DISPLAY_PLOTS
    figure(1);
    templateCornerPoints = [0,                0,              ;
                            size(template,2), 0,              ;
                            size(template,2), size(template,1);
                            0,                size(template,1)]';
    hold off;
    plot(templateCornerPoints(1,[1:4,1]), -templateCornerPoints(2,[1:4,1]), 'b');
    hold on;
    plot(warpedPoints(1,[1:4,1]), -warpedPoints(2,[1:4,1]), 'r');
    axis equal
    hold off
end

[~, leftmostIndex] = min(warpedPoints(1,:));
[~, rightmostIndex] = max(warpedPoints(1,:));

minIndexes = Inf * ones(size(template,1),1);
maxIndexes = -Inf * ones(size(template,1),1);

leftmost_clockwise1 = mod(leftmostIndex, 4) + 1;
leftmost_clockwise2 = mod(leftmost_clockwise1, 4) + 1;

leftmost_counter1 = mod(leftmostIndex-2, 4) + 1;
leftmost_counter2 = mod(leftmost_counter1-2, 4) + 1;

rightmost_clockwise1 = mod(rightmostIndex, 4) + 1;
rightmost_clockwise2 = mod(rightmost_clockwise1, 4) + 1;

rightmost_counter1 = mod(rightmostIndex-2, 4) + 1;
rightmost_counter2 = mod(rightmost_counter1-2, 4) + 1;

% Compute the mins

minIndexes = updateMinimums(warpedPoints(:,leftmostIndex),...
    warpedPoints(:,leftmost_counter1),...
    minIndexes, size(template));

if warpedPoints(2,leftmost_counter2) > warpedPoints(2,leftmostIndex)
    minIndexes = updateMinimums(warpedPoints(:,leftmost_counter1),...
        warpedPoints(:,leftmost_counter2),...
        minIndexes, size(template));
end

minIndexes = updateMinimums(warpedPoints(:,leftmost_clockwise1),...
    warpedPoints(:,leftmostIndex),...
    minIndexes, size(template));

if warpedPoints(2,leftmostIndex) > warpedPoints(2,leftmost_clockwise2)
    minIndexes = updateMinimums(warpedPoints(:,leftmost_clockwise2),...
        warpedPoints(:,leftmost_clockwise1),...
        minIndexes, size(template));
end

% Compute the maxes

maxIndexes = updateMaximums(warpedPoints(:,rightmostIndex),...
    warpedPoints(:,rightmost_clockwise1),...
    maxIndexes, size(template));

if warpedPoints(2,rightmost_clockwise2) > warpedPoints(2,rightmostIndex)
    maxIndexes = updateMinimums(warpedPoints(:,rightmost_clockwise1),...
        warpedPoints(:,rightmost_clockwise2),...
        maxIndexes, size(template));    
end

maxIndexes = updateMaximums(warpedPoints(:,rightmost_counter1),...
    warpedPoints(:,rightmostIndex),...
    maxIndexes, size(template));

if warpedPoints(2,rightmostIndex) > warpedPoints(2,rightmost_counter2)
    maxIndexes = updateMinimums(warpedPoints(:,rightmost_counter2),...
        warpedPoints(:,rightmost_counter1),...
        maxIndexes, size(template));    
end

assert(length(minIndexes) == length(maxIndexes));

minIndexes = max(0, minIndexes);
maxIndexes = min(size(template,2), maxIndexes);

if DISPLAY_PLOTS
    for i = 1:length(minIndexes)
        hold on;
        plot([minIndexes(i),maxIndexes(i)], [-i+1,-i+1], 'g');        
        axis equal
    end
end % if DISPLAY_PLOTS
minIndexes
maxIndexes

% keyboard

end % function interpolate_affine()

function minIndexes = updateMinimums(topPoint, bottomPoint, minIndexes, templateSize)
    if bottomPoint(2) > topPoint(2)
        x = topPoint(1);

        if abs(topPoint(1) - bottomPoint(1)) < .0001
            dx = 0;
        else
            dx = (topPoint(1) - bottomPoint(1)) /...
                 (topPoint(2) - bottomPoint(2));
        end

        minY = topPoint(2);
        minYRounded = min(templateSize(1)-1, max(0, ceil(minY)));
        
        if ceil(minY) >= templateSize(1)
            return;
        end

        maxY = bottomPoint(2);
        maxYRounded = min(templateSize(1)-1, max(0, floor(maxY)));

        % x was first computed at the floating-point coordinate of the
        % corner. Update it to the nearest integer y coordinate
        x = x + dx * (minYRounded-minY);

        for y = minYRounded:maxYRounded
            minIndexes(y+1) = x;
            x = x + dx;
        end
    end
end % function updateMinimums()

function maxIndexes = updateMaximums(topPoint, bottomPoint, maxIndexes, templateSize)
    if bottomPoint(2) > topPoint(2)
        x = topPoint(1);
    
        if abs(topPoint(1) - bottomPoint(1)) < .0001
            dx = 0;
        else
            dx = (topPoint(1) - bottomPoint(1)) /...
                 (topPoint(2) - bottomPoint(2));
        end

        minY = topPoint(2);
        minYRounded = min(templateSize(1)-1, max(0, ceil(minY)));
        
        if ceil(minY) >= templateSize(1)
            return;
        end

        maxY = bottomPoint(2);
        maxYRounded = min(templateSize(1)-1, max(0, floor(maxY)));

        % x was first computed at the floating-point coordinate of the
        % corner. Update it to the nearest integer y coordinate
        x = x + dx * (minYRounded-minY);

        for y = minYRounded:maxYRounded
            maxIndexes(y+1) = x;
            x = x + dx;
        end
    end
end % function updateMaximums()


