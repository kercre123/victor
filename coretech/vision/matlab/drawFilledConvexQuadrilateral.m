% function image = drawFilledConvexQuadrilateral(quad)

% quad is 4x2, each row is [x,y]

% for i = 1:1000 quad = rand(4,2) * 100; image = drawFilledConvexQuadrilateral(quad); pause(.1); end
% for i = 1:1000 quad = rand(4,2) * 100; image = drawFilledConvexQuadrilateral(quad); hold off; imshow(image); hold on; plot(quad([1:4,1],1)+0.5, quad([1:4,1],2)+0.5); pause(); end

function image = drawFilledConvexQuadrilateral(quad)
    
    assert(min(size(quad) == [4,2]) == 1);
    
    assert(min(quad(:)) >= 0);
    
    %     rect_x0 = min(quad(:,1));
    %     rect_x1 = max(quad(:,1));
    rect_y0 = min(quad(:,2));
    rect_y1 = max(quad(:,2));
    
    % For circular indexing
    quad = [quad; quad(1,:)];
    
    minY = ceil(rect_y0 - 0.5) + 0.5;
    maxY = floor(rect_y1 + 0.5) - 0.5;
    ys = minY:maxY;
    
    for iy = 1:length(ys)
        y = ys(iy);
        
        xIntercepts = zeros(0,1);
        
        % Compute all intersections
        for iCorner = 1:4
            if (quad(iCorner, 2) < y && quad(iCorner+1, 2) >= y) || (quad(iCorner+1, 2) < y && quad(iCorner, 2) >= y)
                dy = quad(iCorner+1, 2) - quad(iCorner, 2);
                dx = quad(iCorner+1, 1) - quad(iCorner, 1);
                
                alpha = (y - quad(iCorner, 2)) / dy;
                
                xIntercepts(end+1,:) = quad(iCorner, 1) + alpha * dx; %#ok<AGROW>
            end
        end % for iCorner = 1:4
        
        assert(size(xIntercepts,1) >= 1 && size(xIntercepts,1) <= 4);
        
        minX = min(xIntercepts);
        maxX = max(xIntercepts);
        
        %         xs = ceil(minX+0.5):floor(maxX-0.5);
        %         xs = ceil(minX):floor(maxX);
        %         xs = floor(minX+1.5):ceil(maxX-1.5);
        
        xs = ceil(minX+0.5):ceil(maxX-0.5);
        
        if ~isempty(xs)
            image(y+0.5, xs) = y + 0.5;
        end
    end % for iy = 1:length(ys)
    
    %     imshow(image);
    
    %     keyboard
    