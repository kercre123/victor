
% function [components2d, regionMap1d] = approximateConnectedComponents(binaryImg, minimumComponentWidth, create1dRegionMap)

% Compute approximate 4-way connected components (will work for a ring
% structure, but not for a U structure.)

% binaryImg = [0,1,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0;
%              0,0,0,1,1,1,0,0,0,0,1,1,0,1,1,1,0;
%              0,0,0,0,0,1,1,0,0,1,1,1,0,1,1,0,0;
%              0,0,0,0,0,0,1,1,1,1,0,0,1,1,1,0,0;
%              0,0,0,0,0,0,0,1,1,0,0,1,1,0,1,1,1];
% components2d = approximateConnectedComponents(binaryImg, 2);
% [components2d, regionMap1d] = approximateConnectedComponents(binaryImg, 3, true);

function [components2d, regionMap1d] = approximateConnectedComponents(binaryImg, minimumComponentWidth, create1dRegionMap, eightConnected)

% 1. Compute 1d connected components
% 2. Check current 1d components with previous components. Put matches in
% global list
% 3. Put current 1d components in previous-row list

components2d = cell(0,0);
previousComponents1d = zeros(0,3);
num2dComponents = 0;

if ~exist('create1dRegionMap', 'var')
    create1dRegionMap = false;
end

if ~exist('eightConnected', 'var')
    eightConnected = false;
end

if create1dRegionMap
    regionMap1d = zeros(size(binaryImg));
end

num1dComponents = 0;
sum1dComponentLength = 0;

for y = 1:size(binaryImg, 1)
    currentComponents1d = extract1dComponents(binaryImg(y,:), minimumComponentWidth);

    if create1dRegionMap
        for i = 1:size(currentComponents1d,1)
            regionMap1d(y, currentComponents1d(i, 1):currentComponents1d(i, 2)) = 1;
            num1dComponents = num1dComponents + 1;
            sum1dComponentLength = sum1dComponentLength + (currentComponents1d(i, 2)-currentComponents1d(i, 1)+1);
        end
    end
    
    [components2d, num2dComponents, previousComponents1d] = ...
        addTo2dComponents(currentComponents1d, previousComponents1d, components2d, y, num2dComponents, eightConnected);
end

if create1dRegionMap
    disp(sprintf('%d 1d components found. Average length: %f. Total bytes needed (at 16-bit-precision): %d\n', num1dComponents, sum1dComponentLength/num1dComponents, num1dComponents*4*2));
end

%     keyboard

end % function componentIndexes = approximateConnectedComponents(binaryImg)


% currentComponents1d components is an array of format [xStart, xEnd];
% previousComponents1d components is an array of format [xStart, xEnd, 2dIndex];    
function [components2d, num2dComponents, newPreviousComponents1d] =...
    addTo2dComponents(currentComponents1d, previousComponents1d, components2d, currentRow, num2dComponents, eightConnected)

    newPreviousComponents1d = zeros(size(currentComponents1d,1), 3);

    if eightConnected
        extraPixelsAllowed = 1;
    else
        extraPixelsAllowed = 0;
    end
    
    for iCurrent = 1:size(currentComponents1d,1)
        foundMatch = false;
        for iPrevious = 1:size(previousComponents1d, 1)
            % The current component matches the previous one if 
            % 1. the previous start is less-than-or-equal the current end, and
            % 2. the previous end is greater-than-or-equal the current start
            if previousComponents1d(iPrevious, 1) <= (currentComponents1d(iCurrent, 2)+extraPixelsAllowed) &&... 
               previousComponents1d(iPrevious, 2) >= (currentComponents1d(iCurrent, 1)-extraPixelsAllowed) 
           
                foundMatch = true;
                componentId = previousComponents1d(iPrevious, 3);
                components2d{componentId+1}(end+1,:) = [currentRow, currentComponents1d(iCurrent,1:2)]; % [y, xStart, xEnd]
                newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), componentId];
                
                break;
            end
        end
        
        if ~foundMatch
            componentId = num2dComponents;
            newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), componentId];
            components2d{end+1} = [currentRow, currentComponents1d(iCurrent, 1:2)]; %#ok<AGROW> % [y, xStart, xEnd]
            num2dComponents = num2dComponents + 1;
        end
    end    
end % function newComponents2d = addTo2dComponents()

function components1d = extract1dComponents(binaryImgRow, minimumComponentWidth)

    components1d = zeros(0,2);

    if binaryImgRow(1) == 0
        onComponent = false;
        componentStart = -1;
    else
        onComponent = true;
        componentStart = 1;
    end

    for x = 2:size(binaryImgRow, 2)
        if onComponent
            if binaryImgRow(x) == 0
                componentWidth = x - componentStart;
                if componentWidth >= minimumComponentWidth
                    components1d(end+1, :) = [componentStart, x-1]; %#ok<AGROW>
                end
                onComponent = false;
            end
        else
            if binaryImgRow(x) ~= 0
                componentStart = x;
                onComponent = true;
            end
        end
    end
    
    if onComponent
        componentWidth = x - componentStart + 1;
        if componentWidth >= minimumComponentWidth
            components1d(end+1, :) = [componentStart, x];
        end
    end
    
end % function components1d = extract1dComponents(binaryImgRow)