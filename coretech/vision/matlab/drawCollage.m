% function drawCollage()

% images = {uint8(255*rand(480,640,3)), uint8(255*rand(480,640,3)), uint8(255*rand(480,640,3))};
% [bigImage, bigImageArray, numImagesY, numImagesX, imageHeights, imageWidths] = drawCollage(images);

function [bigImage, bigImageArray, numImagesY, numImagesX, imageHeights, imageWidths] = drawCollage(images)
    %     maxCollageSize = [950,1800];
    maxCollageSize = [5000,5000];
    
    numImages = length(images);
    
    numImagesX = ceil(sqrt(numImages));
    numImagesY = ceil(numImages / numImagesX);

    bigImageArray = cell(numImagesY, numImagesX);

    imageWidths = zeros(numImagesY, numImagesX);
    imageHeights = zeros(numImagesY, numImagesX);

    cx = 1;
    cy = 1;
    for j = 1:length(images)
        bigImageArray{cy,cx} = images{j};
        imageHeights(cy,cx) = size(images{j},1);
        imageWidths(cy,cx) = size(images{j},2);
        cx = cx + 1;
        if cx > numImagesX
            cy = cy + 1;
            cx = 1;
        end
    end  % for j = 1:length(images)

    bigImageHeightOriginal = max(sum(imageHeights, 1));
    bigImageWidthOriginal = max(sum(imageWidths, 2));

    bigImageHeight = bigImageHeightOriginal;
    bigImageWidth = bigImageWidthOriginal;

    scale = 1;
    if bigImageHeight > maxCollageSize(1)
        scale = maxCollageSize(1) / bigImageHeight;
        bigImageHeight = bigImageHeight * scale;
        bigImageWidth = bigImageWidth * scale;
    end

    if bigImageWidth > maxCollageSize(2)
        scaleMultiplier = maxCollageSize(2) / bigImageWidth;
        scale = scale * scaleMultiplier;
        bigImageHeight = bigImageHeight * scaleMultiplier;
        bigImageWidth = bigImageWidth * scaleMultiplier;
    end

    bigImageHeight = ceil(bigImageHeight);
    bigImageWidth = ceil(bigImageWidth);

    bigImage = zeros([bigImageHeight, bigImageWidth, 3], 'uint8');

    cy = 1;
    for y = 1:numImagesY
        cx = 1;
        for x = 1:numImagesX
            smallImage = bigImageArray{y,x};

            if isempty(smallImage)
                break;
            end

            if ndims(smallImage) == 2
                smallImage = repmat(smallImage, [1,1,3]);
            end

            smallImage = im2uint8(smallImage);

            if scale ~= 1
                smallImage = imresize(smallImage, floor([size(smallImage,1),size(smallImage,2)]*scale));
            end

            bigImage(cy:(cy+size(smallImage,1)-1), cx:(cx+size(smallImage,2)-1), :) = smallImage;

            cx = cx + size(smallImage,2);
        end % for x = 1:numImagesX

        cy = cy + ceil(max(imageHeights(y,:))*scale);
    end % for y = 1:numImagesY