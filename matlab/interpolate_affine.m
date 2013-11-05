% function output = interpolate_affine(inputImage, homography, outputImageSize)

% homography can be either 2x3 or 3x3, though only the first two rows are
% used. The x-coordinate comes first.
% 
% homography warps the input image coordinates to the template coordinates

% inputImage = zeros([5,5]); for y=1:5 inputImage(y,:)=y*(1:5); end
% output = interpolate_affine(inputImage, [2,1.5,0;.1,2,1.5], zeros(13,11));

function output = interpolate_affine(inputImage, homography, template)
DISPLAY_PLOTS = true;
% DISPLAY_PLOTS = false;

homography = homography(1:2, :);

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

indexClockwiseToLeftmost = mod(leftmostIndex, 4) + 1;
indexTwiceClockwiseToLeftmost = mod(indexClockwiseToLeftmost, 4) + 1;

indexCounterclockwiseToLeftmost = mod(leftmostIndex-2, 4) + 1;
indexTwiceCounterclockwiseToLeftmost = mod(indexCounterclockwiseToLeftmost-2, 4) + 1;

if warpedPoints(2,indexCounterclockwiseToLeftmost) > warpedPoints(2,leftmostIndex)
    minIndexes = updateMinimums(warpedPoints(:,leftmostIndex),...
        warpedPoints(:,indexCounterclockwiseToLeftmost),...
        minIndexes,...
        size(template));
    
    if warpedPoints(2,indexTwiceCounterclockwiseToLeftmost) > warpedPoints(2,indexCounterclockwiseToLeftmost)
        minIndexes = updateMinimums(warpedPoints(:,indexCounterclockwiseToLeftmost),...
            warpedPoints(:,indexTwiceCounterclockwiseToLeftmost),...
            minIndexes,...
            size(template));
    end
    
%     x = warpedPoints(1,leftmostIndex);
%     
%     if abs(warpedPoints(1,leftmostIndex) - warpedPoints(1,indexCounterclockwiseToLeftmost)) < .0001
%         dx = 0;
%     else
%         dx = (warpedPoints(1,leftmostIndex) - warpedPoints(1,indexCounterclockwiseToLeftmost)) /...
%              (warpedPoints(2,leftmostIndex) - warpedPoints(2,indexCounterclockwiseToLeftmost));
%     end
%     
%     minY = warpedPoints(2,leftmostIndex);
%     minYRounded = min(size(template,1)-1, max(0, ceil(minY)));
%     
%     maxY = warpedPoints(2,indexCounterclockwiseToLeftmost);
%     maxYRounded = min(size(template,1)-1, max(0, floor(maxY)));
%     
%     x = x + dx * (minYRounded-minY);
%     
%     for y = minYRounded:maxYRounded
%         minIndexes(y+1) = x;
%         x = x + dx;
%     end
%     
%     if warpedPoints(2,indexTwiceCounterclockwiseToLeftmost) > warpedPoints(2,indexCounterclockwiseToLeftmost) &&...
%             (maxY+1) < size(template,1)
%         
%         x = warpedPoints(1,indexCounterclockwiseToLeftmost);
%         if abs(warpedPoints(1,indexCounterclockwiseToLeftmost) - warpedPoints(1,indexTwiceCounterclockwiseToLeftmost)) < .0001
%             dx = 0;
%         else
%             dx = (warpedPoints(1,indexCounterclockwiseToLeftmost) - warpedPoints(1,indexTwiceCounterclockwiseToLeftmost)) /...
%                  (warpedPoints(2,indexCounterclockwiseToLeftmost) - warpedPoints(2,indexTwiceCounterclockwiseToLeftmost));
%         end
%         
%         minY = warpedPoints(2,indexCounterclockwiseToLeftmost);
%         minYRounded = min(size(template,1)-1, max(0, ceil(minY)));
%         
%         maxY = warpedPoints(2,indexTwiceCounterclockwiseToLeftmost);
%         maxYRounded = min(size(template,1)-1, max(0, floor(maxY)));
%         
%         x = x + dx * (minYRounded-minY);
%         for y = minYRounded:maxYRounded
%             minIndexes(y+1) = x;
%             x = x + dx;
%         end
%     end
end
    
keyboard

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

        x = x + dx * (minYRounded-minY);

        for y = minYRounded:maxYRounded
            minIndexes(y+1) = x;
            x = x + dx;
        end
    end
end % function updateMinimums()


