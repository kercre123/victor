% function [labelName, labelID, path, failedAt] = decisionTree2_query(tree, img, tform, blackValue, whiteValue)

% img = imresize(rgb2gray2(imread('/Users/pbarnum/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/rotated/dice6_000.png')), [30,30]);
% quad = [1 1; 1 size(img,1); size(img,2) 1; size(img,2) size(img,1)];
% tform = cp2tform(quad, [0 0; 0 1; 1 0; 1 1], 'projective');
% [labelName, labelID] = decisionTree2_query(tree, img, tform 0, 255)

% quad = [1 1; 1 512; 512 1; 512 512]; tform = cp2tform(quad, [0 0; 0 1; 1 0; 1 1], 'projective');
% for i = 1:length(files) img = imread(files{i}); [labelName, labelID] = decisionTree2_query(tree, img, tform, 0, 255); disp(sprintf('%s is %s', files{i}, labelName)); end

function [labelName, labelID, path, failedAt] = decisionTree2_query(tree, img, tform, blackValue, whiteValue, groundTruthLabel)
    
    if ~exist('groundTruthLabel', 'var')
        groundTruthLabel = [];
    end
    
    if isfield(tree, 'labelName')
        labelName = tree.labelName;
        labelID   = tree.labelID;
        failedAt = [];
        path = {};
        normalizedPixelValue = [];
        xp = [];
        yp = [];
    else
        img = rgb2gray2(img);
        
        % TODO: does this have the correct 0.5 bias?
        [xp, yp] = tforminv(tform, tree.x, tree.y);
        xp = round(xp + 0.5);
        yp = round(yp + 0.5);
        
        minSubtractedValue = int32(img(yp,xp)) - blackValue;
        normalizedPixelValue = (minSubtractedValue * 255) / (whiteValue - blackValue);
        normalizedPixelValue = uint8(normalizedPixelValue);
        
        if normalizedPixelValue < tree.u8Threshold
            [labelName, labelID, path, failedAt] = decisionTree2_query(tree.leftChild, img, tform, blackValue, whiteValue, groundTruthLabel);
        else
            [labelName, labelID, path, failedAt] = decisionTree2_query(tree.rightChild, img, tform, blackValue, whiteValue, groundTruthLabel);
        end
    end
    
    curNode = tree;
    if isfield(tree, 'leftChild')
        curNode = rmfield(curNode, 'leftChild');
        curNode = rmfield(curNode, 'rightChild');
    end
    
    curNode.normalizedPixelValue = normalizedPixelValue;
    curNode.xp = xp;
    curNode.yp = yp;
    
    path = [{curNode}, path];
    
    if ~isempty(groundTruthLabel)
        if isempty(find(ismember(tree.remainingLabelNames,'0_000'), 1))
            failedAt = tree.depth; % Actually -1, but matlab's 1 indexing
        end
    end
    