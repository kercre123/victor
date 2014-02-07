% function lucasKandeBinary_binarizeWithAutomata()

% im = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_0.png');
% im = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_10.png');
% im = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_49.png');

% processingSize = [240,320];

% [xMinima1Image, xMaxima1Image, yMinima1Image, yMaxima1Image] = lucasKandeBinary_binarizeWithAutomata(imresize(im,processingSize), 100, 3);

function [xMinima1Image, xMaxima1Image, yMinima1Image, yMaxima1Image] = lucasKandeBinary_binarizeWithAutomata(image, grayvalueThreshold, minComponentWidth)

assert(grayvalueThreshold >=0 && grayvalueThreshold <= 255); 

xMinima1Image = zeros(size(image));
xMaxima1Image = zeros(size(image));

yMinima1Image = zeros(size(image));
yMaxima1Image = zeros(size(image));

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
            
            if x ~= size(image,2)
                componentWidth = x - lastSwitchX;
                
                if componentWidth >= minComponentWidth
                    xMinima1Image(y,x) = 1;
                end
                
                lastSwitchX = x;
            end
        else
            while (x <= size(image,2)) && (image(y,x) < grayvalueThreshold)
                x = x + 1;
            end
            
            curState = 1;
            
            if x ~= size(image,2)
                componentWidth = x - lastSwitchX;
                
                if componentWidth >= minComponentWidth
                    xMaxima1Image(y,x) = 1;
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
            
            if y ~= size(image,1)
                componentWidth = y - lastSwitchY;
                
                if componentWidth >= minComponentWidth
                    yMinima1Image(y,x) = 1;
                end
                
                lastSwitchY = y;
            end
        else
            while (y <= size(image,1)) && (image(y,x) < grayvalueThreshold)
                y = y + 1;
            end
            
            curState = 1;
            
            if y ~= size(image,1)
                componentWidth = y - lastSwitchY;
                
                if componentWidth >= minComponentWidth
                    yMaxima1Image(y,x) = 1;
                end
                
                lastSwitchY = y;
            end
        end
        
        y = y + 1;
    end    
end % for y = 1:size(image,1)



