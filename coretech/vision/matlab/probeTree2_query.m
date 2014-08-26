% function [labelName, labelID] = probeTree2_query(probeTree, img, tform, blackValue, whiteValue)

% img = imresize(rgb2gray2(imread('/Users/pbarnum/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/rotated/dice6_000.png')), [30,30]);
% quad = [1 1; 1 size(img,1); size(img,2) 1; size(img,2) size(img,1)];
% tform = cp2tform(quad, [0 0; 0 1; 1 0; 1 1], 'projective');
% [labelName, labelID] = probeTree2_query(probeTree, img, tform 0, 255)

% quad = [1 1; 1 512; 512 1; 512 512]; tform = cp2tform(quad, [0 0; 0 1; 1 0; 1 1], 'projective');
% for i = 1:length(files) img = imread(files{i}); [labelName, labelID] = probeTree2_query(probeTree, img, tform, 0, 255); disp(sprintf('%s is %s', files{i}, labelName)); end

function [labelName, labelID] = probeTree2_query(probeTree, img, tform, blackValue, whiteValue)
    
    if isfield(probeTree, 'labelName')
        labelName = probeTree.labelName;
        labelID   = probeTree.labelID;
    else
        img = rgb2gray2(img);
        
        % TODO: does this have the correct 0.5 bias?
        [xp, yp] = tforminv(tform, probeTree.x, probeTree.y);
        xp = xp + 0.5;
        yp = yp + 0.5;
        
        minSubtractedValue = int32(img(round(yp),round(xp))) - blackValue;
        curPixel = (minSubtractedValue * 255) / (whiteValue - blackValue);
        curPixel = uint8(curPixel);
        
        if curPixel < probeTree.grayvalueThreshold
            [labelName, labelID] = probeTree2_query(probeTree.leftChild, img, tform, blackValue, whiteValue);
        else
            [labelName, labelID] = probeTree2_query(probeTree.rightChild, img, tform, blackValue, whiteValue);
        end
    end
    