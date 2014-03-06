% function binaryTracker_binarizeWithAutomata()

% im1 = imread('C:/Anki/systemTestImages/cozmo_date2014_01_29_time11_41_05_frame0.png');
% im2 = imread('C:/Anki/systemTestImages/cozmo_date2014_01_29_time11_41_05_frame10.png');
% im3 = imread('C:/Anki/systemTestImages/cozmo_date2014_01_29_time11_41_05_frame49.png');

% processingSize = [240,320];

% [xDecreasingImage, xIncreasingImage, yDecreasingImage, yIncreasingImage] = binaryTracker_binarizeWithAutomata(imresize(im,processingSize), 100, 3);

% Minima = Decreasing 
% Maxima = Increasing

% The value of the output images is the number of pixels that should be
% subtracted from the integer position of any nonzero pixel. For example,
% if xDecreasingImage(50,50)=0.4, then the true position is (49.6,50.0)
function [xDecreasingImage, xIncreasingImage, yDecreasingImage, yIncreasingImage] = binaryTracker_binarizeWithAutomata(image, grayvalueThreshold, minComponentWidth)

assert(grayvalueThreshold >=0 && grayvalueThreshold <= 255);

xDecreasingImage = zeros(size(image));
xIncreasingImage = zeros(size(image));

yDecreasingImage = zeros(size(image));
yIncreasingImage = zeros(size(image));

% states:
% 1. on white
% 2. on black

%
% Detect horizontal minima and maxima
%

for y = 1:size(image,1)
    % Is the first pixel white or black? (probably noisy, but that's okay)
    if image(y,1) > grayvalueThreshold
        curState = 1;
    else
        curState = 2;
    end

    lastSwitchX = 1;
    x = 1;
    while x <= size(image,2)
        if curState == 1
            while (x <= size(image,2)) && (image(y,x) > grayvalueThreshold)
                x = x + 1;
            end

            curState = 2;

            if x < size(image,2)
                componentWidth = x - lastSwitchX;

                if componentWidth >= minComponentWidth
                    lastDiff = image(y,x-1) - grayvalueThreshold;
                    curDiff = grayvalueThreshold - image(y,x);
                    xDecreasingImage(y,x) = curDiff / (lastDiff+curDiff);
                end

                lastSwitchX = x;
            end
        else
            while (x <= size(image,2)) && (image(y,x) < grayvalueThreshold)
                x = x + 1;
            end

            curState = 1;

            if x < size(image,2)
                componentWidth = x - lastSwitchX;

                if componentWidth >= minComponentWidth
                    lastDiff = grayvalueThreshold - image(y,x-1);
                    curDiff = image(y,x) - grayvalueThreshold;
                    xIncreasingImage(y,x) = lastDiff / (lastDiff+curDiff);
                end

                lastSwitchX = x;
            end
        end

        x = x + 1;
    end
end % for y = 1:size(image,1)

%
% Detect vertical minima and maxima
%

for x = 1:size(image,2)
    % Is the first pixel white or black? (probably noisy, but that's okay)
    if image(1,x) > grayvalueThreshold
        curState = 1;
    else
        curState = 2;
    end

    lastSwitchY = 1;
    y = 1;
    while y <= size(image,1)
        if curState == 1
            while (y <= size(image,1)) && (image(y,x) > grayvalueThreshold)
                y = y + 1;
            end

            curState = 2;

            if y < size(image,1)
                componentWidth = y - lastSwitchY;

                if componentWidth >= minComponentWidth
                    lastDiff = image(y-1,x) - grayvalueThreshold;
                    curDiff = grayvalueThreshold - image(y,x);
                    yDecreasingImage(y,x) = curDiff / (lastDiff+curDiff);
                end

                lastSwitchY = y;
            end
        else
            while (y <= size(image,1)) && (image(y,x) < grayvalueThreshold)
                y = y + 1;
            end

            curState = 1;

            if y < size(image,1)
                componentWidth = y - lastSwitchY;

                if componentWidth >= minComponentWidth
                    lastDiff = grayvalueThreshold - image(y-1,x);
                    curDiff = image(y,x) - grayvalueThreshold;
                    yIncreasingImage(y,x) = lastDiff / (lastDiff+curDiff);
                end

                lastSwitchY = y;
            end
        end

        y = y + 1;
    end
end % for y = 1:size(image,1)



