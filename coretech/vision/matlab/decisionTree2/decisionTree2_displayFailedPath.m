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
        
        scatter(xp, yp, 'k', 'linewidth', 5);
        numberText = sprintf('%d', iNode);
        
        if isempty(failedAt) || failedAt > iNode            
            scatter(xp, yp, 'b', 'linewidth', 1);
            text(xp, yp, numberText, 'color', 'w');
        else
            scatter(xp, yp, 'r');
            text(xp, yp, numberText, 'color', 'w');
            return
        end
    end % for iNode = 1:length(path)
    
    
    
    