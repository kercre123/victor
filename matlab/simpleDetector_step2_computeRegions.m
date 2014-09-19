function [numRegions, area, indexList, bb, centroid, components2d] = simpleDetector_step2_computeRegions(binaryImg, usePerimeterCheck, embeddedConversions, DEBUG_DISPLAY)

if usePerimeterCheck
    % Perimeter not supported by mexRegionprops yet.
    statTypes = {'Area', 'PixelIdxList', 'BoundingBox', 'Centroid', 'Perimeter'};
    stats = regionprops(binaryImg, statTypes);
    
    numRegions = length(stats);
    
    indexList = {stats.PixelIdxList};
    area = [stats.area];
    bb = vercat(stats.BoundinBox);
    centroid = vertcat(stats.Centroid);
else
    if strcmp(embeddedConversions.connectedComponentsType, 'matlab_original')
        [regionMap, numRegions] = bwlabel(binaryImg');
        regionMap = regionMap';
    elseif strcmp(embeddedConversions.connectedComponentsType, 'matlab_approximate')
%         minimumComponentWidth = 4;
        minimumComponentWidth = 0;
        create1dRegionMap = false;
        eightConnected = true;
        components2d = approximateConnectedComponents(binaryImg, minimumComponentWidth, create1dRegionMap, eightConnected);
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
    
    [area, indexList, bb, centroid] = mexRegionprops( ...
        uint32(regionMap), numRegions);
    area = double(area);
end

if ~exist('components2d', 'var')
    components2d = [];
end
