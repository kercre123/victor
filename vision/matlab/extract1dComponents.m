% function components1d = extract1dComponents(binaryImgRow, minComponentWidth, maxSkipDistance)

% Compute 1d conencted components, in a 1xN binaryImageRow.
% To be detected as a component, a 1D component must be at least minComponentWidth wide
%
% If maxSkipDistance > 0, then the 1d components can span a gap of zeros of
% width maxSkipDistance

% binaryImgRow = [1,1,1,1,1,1,1,1,1,1]
% components1d = extract1dComponents(binaryImgRow, 2, 0)

% binaryImgRow = [0,1,1,1,1,1,0,1,1,1,0,0,1,1,1]
% components1d = extract1dComponents(binaryImgRow, 2, 0)
% components1d = extract1dComponents(binaryImgRow, 2, 1)


function components1d = extract1dComponents(binaryImgRow, minComponentWidth, maxSkipDistance)

    components1d = zeros(0,2);

    if binaryImgRow(1) == 0
        onComponent = false;
        componentStart = -1;
        numSkipped = 0;
    else
        onComponent = true;
        componentStart = 1;
        numSkipped = 0;
    end

    for x = 2:size(binaryImgRow, 2)
        if onComponent
            if binaryImgRow(x) == 0
                numSkipped = numSkipped + 1;
                if numSkipped > maxSkipDistance
                    componentWidth = x - componentStart;
                    if componentWidth >= minComponentWidth
                        components1d(end+1, :) = [componentStart, x-numSkipped];
                    end
                    onComponent = false;
                end
            else
                numSkipped = 0;
            end
        else
            if binaryImgRow(x) ~= 0
                componentStart = x;
                onComponent = true;
                numSkipped = 0;
            end
        end
    end
    
    if onComponent
        componentWidth = x - componentStart + 1;
        if componentWidth >= minComponentWidth
            components1d(end+1, :) = [componentStart, x-numSkipped];
        end
    end
    
end % function components1d = compute1dComponents(binaryImgRow)