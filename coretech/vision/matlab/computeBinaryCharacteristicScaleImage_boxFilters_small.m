% function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_small(image, numLevels, thresholdFraction)

%  binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_small(im, 5, 0.75);

function binaryImage = computeBinaryCharacteristicScaleImage_boxFilters_small(image, numLevels, smallCharacterisicParameter)
    
    if ~exist('thresholdFraction' ,'var')
        thresholdFraction = 0.75;
    end
    
    DEBUG_DISPLAY = false;
    % DEBUG_DISPLAY = true;
    
    assert(size(image,3)==1, 'Image should be scalar-valued.');
    [nrows,ncols] = size(image);
    scaleImage = image;
    dog_max = zeros(nrows,ncols);
    
    maxFilterHalfWidth = 2 ^ (numLevels+1);
    
    imageWithBorders = zeros([nrows+2*maxFilterHalfWidth+1, ncols+2*maxFilterHalfWidth+1]);
    
    validYIndexes = (maxFilterHalfWidth+1):(maxFilterHalfWidth+nrows);
    validXIndexes = (maxFilterHalfWidth+1):(maxFilterHalfWidth+ncols);
    
    % Replicate the last pixel on the borders
    imageWithBorders(1:maxFilterHalfWidth, validXIndexes) = repmat(image(1,:), [maxFilterHalfWidth,1]);
    imageWithBorders((maxFilterHalfWidth+nrows+1):end, validXIndexes) = repmat(image(end,:), [maxFilterHalfWidth+1,1]);
    imageWithBorders(validYIndexes, 1:maxFilterHalfWidth) = repmat(image(:,1), [1,maxFilterHalfWidth]);
    imageWithBorders(validYIndexes, (maxFilterHalfWidth+ncols+1):end) = repmat(image(:,end), [1,maxFilterHalfWidth+1]);
    
    % Replicate the corners from the corner pixel
    imageWithBorders(1:maxFilterHalfWidth, 1:maxFilterHalfWidth) = image(1,1);
    imageWithBorders((maxFilterHalfWidth+nrows+1):end, 1:maxFilterHalfWidth) = image(end,1);
    imageWithBorders(1:maxFilterHalfWidth, (maxFilterHalfWidth+ncols+1):end) = image(1,1);
    imageWithBorders((maxFilterHalfWidth+nrows+1):end, (maxFilterHalfWidth+nrows+1):end) = image(end,end);
    
    imageWithBorders(validYIndexes, validXIndexes) = image;
    integralImageWithBorders = integralimage(imageWithBorders);
    
    filters = [ 0, 0, 0, 0, 1/1;
        %            -1, 0, 0, 0, 1/2;
        -1, 0, 1, 0, 1/3;
        %            -2, 0, 1, 0, 1/4;
        -2, 0, 2, 0, 1/5;
        %            -3, 0, 2, 0, 1/6;
        -3, 0, 3, 0, 1/7
        ];
    
    filtersT = zeros(size(filters));
    for i = 1:size(filters, 1)
        filtersT(i,:) = filters(i,[2,1,4,3,5]);
    end
    
%     [dog_max, scaleImage] = computeScale(integralImageWithBorders, filters, dog_max, scaleImage, validYIndexes, validXIndexes);
%     [~, scaleImage] = computeScale(integralImageWithBorders, filtersT, dog_max, scaleImage, validYIndexes, validXIndexes);
%     [~, scaleImage] = computeScale(image, dog_max, scaleImage);
    [~, binaryImage] = computeScale_localMinima(image, dog_max, scaleImage, smallCharacterisicParameter);
    
%     thresholdFraction = 0.99;
    
%     binaryImage = image < thresholdFraction*scaleImage;
    
%     imshows(binaryImage,2,'maximize')
    
end % FUNCTION computeCharacteristicScaleImage()

function [dog_max, binaryImage] = computeScale_localMinima(image, dog_max, scaleImage, threshold)
    binaryImage = zeros(size(scaleImage));
    
    xs = (1+3):(size(image,2)-3);
    
    for y = (1+3):(size(image,1)-3)        
        scale1ys = image(y,xs);
        scale1xs = image(y,xs);

        scale2ys = (image(y-1,xs) + image(y+1,xs)) / 2;
        scale2xs = (image(y,xs-1) + image(y,xs+1)) / 2;
            
        binaryImage(y,xs(scale1xs < (scale2xs*threshold))) = 255;
        binaryImage(y,xs(scale1ys < (scale2ys*threshold))) = 255;
        
%         for ix = 1:length(xs)
% %             scale = 0;            
% %             ys1 = [y-scale,y+scale];
% %             xs1 = [x-scale,x+scale];
% %             scale1y = sum(image(ys1,x)) / length(image(ys1,x));
% %             scale1x = sum(image(y,xs1)) / length(image(y,xs1));
% %             ys2 = [y-1,y+1];
% %             xs2 = [x-1,x+1];
% %             scale2y = sum(image(ys2,x)) / length(image(ys2,x));
% %             scale2x = sum(image(y,xs2)) / length(image(y,xs2));
% 
%             
% 
% %             ys2 = [y-1,y+1];
% %             xs2 = [x-1,x+1];
% %             scale2y = sum(image(ys2,x)) / 2;
% %             scale2x = sum(image(y,xs2)) / 2;
% 
%             if scale1xs(ix) < (scale2xs(ix)*threshold) || scale1ys(ix) < (scale2ys(ix)*threshold)
%                 binaryImage(y,xs(ix)) = 255;
%             end
%         end
    end
end % computeScale()


function [dog_max, scaleImage] = computeScale(image, dog_max, scaleImage)
    for y = (1+3):(size(image,1)-3)
        for x = (1+3):(size(image,2)-3)
            for scale = 0:2
                ys1 = [y-scale,y+scale];
                xs1 = [x-scale,x+scale];
                scale1y = sum(image(ys1,x)) / length(image(ys1,x));
                scale1x = sum(image(y,xs1)) / length(image(y,xs1));
                
                ys2 = [y-(scale+1),y+(scale+1)];
                xs2 = [x-(scale+1),x+(scale+1)];
                scale2y = sum(image(ys2,x)) / length(image(ys2,x));
                scale2x = sum(image(y,xs2)) / length(image(y,xs2));
                
                dogY = scale2y - scale1y;
                dogX = scale2x - scale1x;
                
%                 if x == 151 && y == 199
%                     disp(sprintf('%f %f %f', scale1, scale2, dog));
% %                     keyboard
%                 end
                
                if dogY > dog_max(y,x)
                    dog_max(y,x) = dogY;
                    scaleImage(y,x) = scale1y;
                end
                
                if dogX > dog_max(y,x)
                    dog_max(y,x) = dogX;
                    scaleImage(y,x) = scale1x;
                end
            end
        end
    end
end % computeScale()

% function [dog_max, scaleImage] = computeScale(integralImageWithBorders, filters, dog_max, scaleImage, validYIndexes, validXIndexes)
%     
%     numFilters = size(filters,1);
%     
%     filtered = cell(numFilters,1);
%     for iFilt = 1:numFilters
%         filtered{iFilt} = integralfilter(integralImageWithBorders, filters(iFilt,:));
%         filtered{iFilt} = filtered{iFilt}(validYIndexes,validXIndexes);
%     end
%     
%     dogs = cell(numFilters-1,1);
%     for iFilt = 1:(numFilters-1)
% %         dogs{iFilt} = abs(filtered{iFilt+1} - filtered{iFilt});
%         dogs{iFilt} = filtered{iFilt+1} - filtered{iFilt};
%         
%         larger = dogs{iFilt} > dog_max;
%         if any(larger(:))
%             dog_max(larger) = dogs{iFilt}(larger);
%             scaleImage(larger) = filtered{iFilt}(larger);
%         end
%     end
% end % computeScale()