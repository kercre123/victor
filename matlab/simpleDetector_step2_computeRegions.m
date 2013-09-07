function [numRegions, area, indexList, bb, centroid] = simpleDetector_step2_computeRegions(binaryImg, usePerimeterCheck, embeddedConversions, DEBUG_DISPLAY)

if usePerimeterCheck
    % Perimeter not supported by mexRegionprops yet.
    statTypes = {'Area', 'PixelIdxList', 'BoundingBox', ...
        'Centroid', 'Perimeter'}; %#ok<UNRCH>
    stats = regionprops(binaryImg, statTypes);
    
    numRegions = length(stats);
    
    indexList = {stats.PixelIdxList};
    area = [stats.area];
    bb = vercat(stats.BoundinBox);
    centroid = vertcat(stats.Centroid);
else
    if strcmp(embeddedConversions.connectedComponentsType, 'matlab_original')
        [regionMap, numRegions] = bwlabel(binaryImg);
    elseif strcmp(embeddedConversions.connectedComponentsType, 'matlab_approximate')
        minimumComponentWidth = 4;
        components2d = approximateConnectedComponents(binaryImg, minimumComponentWidth);
        numRegions = length(components2d);
        regionMap = zeros(size(binaryImg));
        
%         [regionMapOriginal, numRegionsOriginal] = bwlabel(binaryImg);
        
        for i = 1:length(components2d)
            curComponents = components2d{i};
            for j = 1:size(curComponents, 1)
                regionMap(curComponents(j, 1), curComponents(j, 2):curComponents(j, 3)) = i;
            end
        end
%         keyboard
    end
    
    [area, indexList, bb, centroid] = mexRegionProps( ...
        uint32(regionMap), numRegions);
    area = double(area);
end
