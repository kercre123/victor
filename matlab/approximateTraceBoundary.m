
% component is Nx3, for each 1D component as [y, xStart, xEnd]

% component = [200, 102, 102;
%              200, 104, 105;
%              201, 102, 105;
%              201, 108, 109;
%              202, 102, 109;
%              203, 100, 105;
%              204, 100, 105;
%              205, 104, 109;
%              206, 100, 105;
%              207, 100, 101;
%              207, 103, 104;
%              208, 100, 101;
%              208, 103, 104;
%              209, 100, 101;
%              209, 103, 104;
%              210, 100, 101;
%              210, 103, 104];

function boundary = approximateTraceBoundary(component)

assert(BlockMarker2D.UseOutsideOfSquare, ...
    'You need to set constant property BlockMarker2D.UseOutsideOfSquare = true to use this method.');

coordinate_top = min(component(:,1), [], 1);
coordinate_bottom = max(component(:,1), [], 1);
coordinate_left = min(component(:,2), [], 1);
coordinate_right = max(component(:,3), [], 1);

component(:, 1) = component(:, 1) - coordinate_top + 1;
component(:, 2:3) = component(:, 2:3) - coordinate_left + 1;

height = coordinate_bottom - coordinate_top + 1;
width = coordinate_right - coordinate_left + 1;

edge_top = inf * ones(width, 1);
edge_bottom = -inf * ones(width, 1);
edge_left = inf * ones(height, 1);
edge_right = -inf * ones(height, 1);

% 1. Compute the extreme pixels of the components, on each edge
for iSubComponent = 1:size(component, 1)
    xStart = component(iSubComponent, 2);
    xEnd = component(iSubComponent, 3);
    y = component(iSubComponent, 1);
    
    edge_top(xStart:xEnd) = min(edge_top(xStart:xEnd), y);
    edge_bottom(xStart:xEnd) = max(edge_top(xStart:xEnd), y);
    
    edge_left(y) = min(edge_left(y), xStart);
    edge_right(y) = max(edge_right(y), xEnd);
end

if ~isempty(find(isinf(edge_top), 1)) || ~isempty(find(isinf(edge_bottom), 1)) || ~isempty(find(isinf(edge_left), 1)) || ~isempty(find(isinf(edge_right), 1))
    disp('This should only happen if the component is buggy, but it should probably be either detector or corrected for');
    keyboard
end

% Correct for buggy components
edge_left(isinf(edge_left)) = 1;
edge_right(isinf(edge_right)) = 1;
edge_top(isinf(edge_top)) = 1;
edge_bottom(isinf(edge_bottom)) = 1;

boundary = zeros(0, 2);

% 2. Go through the right edge, from top to bottom. Add each to the
% boundary. If two right edge pixels are not adjacent, draw a line between
% them. The boundary will be Manhattan (4-connected) style
boundary(end+1, :) = [edge_right(1), 1];
for y = 2:height
    % Draw the horizontal line between the previous and current right edge
    if edge_right(y) > edge_right(y-1)
        lineWidth = edge_right(y) - edge_right(y-1);
        newBoundary = zeros(lineWidth, 2);
        newBoundary(:,1) = edge_right(y-1):(edge_right(y)-1);
        newBoundary(:,2) = y;
        boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
    elseif edge_right(y-1) > edge_right(y)
        lineWidth = edge_right(y-1) - edge_right(y);
        newBoundary = zeros(lineWidth, 2);
        newBoundary(:,1) = (edge_right(y-1)-1):-1:edge_right(y);
        newBoundary(:,2) = y-1;
        boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
    end
    
    boundary(end+1, :) = [edge_right(y), y];
end

% 3. Make a bridge from the bottomost right to the bottomost left. Make the
% bridge using the bottom edge pixels. Note that this this loop should by
% definition start at a pixel that is both a bottom and a right (I don't see a way this can't be true).
for x = edge_right(end):-1:(edge_left(end)+1)
    if edge_bottom(x) > edge_bottom(x-1)
        lineHeight = edge_bottom(x) - edge_bottom(x-1);
        newBoundary = zeros(lineHeight, 2);
        newBoundary(:,1) = x;
        newBoundary(:,2) = (edge_bottom(x)-1):-1:edge_bottom(x-1);
        boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
    elseif edge_bottom(x-1) > edge_bottom(x)
        lineHeight = edge_bottom(x-1) - edge_bottom(x);
        newBoundary = zeros(lineHeight, 2);
        newBoundary(:,1) = x-1;
        newBoundary(:,2) = edge_bottom(x):(edge_bottom(x-1)-1);
        boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
    end
    
    boundary(end+1, :) = [x-1, edge_bottom(x-1)];
end

% 4. Go through the left edge, from bottom to top.
for y = (height-1):-1:1
    % Draw the horizontal line between the previous and current left edge
    if edge_left(y) > edge_left(y+1)
        lineWidth = edge_left(y) - edge_left(y+1);
        newBoundary = zeros(lineWidth, 2);
        newBoundary(:,1) = (edge_left(y+1)+1):(edge_left(y));
        newBoundary(:,2) = y+1;
        boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
    elseif edge_left(y+1) > edge_left(y)
        lineWidth = edge_left(y+1) - edge_left(y);
        newBoundary = zeros(lineWidth, 2);
        newBoundary(:,1) = edge_left(y+1):-1:(edge_left(y)+1);
        newBoundary(:,2) = y;
        boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
    end
    
    boundary(end+1, :) = [edge_left(y), y];
end

% 5. Make a bridge from the topmost left pixel to the topmost right.
for x = (edge_left(1)+1):edge_right(1)
    if edge_top(x) > edge_top(x-1)
        lineHeight = edge_top(x) - edge_top(x-1);
        newBoundary = zeros(lineHeight, 2);
        newBoundary(:,1) = x-1;
        newBoundary(:,2) = (edge_top(x-1)+1):edge_top(x);
        boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
    elseif edge_top(x-1) > edge_top(x)
        lineHeight = edge_top(x-1) - edge_top(x);
        newBoundary = zeros(lineHeight, 2);
        newBoundary(:,1) = x;
        newBoundary(:,2) = edge_top(x-1):-1:(edge_top(x)+1);
        boundary((end+1):(end+size(newBoundary,1)), :) = newBoundary;
    end
    
    boundary(end+1, :) = [x, edge_top(x)];
end

boundary(:,1) = boundary(:,1) + coordinate_left - 1;
boundary(:,2) = boundary(:,2) + coordinate_top - 1;

boundary = fliplr(boundary);