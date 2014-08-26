% function minimalProbeTree = decisionTree2_stripProbeTree(probeTree)

% Strip the parts of the probeTree that are not needed for querying

% minimalProbeTree = decisionTree2_stripProbeTree(probeTree);

function minimalProbeTree = decisionTree2_stripProbeTree(probeTree)
    
    if isfield(probeTree, 'labelName')
        minimalProbeTree.labelID = probeTree.labelID;
        minimalProbeTree.labelName = probeTree.labelName;
    else
        minimalProbeTree.whichProbe = probeTree.whichProbe;
        minimalProbeTree.grayvalueThreshold = probeTree.grayvalueThreshold;
        minimalProbeTree.x = probeTree.x;
        minimalProbeTree.y = probeTree.y;
        minimalProbeTree.leftChild = decisionTree2_stripProbeTree(probeTree.leftChild);
        minimalProbeTree.rightChild = decisionTree2_stripProbeTree(probeTree.rightChild);
    end
end % decisionTree2_stripProbeTree()


    