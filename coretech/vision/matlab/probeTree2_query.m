% function [labelName, labelID] = probeTree2_query(probeTree, probeLocationsXGrid, probeLocationsYGrid, img, quad)

% img = imresize(rgb2gray2(imread('/Users/pbarnum/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/rotated/dice6_000.png')), [30,30]);
% quad = [1 1; 1 size(img,1); size(img,2) 1; size(img,2) size(img,1)];
% tform = cp2tform(quad, [0 0; 0 1; 1 0; 1 1], 'projective');
% [labelName, labelID] = probeTree2_query(probeTree, probeLocationsXGrid, probeLocationsYGrid, img, tform)

% quad = [1 1; 1 512; 512 1; 512 512]; tform = cp2tform(quad, [0 0; 0 1; 1 0; 1 1], 'projective');
% for i = 1:length(files) img = imread(files{i}); [labelName, labelID] = probeTree2_query(probeTree, probeLocationsXGrid, probeLocationsYGrid, img, tform); disp(sprintf('%s is %s', files{i}, labelName)); end

function [labelName, labelID] = probeTree2_query(probeTree, probeLocationsXGrid, probeLocationsYGrid, img, tform)
    
    if isfield(probeTree, 'labelName')
        labelName = probeTree.labelName;
        labelID   = probeTree.labelID;
    else
        img = rgb2gray2(img);
        
        % TODO: does this have the correct 0.5 bias?
        [xp, yp] = tforminv(tform, probeTree.x, probeTree.y);
        
        curPixel = img(round(yp),round(xp));
        
        if curPixel < probeTree.grayvalueThreshold
            [labelName, labelID] = probeTree2_query(probeTree.leftChild, probeLocationsXGrid, probeLocationsYGrid, img, tform);
        else
            [labelName, labelID] = probeTree2_query(probeTree.rightChild, probeLocationsXGrid, probeLocationsYGrid, img, tform);
        end
    end
    