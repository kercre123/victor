% function decisionTree2_displayFailedPath(image, blackValue, whiteValue, path, failedAt)

% [labelName, labelID, path, failedAt] = decisionTree2_query(tree, imgU8, tform, blackValue, whiteValue, '0_000')
% decisionTree2_displayFailedPath(imgU8, blackValue, whiteValue, path, failedAt);

function decisionTree2_displayFailedPath(image, blackValue, whiteValue, path, failedAt)
    
    image = int32(image) - int32(blackValue);
    image = (image * 255) / (whiteValue - blackValue);
    image = uint8(image/2);
    
    hold off;
    imshow(image);
    hold on;
    
    for iNode = 1:(length(path)-1)
        xp = path{iNode}.xp;
        yp = path{iNode}.yp;
        
        scatter(xp, yp, 'k', 'linewidth', 5);
        numberText = sprintf('%d (%d)', iNode, int32(double(path{iNode}.normalizedPixelValue) - double(128)));
        
        if isempty(failedAt) || failedAt > iNode            
            scatter(xp, yp, 'b', 'linewidth', 1);
            text(xp, yp, numberText, 'color', 'w');
        else
            scatter(xp, yp, 'r');
            text(xp, yp, numberText, 'color', 'w');
            return
        end
    end % for iNode = 1:length(path)
    
    
    
    