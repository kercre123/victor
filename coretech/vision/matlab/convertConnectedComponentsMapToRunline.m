
function components2d = convertConnectedComponentsMapToRunline(regionMap)
%     function components1d = extract1dComponents(regionMap, minimumComponentWidth)

    maxId = 0;
    components2d = cell(10000,1);

    for y = 1:size(regionMap, 1)
        if regionMap(y,1) == 0
            onComponent = false;
            componentStart = -1;
        else
            onComponent = true;
            componentStart = 1;
        end

        for x = 2:size(regionMap, 2)
            if onComponent
                if regionMap(y,x) == 0
                    curId = regionMap(y,x-1);
                    maxId = max(maxId, curId);
                    components2d{curId}(end+1, :) = [y, componentStart, x-1];
                    onComponent = false;
                end
            else
                if regionMap(y,x) ~= 0
                    componentStart = x;
                    onComponent = true;
                end
            end
        end

        if onComponent
            curId = regionMap(y,x-1);
            maxId = max(maxId, curId);
            components2d{regionMap(y,x-1)}(end+1, :) = [y, componentStart, x-1];
        end
    end % for y = 1:size(regionMap, 1)
    
    components2d = components2d(1:maxId);