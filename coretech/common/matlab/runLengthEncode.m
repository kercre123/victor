% function compressed = runLengthEncode(image, filenameOut)

function compressed = runLengthEncode(image, filenameOut)

% Check that it is a binary image
uniqueValues = sort(unique(image(:)));
assert(length(uniqueValues) <= 2);

if length(uniqueValues) == 1
    assert(uniqueValues == 0 || uniqueValues == 1);
else
    assert(uniqueValues(1) == 0 && uniqueValues(2) == 1);
end
    

allComponents = zeros(0,2); % [isWhite, length]

curComponent = [image(1), 1];

numPixels = numel(image);

curPixel = 2;

image = image';

while curPixel <= numPixels
    % If the current component is the same color as the current pixel, and
    % not too big, keep growing it
    makeNewComponent = false;
    if curComponent(1) == image(curPixel) && curComponent(2) < 128
        curComponent(2) = curComponent(2) + 1;
    else
        makeNewComponent = true;
    end
        
    % Otherwise
    if makeNewComponent
        allComponents(end+1, :) = curComponent;
        
        curComponent = [image(curPixel), 1];
    end

    curPixel = curPixel + 1;
end

allComponents(end+1, :) = curComponent;

compressed = zeros([size(allComponents, 1),1], 'uint8');

for i = 1:size(allComponents, 1)
    compressed(i) = 128 * allComponents(i,1) + (allComponents(i,2)-1);
end

if exist('filenameOut', 'var')
    fileId = fopen(filenameOut, 'w');
    
    fwrite(fileId, compressed, 'uint8');
    
    fclose(fileId);
end