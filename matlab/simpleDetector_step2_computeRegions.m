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
    [regionMap, numRegions] = bwlabel(binaryImg);
    
    [area, indexList, bb, centroid] = mexRegionProps( ...
        uint32(regionMap), numRegions);
    area = double(area);
end
