
% function tree = decisionTree2_convertCTree(depths, infoGains, whichFeatures, u8Thresholds, leftChildIndexs, labelNames, probeLocationsXGrid, probeLocationsYGrid)

%#ok<*CCAT>

function tree = decisionTree2_convertCTree(depths, infoGains, whichFeatures, u8Thresholds, leftChildIndexs, labelNames, probeLocationsXGrid, probeLocationsYGrid)
    
    tree = addNodes(1, depths, infoGains, whichFeatures, u8Thresholds, leftChildIndexs, labelNames, probeLocationsXGrid, probeLocationsYGrid);  
    
end % decisionTree2_convertCTree()

function tree = addNodes(curIndex, depths, infoGains, whichFeatures, u8Thresholds, leftChildIndexs, labelNames, probeLocationsXGrid, probeLocationsYGrid)
    
    if leftChildIndexs(curIndex) < 0
        if leftChildIndexs(curIndex) == -1
            labelId = length(labelNames) + 1;
            labelName = 'MARKER_UNKNOWN';
        else
            labelId = (-leftChildIndexs(curIndex)) - 1000000 + 1;
            labelName = labelNames(labelId);
        end
        
        tree = struct(...
            'depth', depths(curIndex),...
            'labelName', labelName,...
            'labelID', labelId);
                
        tree.remainingLabelNames = {tree.labelName};
    else
        tree = struct(...
            'depth', depths(curIndex),...
            'infoGain', infoGains(curIndex),...
            'whichFeature', whichFeatures(curIndex) + 1,...
            'u8Threshold', u8Thresholds(curIndex),...
            'x', double(probeLocationsXGrid(whichFeatures(curIndex) + 1)),...
            'y', double(probeLocationsYGrid(whichFeatures(curIndex) + 1)));
        
        tree.leftChild = addNodes(leftChildIndexs(curIndex) + 1, depths, infoGains, whichFeatures, u8Thresholds, leftChildIndexs, labelNames, probeLocationsXGrid, probeLocationsYGrid);
        tree.rightChild = addNodes(leftChildIndexs(curIndex) + 2, depths, infoGains, whichFeatures, u8Thresholds, leftChildIndexs, labelNames, probeLocationsXGrid, probeLocationsYGrid);
        
        tree.remainingLabelNames = unique({tree.leftChild.remainingLabelNames{:}, tree.rightChild.remainingLabelNames{:}});
    end
end
