% function minimalProbeTree = decisionTree2_stripTree(probeTree)

% Strip the parts of the probeTree that are not needed for querying

% minimalProbeTree = decisionTree2_stripTree(probeTree);

function minimalProbeTree = decisionTree2_stripTree(probeTree)
    
    if isfield(probeTree, 'labelName')
        minimalProbeTree.labelID = probeTree.labelID;
        minimalProbeTree.labelName = probeTree.labelName;
    else
        minimalProbeTree.whichProbe = probeTree.whichProbe;
        minimalProbeTree.grayvalueThreshold = probeTree.grayvalueThreshold;
        minimalProbeTree.x = probeTree.x;
        minimalProbeTree.y = probeTree.y;
        minimalProbeTree.leftChild = decisionTree2_stripTree(probeTree.leftChild);
        minimalProbeTree.rightChild = decisionTree2_stripTree(probeTree.rightChild);
    end
end % decisionTree2_stripTree()


    