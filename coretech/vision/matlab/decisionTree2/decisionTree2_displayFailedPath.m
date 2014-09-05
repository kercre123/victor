% function decisionTree2_displayFailedPath(image, tform, labelName, labelID, blackValue, whiteValue, path, failedAt)

% decisionTree2_displayFailedPath(imgU8, tform, labelName, labelID, blackValue, whiteValue, path, failedAt);

function decisionTree2_displayFailedPath(image, tform, labelName, labelID, blackValue, whiteValue, path, failedAt)
    
    image = int32(image) - int32(blackValue);
    image = (image * 255) / (whiteValue - blackValue);
    image = uint8(image);
    
    hold off;
    imshow(image);
    hold on;
    
    for iNode = 1:(length(path)-1)
        [xp, yp] = tforminv(tform, path{iNode}.x, path{iNode}.y);
        
        if isempty(failedAt) || failedAt > iNode
            scatter(xp, yp, 'y');
            text(xp, yp, sprintf('%d', iNode), 'color', 'b');
        else
            scatter(xp, yp, 'y');
            text(xp, yp, sprintf('%d', iNode), 'color', 'r');
            return
        end
    end % for iNode = 1:length(path)
    
    
    
    