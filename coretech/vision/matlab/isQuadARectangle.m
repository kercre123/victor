% function isRect = isQuadARectangle(quad)

% quad is a 4x2 array. It can be [y,x] or [x,y].

function isRect = isQuadARectangle(quad)

isRect = true;

assert(size(quad,1) == 4); % Is quad even a quad

minQuadX = min(quad(:,1));
maxQuadX = max(quad(:,1));
minQuadY = min(quad(:,2));
maxQuadY = max(quad(:,2));

if ~(length(find(quad(:,1)==minQuadX)) == 2)
    isRect = false;
end
    
if ~(length(find(quad(:,1)==maxQuadX)) == 2)
    isRect = false;
end

if ~(length(find(quad(:,2)==minQuadY)) == 2)
    isRect = false;
end

if ~(length(find(quad(:,2)==maxQuadY)) == 2)
    isRect = false;
end