
% draw the tracks
function keypointImage = drawKeypointMatches(image, points0, points1, distanceColormap, drawLines)
    
    assert(length(points0) == length(points1));
    
    if ~exist('distanceColormap', 'var')
        distanceColormap = [];
    end
    
    if ~exist('drawLines', 'var')
        drawLines = true;
    end
    
    if drawLines
        thickness = 1;
    else
        thickness = 11;
    end
    
    keypointImage = repmat(image,[1,1,3]);
    
    colors = cell(length(points0), 1);
    
    for iPoint = 1:length(points0)
        x0 = points0{iPoint}(1);
        y0 = points0{iPoint}(2);
        
        x1 = points1{iPoint}(1);
        y1 = points1{iPoint}(2);
        
        if isempty(distanceColormap)
            colors{iPoint} = [255,0,0];
        else
            dist = sqrt((x0-x1)^2 + (y0-y1)^2);
            dist = round(dist);
            dist = dist + 1; % 0 is actually index 1
            
            if dist > length(distanceColormap)
                dist = length(distanceColormap);
            end
            
            colors{iPoint} = distanceColormap(dist, :);
        end
        
        if ~drawLines
            points1{iPoint} = points0{iPoint};
        end
    end % for iPoint = 1:length(points0)
    
    keypointImage = cv.line(keypointImage, points0, points1, 'Colors', colors, 'Thickness', thickness);
end % function drawKeypointMatches(image, points0, points1, distanceColormap)

