% function image = components2dToImage(components2d, imageSize)

function image = components2dToImage(components2d, imageSize)

image = zeros(imageSize);

for iComponent = 1:length(components2d)
    for iSegment = 1:size(components2d{iComponent}, 1)
        y = components2d{iComponent}(iSegment,1);
        xs = components2d{iComponent}(iSegment,2):components2d{iComponent}(iSegment,3);
        image(y, xs) = iComponent;        
    end
end