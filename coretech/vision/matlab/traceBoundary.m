
% function [boundary, boundaryImg] = traceBoundary(binaryImg, startPoint, initialDirection)

% May work incorrectly on an object that is less than two pixels thick
% If the start point is in a corner, the boundary may circle the object twice

% img = ones(10,10); img(4:6, 4:6) = 0;
% [boundary, boundaryImg] = traceBoundary(img, [3,7], 'n')

% img2 = ones(16,16); img2(6:8, 6:8) = 0; img2(4, 4:10) = 0; img2(3:10, 10) = 0; img2(10, 4:10) = 0; img2(3:10, 4) = 0;
% [boundary2, boundaryImg2] = traceBoundary(img2, [7,9], 'n')

% img3 = ones(16,16); img3(6:8, 6:8) = 0; img3(3, 4:10) = 0; img3(3:10, 11) = 0; img3(11, 4:10) = 0; img3(3:10, 3) = 0;
% [boundary3, boundaryImg3] = traceBoundary(img3, [7,9], 'n')

% img4 = ones(16,16); img4(6:8, 6:8) = 0; img4(3, 4:10) = 0; img4(3:10, 11) = 0; img4(11, 4:10) = 0; img4(3:10, 3) = 0; img4(4:5, 7) = 0;
% [boundary4, boundaryImg4] = traceBoundary(img4, [7,9], 'n')

% img5 = ones(16,16); img5(6:8, 6:8) = 0; img5(3, 4:11) = 0; img5(3:10, 11) = 0; img5(11, 3:11) = 0; img5(3:10, 3) = 0; img5(4:5, 7) = 0;
% [boundary5, boundaryImg5] = traceBoundary(img5, [7,9], 'n')

function [boundary, boundaryImg] = traceBoundary(binaryImg, startPoint, initialDirection)

validDirectionStrings = {'n','ne','e','se','s','sw','w','nw'};
initialDirection = find(strcmpi(initialDirection, validDirectionStrings) == 1, 1) - 1;

if isempty(initialDirection)
    return;
end

if nargout == 2
    boundaryImg = zeros(size(binaryImg));
end

checkForOutOfBounds = false;

if ~checkForOutOfBounds
    % The boundaries are zero, which means we don't have to check for out of bounds
    binaryImg([1,end],:) = 0;
    binaryImg(:, [1,end]) = 0;
end

allDirections = 0:7;
dx = [0, 1, 1, 1, 0, -1, -1, -1];
dy = [-1, -1, 0, 1, 1, 1, 0, -1];

curDirection = initialDirection;

boundary = [startPoint(1), startPoint(2)];
curPoint = [startPoint(1), startPoint(2)];

if nargout == 2
    curBoundaryIndex = 1;
    boundaryImg(curPoint(1), curPoint(2)) = curBoundaryIndex;
end

while size(boundary,1) < 5000
    % Search counter-clockwise until we find a 0
   zeroDirection = findNewDirection(binaryImg, curPoint, curDirection, 0);
   if zeroDirection == -1
       break;
   end
   
    % Search counter-clockwise until we find a 1
    zeroDirectionPlusOne = zeroDirection + 1;
    if zeroDirectionPlusOne == 8
        zeroDirectionPlusOne = 0;
    end
    
    oneDirection = findNewDirection(binaryImg, curPoint, zeroDirectionPlusOne, 1);
        
    newPoint = [curPoint(1)+dy(oneDirection+1), curPoint(2)+dx(oneDirection+1)];
        
    curPoint = newPoint;
    boundary(end+1, :) = newPoint;
    if nargout == 2
        curBoundaryIndex = curBoundaryIndex + 1;
        boundaryImg(newPoint(1), newPoint(2)) = curBoundaryIndex;
    end
    
    % If the new point is the same as the last point (or second-to-last to detect oscillations), quit.
    if sum(newPoint == boundary(end-1,:)) == 2 ...
        || (size(boundary,1)>2 && sum(newPoint == boundary(end-2,:)) == 2)
        break;
    end
    
    % If the new point is the same as the first point, quit.
%     keyboard
    if sum(newPoint == boundary(1,:)) == 2 || ...
        (size(boundary,1)>2 && sum(abs(newPoint-boundary(1,:))) <= 1)
%         keyboard
        break;
    end
    
    if checkForOutOfBounds
        % If the new point is out of bounds, quit.
        if newPoint(1)<=1 || newPoint(2)<=1 ||...
           newPoint(1)>=size(binaryImg,1) || newPoint(2)>=size(binaryImg,2)
            break;
        end
    end
end % while true

if size(boundary,1) > 4000
    keyboard;
end

end % function boundary = traceBoundary(binaryImg, startPoint, initialDirection)

function newDirection = findNewDirection(img, curPoint, curDirection, value)
    allDirections = 0:7;
    dx = [0, 1, 1, 1, 0, -1, -1, -1];
    dy = [-1, -1, 0, 1, 1, 1, 0, -1];

    newDirection = -1;
    
    if curDirection == 0
        searchDirections = [7, 0:6];
    else
        searchDirections = [(curDirection-1):(length(allDirections)-1), 0:(curDirection-2)];
    end
    
    for direction = searchDirections
        if img(curPoint(1)+dy(direction+1), curPoint(2)+dx(direction+1)) == value
            newDirection = direction;
            break;
        end
    end
    
%     keyboard
end

