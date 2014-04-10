
function [xDecreasingImage, xIncreasingImage, yDecreasingImage, yIncreasingImage] = binaryTracker_binarizeWithDerivative(im, edgeThreshold, combHalfWidth)

% combHalfWidth = 1:9;
%
% im = double(im);
%
% figure(1); imshow(uint8(im));
% for h = combHalfWidth
%     xDiff = zeros(size(im));
%     for y = (1+max(combHalfWidth)):(size(im,1)-max(combHalfWidth))
%         for x = (1+max(combHalfWidth)):(size(im,2)-max(combHalfWidth))
%             xDiff(y,x) = abs(im(y,x-h) - im(y,x+h));
%         end
%     end
%
% %     figure(h+1); imshow(xDiff/max(xDiff(:)));
%     figure(h+1); imshow(xDiff/50);
% %     pause();
% end

% edgeThreshold = 10;
% grayvalueThreshold = 30;

xDecreasingImage = zeros(size(im));
xIncreasingImage = zeros(size(im));
yDecreasingImage = zeros(size(im));
yIncreasingImage = zeros(size(im));

im = double(im);
xDiff = zeros(size(im));
for y = (1+combHalfWidth):(size(im,1)-combHalfWidth)
    onEdge = false;
    componentStart = -1;

    for x = (1+combHalfWidth):(size(im,2)-combHalfWidth)
        xDiff(y,x) = im(y,x+combHalfWidth) - im(y,x-combHalfWidth);

        absDiff = abs(xDiff(y,x));

        if onEdge
            if absDiff < edgeThreshold
                middleX = floor((componentStart + x - 1) / 2);

                xDiffTotal = im(y,x-1) - im(y,componentStart);

                if xDiffTotal > 0
                    xIncreasingImage(y, middleX) = 1;
                else
                    xDecreasingImage(y, middleX) = 1;
                end

                onEdge = false;
            end
        else
            if absDiff >= edgeThreshold
                componentStart = x;
                onEdge = true;
            end
        end
    end % for x = 2:(size(im,2)-1)
end % for y = 2:(size(im,1)-1)

yDiff = zeros(size(im));
for x = (1+combHalfWidth):(size(im,2)-combHalfWidth)
    onEdge = false;
    componentStart = -1;

    for y = (1+combHalfWidth):(size(im,1)-combHalfWidth)
        yDiff(y,x) = im(y+combHalfWidth,x) - im(y-combHalfWidth,x);

        absDiff = abs(yDiff(y,x));
        
        disp(sprintf('%d %d', im(y+combHalfWidth,x), absDiff))

        if onEdge
            if absDiff < edgeThreshold
                middleY = floor((componentStart + y - 1) / 2);

                yDiffTotal = im(y-1,x) - im(componentStart,x);

                if yDiffTotal > 0
                    yIncreasingImage(middleY, x) = 1;
                else
                    yDecreasingImage(middleY, x) = 1;
                end

                onEdge = false;
            end
        else
            if absDiff >= edgeThreshold
                componentStart = y;
                onEdge = true;
            end
        end
    end % for x = 2:(size(im,2)-1)
end % for y = 2:(size(im,1)-1)

imshows(uint8(im), xDecreasingImage, xIncreasingImage, yDecreasingImage, yIncreasingImage, 'maximize')

keyboard



