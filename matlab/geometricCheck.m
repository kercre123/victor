function [quad] = geometricCheck(img, x, y, r)

DEBUG_DISPLAY = true;
maxDistMultiple = 20;
minDistMultiple = 5;
cornerLocationMultiple = 2;

quad = {};


if isempty(x)
    return;
end

points = [x(:) y(:)];

% Create edges (via an adjacency matrix) between points that are within an
% acceptable distance of each other and for which the intensity profile
% along the line connecting them is "white"
numPoints = size(points,1);
adj = false(numPoints);
for i=1:(numPoints-1)
    for j = (i+1):numPoints
        
        r_max = max(r([i j]));
        r_min = min(r([i j]));
        if r_max/r_min < 3
            
            d_sq = sum( (points(i,:)-points(j,:)).^2 );
            
            if d_sq > (minDistMultiple*r_min)^2 && d_sq < (maxDistMultiple*r_max)^2
                
                prof = improfile(img, points([i j],1), points([i j],2));
                dark = min(img(round(points(i,2)),round(points(i,1))), ...
                    img(round(points(j,2)),round(points(j,1))));
                bright = max(prof);
                intensityThreshold = (dark+bright)/2;
                intensityCheck = all(prof(round(2*r(i)):(end-round(2*r(j)))) > intensityThreshold);
                
                %{
                % DEBUG:
                namedFigure('Profile')
                subplot(121), hold off, imshow(img), hold on
                plot(points([i j], 1), points([i,j],2), 'b');
                title([i j])
                subplot 122
                hold off, plot(prof), hold on, plot([1 length(prof)], intensityThreshold*[1 1], 'r--'), title(sprintf('Intensity Check = %d', intensityCheck))
                pause
                %}
                
                if intensityCheck
                    adj(i,j) = true;
                    % make symmetric
                    adj(j,i) = adj(i,j);
                end
                
            end % IF the two points are an acceptable distance apart
            
        end % IF the two detections aren't drastically different sizes
    end
end

% Remove points that don't have at least two neighbors
toKeep = sum(adj,2) >= 2;
adj = adj(toKeep,toKeep);
points = points(toKeep,:);
r = r(toKeep);

% Store the remaining edges
[e1,e2] = find(adj);
edges = [e1(:) e2(:)];

% Canonical equilateral triangle:
trix = [0 0 1]';
triy = [0 1 0]';

if DEBUG_DISPLAY
    %namedFigure(sprintf('Geometric Check %.1f', dotRadius))
    namedFigure('Geometric Check')
    hold off, imshow(img), hold on
    %triplot(DT, 'b', 'LineStyle', '--');
    for i = 1:size(points,1)
        rectangle('Pos', [points(i,:)-r(i) r(i)*[2 2]], ...
            'Curvature', [1 1], 'EdgeColor', 'b');
    end
    line([points(edges(:,1),1) points(edges(:,2),1)]', ...
        [points(edges(:,1),2) points(edges(:,2),2)]', ...
        'Color', 'b', 'LineWidth', 2);
end

% create a list of neighbors for each point
neighbors = idMap2Index(edges(:,1), edges(:,2), size(points,1));

% for each of those points
for i_pt = 1:size(points,1)
    x1 = points(i_pt,1);
    y1 = points(i_pt,2);
    r1 = r(i_pt);
    
    % for each pair of edges connected to that point
    for i_neighbor1 = 1:(length(neighbors{i_pt})-1)
        neighbor1 = neighbors{i_pt}(i_neighbor1);
        
        if neighbor1 < i_pt, continue, end
        
        x2 = points(neighbor1,1);
        y2 = points(neighbor1,2);
        r2 = r(neighbor1);
        
        for i_neighbor2 = (i_neighbor1+1):length(neighbors{i_pt})
            neighbor2 = neighbors{i_pt}(i_neighbor2);
            
            if neighbor2 < i_pt, continue, end
            
            x3 = points(neighbor2,1);
            y3 = points(neighbor2,2);
            r3 = r(neighbor2);
            
            % make sure the three points aren't colinear (i.e. that two
            % don't predict the third)
            colinearityThresh = min([r1 r2 r3]);
            if ~areColinear(x1,y1, x2,y2, x3,y3, colinearityThresh)
                
                % find any points which are neighbors of both ends of the two edges
                candidates = intersect(neighbors{neighbor1}, neighbors{neighbor2});
                candidates(candidates==i_pt) = [];
                
                if ~isempty(candidates)
                    % compute the affine transformation to map those three points to a
                    % right triangle
                    tform = cp2tform([x1 y1; x2 y2; x3 y3], [trix triy], 'affine');
                    
                    % compute where in the image the 4th point should lie
                    [xquad, yquad] = tforminv(tform, 1, 1);
                    
                    % for each of those points (which form a quadrilateral)
                    for i_cand = 1:length(candidates)
                        x4 = points(candidates(i_cand),1);
                        y4 = points(candidates(i_cand),2);
                        
                        if ~areColinear(x1,y1, x2,y2, x4,y4, colinearityThresh) && ...
                                ~areColinear(x1,y1, x3,y3, x4,y4, colinearityThresh) && ...
                                ~areColinear(x2,y2, x3,y3, x4,y4, colinearityThresh)
                            
                            % see how far that point is from the ideal 4th point
                            d_sq = sum((points(candidates(i_cand),:) - [xquad yquad]).^2);
                            
                            % if close enough, keep this set of 4 points
                            if d_sq < (cornerLocationMultiple*colinearityThresh)^2
                                corners = points([i_pt neighbor1 neighbor2 candidates(i_cand)],:);

                                % Verfiy corners are in a clockwise direction, so we don't get an
                                % accidental projective mirroring when we do the tranformation below to
                                % extract the image.  Can look whether the z direction of cross product
                                % of the two vectors forming the quadrilateral is positive or negative.
                                A = [corners(2,:) - corners(1,:);
                                    corners(3,:) - corners(1,:)];
                                if det(A) > 0
                                    corners([2 3],:) = corners([3 2],:);
                                end
                                
                                quad{end+1} = corners; 
                                
                            elseif false
                                h(1) = plot(x1,y1, 'rx', 'MarkerSize', 20);
                                h(2) = plot([x2 x3], [y2 y3], 'ro', 'MarkerSize', 20);
                                h(3) = plot(xquad, yquad, 'r*', 'MarkerSize', 20);
                                h(4) = plot(x4, y4, 'gs', 'MarkerSize', 20);
                                pause 
                                delete(h)
                            end
                            
                        end % IF 4th point isn't colinear with others
                        
                    end % FOR each candidate
                    
                end % IF there are any candidate 4th points
                
            end % IF points are not co-linear
            
        end % FOR each 2nd neighbor
        
    end % FOR each 1st neighbor
    
end % FOR each point with neighbors

% if ~isempty(quad)
%     % This just makes sure we have a unique set of quads
%     % TODO: need a way to avoid checking the same 4 points over and over
%     % again in the loops above
%     numQuads = length(quad);
%     quad = unique(vertcat(quad{:}), 'rows');
%     if size(quad,1) < numQuads
%         warning('Still getting duplicate quads.');
%     end
%     
%     quad = mat2cell(quad, ones(size(quad,1),1), 4);
%     quad = cellfun(@(q)points(q,:), quad, 'UniformOutput', false);
% end

end % FUNCTION

function tf = areColinear(x1,y1, x2,y2, x3,y3, threshold)

if x1 ~= x2
    m = (y2-y1)/(x2-x1);
    b = y1-m*x1;
    y3pred = m*x3 + b;
    d = abs(y3-y3pred);
elseif y1 ~= y2
    % x's are equal, must be vertical line, just make sure
    % third x value isn't also on that line
    d = abs(x3-x1);
else
    error('First two points cannot be the same!');
end

tf = d < threshold;

end
