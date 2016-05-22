% function minimalTree = decisionTree2_stripTree(tree)

% Strip the parts of the tree that are not needed for querying

% minimalTree = decisionTree2_stripTree(tree);

function minimalTree = decisionTree2_stripTree(tree)
    
    if isfield(tree, 'labelName')
        minimalTree.labelID = tree.labelID;
        minimalTree.labelName = tree.labelName;
    else
        minimalTree.whichFeature = tree.whichFeature;
        minimalTree.u8Threshold = tree.u8Threshold;
        minimalTree.x = tree.x;
        minimalTree.y = tree.y;
        minimalTree.leftChild = decisionTree2_stripTree(tree.leftChild);
        minimalTree.rightChild = decisionTree2_stripTree(tree.rightChild);
    end
end % decisionTree2_stripTree()


    