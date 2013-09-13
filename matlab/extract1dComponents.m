% Compute 1d conencted components, in a 1xN binaryImageRow.
% To be detected as a component, a 1D component must be at least minComponentWidth wide

function components1d = extract1dComponents(binaryImgRow, minComponentWidth)

    components1d = zeros(0,2);

    if binaryImgRow(1) == 0
        onComponent = false;
        componentStart = -1;
    else
        onComponent = true;
        componentStart = 1;
    end

    for x = 2:size(binaryImgRow, 2)
        if onComponent
            if binaryImgRow(x) == 0
                componentWidth = x - componentStart;
                if componentWidth >= minComponentWidth
                    components1d(end+1, :) = [componentStart, x-1];
                end
                onComponent = false;
            end
        else
            if binaryImgRow(x) ~= 0
                componentStart = x;
                onComponent = true;
            end
        end
    end
    
    if onComponent
        componentWidth = x - componentStart + 1;
        if componentWidth >= minComponentWidth
            components1d(end+1, :) = [componentStart, x];
        end
    end
    
end % function components1d = compute1dComponents(binaryImgRow)