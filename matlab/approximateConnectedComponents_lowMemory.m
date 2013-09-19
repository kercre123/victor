
% function componentIndexes = approximateConnectedComponents()

% Compute approximate 4-way connected components (will work for a ring
% structure, but not for a U structure.)

% binaryImg = [0,1,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0;
%              0,0,0,1,1,1,0,0,0,0,1,1,0,1,1,1,0,0;
%              0,0,0,0,0,1,1,0,0,1,1,1,0,1,1,0,0,0;
%              0,0,0,0,0,0,1,1,1,1,0,0,1,1,1,0,1,1;
%              0,0,0,0,0,0,0,1,1,0,0,1,1,0,1,1,1,0];
% [components2d, components2d_packed, components2d_unsorted] = approximateConnectedComponents_lowMemory(binaryImg, 2, 0);

% binaryImg = [1,1,1,1,1,1,1,1,1,1;
%              1,0,0,0,0,0,0,0,0,0;
%              1,0,1,1,1,1,1,1,1,1;
%              1,0,1,0,0,0,0,0,0,1;
%              1,0,1,0,1,1,1,1,0,1;
%              1,0,1,0,1,0,0,1,0,1;
%              1,0,1,0,0,0,0,1,0,1;
%              1,0,1,1,1,1,1,1,0,1;
%              1,0,0,0,0,0,0,0,0,1;
%              1,1,1,1,1,1,1,1,1,1];
% [components2d, components2d_packed, components2d_unsorted] = approximateConnectedComponents_lowMemory(binaryImg, 1, 0)
% [components2d, components2d_packed, components2d_unsorted] = approximateConnectedComponents_lowMemory(binaryImg, 1, 1)

function [components2d, components2d_packed, components2d_unsorted] = approximateConnectedComponents_lowMemory(binaryImg, minComponentWidth, maxSkipDistance)

MAX_1D_COMPONENTS = floor(size(binaryImg,2)/5);
MAX_2D_COMPONENTS = 50000;

MAX_EQUIVALENT_ITERATIONS = 3;

% 1. Compute 1d connected components
% 2. Check current 1d components with previous components. Put matches in
% global list
% 3. Put current 1d components in previous-row list

components2d = zeros(MAX_2D_COMPONENTS, 4); % [componentId, y, xStart, xEnd]
numStored2dComponents = 0;

previousComponents1d = zeros(MAX_1D_COMPONENTS, 3);

numStored1dComponents = 0;

num1dComponents_previous = 0;

equivalentComponents = 1:MAX_2D_COMPONENTS;

for y = 1:size(binaryImg, 1)
    currentComponents1d = extract1dComponents(binaryImg(y,:), minComponentWidth, maxSkipDistance);
    num1dComponents_current = size(currentComponents1d,1);
    
    newPreviousComponents1d = zeros(num1dComponents_current, 3);

    for iCurrent = 1:num1dComponents_current
        foundMatch = false;
        for iPrevious = 1:num1dComponents_previous
            % The current component matches the previous one if 
            % 1. the previous start is less-than-or-equal the current end, and
            % 2. the previous end is greater-than-or-equal the current start
            if previousComponents1d(iPrevious, 1) <= currentComponents1d(iCurrent, 2) &&... 
               previousComponents1d(iPrevious, 2) >= currentComponents1d(iCurrent, 1) 
           
                if ~foundMatch
                    % If this is the first match we've found, add this 1d
                    % component to components2d, using that previous component's id.
                    foundMatch = true;
                    componentId = previousComponents1d(iPrevious, 3);
                    components2d(numStored1dComponents+1, :) = [componentId, y, currentComponents1d(iCurrent, 1:2)]; % [componentId, y, xStart, xEnd]
                    numStored1dComponents = numStored1dComponents + 1;

                    newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), componentId];
                else
                    % Update the lookup table for all of:
                    % 1. The first matching component
                    % 2. This iPrevious component.
                    % 3. The newPreviousComponents1d(iCurrent) component
                    previousId = previousComponents1d(iPrevious, 3);
                    minId = min(componentId, previousId); 
                    equivalentComponents(componentId) = min(equivalentComponents(componentId), minId);
                    equivalentComponents(previousId) = min(equivalentComponents(previousId), minId);
                    newPreviousComponents1d(iCurrent, 3) = minId;
                end
            end
        end % for iPrevious = 1:num1dComponents_previous
        
        % If none of the previous components matched, start a new id, equal
        % to num2dComponents
        if ~foundMatch
            componentId = numStored2dComponents + 1;
            
            newPreviousComponents1d(iCurrent, :) = [currentComponents1d(iCurrent, 1:2), componentId];
            components2d(numStored1dComponents+1, :) = [componentId, y, currentComponents1d(iCurrent, 1:2)]; % [componentId, y, xStart, xEnd]
            
            numStored1dComponents = numStored1dComponents + 1;
            numStored2dComponents = numStored2dComponents + 1;
        end
    end % for iCurrent = 1:num1dComponents_current
    
    previousComponents1d = newPreviousComponents1d;
    num1dComponents_previous = size(previousComponents1d,1);
    
%     disp(components2d(1:numStored1dComponents, :));
%     disp(equivalentComponents(1:numStored2dComponents))
%     keyboard
end % for y = 1:size(binaryImg, 1)

% After all the initial 2d labels have been created, go through
% equivalentComponents, and update equivalentComponents internally
for iEquivalent = 1:MAX_EQUIVALENT_ITERATIONS
    changes = 0;
    for iComponent = 1:numStored2dComponents
        minNeighbor = equivalentComponents(iComponent);
        
        if equivalentComponents(minNeighbor) ~= minNeighbor
            while equivalentComponents(minNeighbor) ~= minNeighbor
                minNeighbor = equivalentComponents(minNeighbor);           
            end
            changes = changes + 1;
            equivalentComponents(iComponent) = minNeighbor;
        end
    end
    
    if changes == 0
        break;
    end       
end

% disp(components2d(1:numStored1dComponents, :));
% Replace the id of all 1d components with their minimum equivalent id
for iComponent = 1:numStored1dComponents
    components2d(iComponent, 1) = equivalentComponents(components2d(iComponent, 1));
end
components2d = components2d(1:numStored1dComponents, :);
% disp(components2d);

% Sort all 1D components by id, y, then x
components2d_unsorted = components2d;
components2d_packed = sortrows(components2d);

components2d = cell(1,0);
components2d{1} = components2d_packed(1,2:4);
previousComponent = components2d_packed(1,1);
for iComponent = 2:numStored1dComponents
    currentComponent = components2d_packed(iComponent,1);
    if currentComponent ~= previousComponent
        components2d{end+1} = components2d_packed(iComponent,2:4);
        previousComponent = currentComponent;
    else
        components2d{end}(end+1,:) = components2d_packed(iComponent,2:4);
    end
end

% keyboard

end % function componentIndexes = approximateConnectedComponents(binaryImg)
