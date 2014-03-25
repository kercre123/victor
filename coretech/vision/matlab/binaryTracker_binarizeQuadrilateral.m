
% function binaryTracker_binarizeQuadrilateral()

% Sometimes misses one pixel per line
 
% quad = [50,50;100,40;100,80;30,60];
% [xDecreasingImage, xIncreasingImage, yDecreasingImage, yIncreasingImage] = binaryTracker_binarizeQuadrilateral(quad, [240,320], true);
% imshows(xDecreasingImage, xIncreasingImage, yDecreasingImage, yIncreasingImage, 'maximize');
% for i=1:4 figure(i); hold on; plot(quad([1:4,1],1), quad([1:4,1],2)); end;

% for iteration = 1:100
%     quad = [rand(4,1)*320, rand(4,1)*240];
%     [xDecreasingImage, xIncreasingImage, yDecreasingImage, yIncreasingImage] = binaryTracker_binarizeQuadrilateral(quad, [240,320], true);
%     imshows(xDecreasingImage, xIncreasingImage, yDecreasingImage, yIncreasingImage, 'maximize');
%     for i=1:4 figure(i); hold on; plot(quad([1:4,1],1), quad([1:4,1],2)); end;
%     pause;
% end

function [xDecreasingImage, xIncreasingImage, yDecreasingImage, yIncreasingImage] = binaryTracker_binarizeQuadrilateral(quad, imageSize, isBlackInside)

xDecreasingImage = zeros(imageSize, 'uint8');
xIncreasingImage = zeros(imageSize, 'uint8');
yDecreasingImage = zeros(imageSize, 'uint8');
yIncreasingImage = zeros(imageSize, 'uint8');

% Sort all the corners
centerX = mean(quad(:,1));
centerY = mean(quad(:,2));

[thetas,~] = cart2pol(quad(:,1)-centerX, quad(:,2)-centerY);
[~,sortedIndexes] = sort(thetas);

newQuad = zeros(4,2);
for i = 1:4
    newQuad(i,:) = quad(sortedIndexes(i), :);
end
quad = newQuad;

% The last point is the first
quad(5,:) = quad(1,:);

for iCorner = 1:4
    dx = quad(iCorner+1, 1) - quad(iCorner, 1);
    dy = quad(iCorner+1, 2) - quad(iCorner, 2);

    addXDecreasing = false;
    addXIncreasing = false;
    addYDecreasing = false;
    addYIncreasing = false;

    % Which edges should be computed?
    if dx > 0
        if dy > 0
%             disp('1');
            if abs(dy) > abs(dx)
                addXIncreasing = true;
            else                
                addYDecreasing = true;
            end
        elseif dy < 0
%             disp('4');
            if abs(dy) > abs(dx)
                addXDecreasing = true;
            else
                addYDecreasing = true;
            end
        else % dy == 0
%             disp('5');
            addYDecreasing = true;
        end
    elseif dx < 0
        if dy > 0
%             disp('2');
            if abs(dy) > abs(dx)
                addXIncreasing = true;
            else
                addYIncreasing = true;
            end
        elseif dy < 0
%             disp('3 ');
            if abs(dy) > abs(dx)
                addXDecreasing = true;
            else
                addYIncreasing = true;
            end
        else % dy == 0
%             disp('6');
            addYIncreasing = true;
        end
    else % dx == 0
        if dy > 0
%             disp('8');
            addXIncreasing = true;
        elseif dy < 0
%             disp('7');
            addXDecreasing = true;
        else % dy == 0
            assert(false);
        end
    end

    % If the quad is white inside, swap all the increasing/decreasing flags
    if ~isBlackInside
        if addXDecreasing
            assert(~addXIncreasing);
            addXIncreasing = true;
            addXDecreasing = false;
        end

        if addXIncreasing
            assert(~addXDecreasing);
            addXDecreasing = true;
            addXIncreasing = false;
        end

        if addYDecreasing
            assert(~addYIncreasing);
            addYIncreasing = true;
            addYDecreasing = false;
        end

        if addYIncreasing
            assert(~addYDecreasing);
            addYDecreasing = true;
            addYIncreasing = false;
        end
    end

%     disp(sprintf('addXDecreasing:%d addXIncreasing:%d addYDecreasing:%d addYIncreasing:%d', addXDecreasing, addXIncreasing, addYDecreasing, addYIncreasing));

    numPixels = max(abs(dx), abs(dy)) + 1;

    if addXDecreasing
        xs = linspace(quad(iCorner, 1), quad(iCorner+1, 1), numPixels);
        ys = linspace(quad(iCorner, 2), quad(iCorner+1, 2), numPixels);
        for i = 1:numPixels
            xDecreasingImage(round(ys(i)), round(xs(i))) = 1;
        end
    end

    if addXIncreasing
        xs = linspace(quad(iCorner, 1), quad(iCorner+1, 1), numPixels);
        ys = linspace(quad(iCorner, 2), quad(iCorner+1, 2), numPixels);
        for i = 1:numPixels
            xIncreasingImage(round(ys(i)), round(xs(i))) = 1;
        end
    end

    if addYDecreasing
        xs = linspace(quad(iCorner, 1), quad(iCorner+1, 1), numPixels);
        ys = linspace(quad(iCorner, 2), quad(iCorner+1, 2), numPixels);
        for i = 1:numPixels
            yDecreasingImage(round(ys(i)), round(xs(i))) = 1;
        end
    end

    if addYIncreasing
        xs = linspace(quad(iCorner, 1), quad(iCorner+1, 1), numPixels);
        ys = linspace(quad(iCorner, 2), quad(iCorner+1, 2), numPixels);
        for i = 1:numPixels
            yIncreasingImage(round(ys(i)), round(xs(i))) = 1;
        end
    end
end % for iCorner = 1:4


