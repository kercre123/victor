
function [numInternalNodes, numLeaves] = decisionTree2_countNumNodes(tree)
    if isfield(tree, 'labelName')
        numInternalNodes = 0;
        numLeaves = 1;
    else
        [numInternalNodesLeft, numLeavesLeft] = decisionTree2_countNumNodes(tree.leftChild);
        [numInternalNodesRight, numLeavesRight] = decisionTree2_countNumNodes(tree.rightChild);
        numLeaves = numLeavesLeft + numLeavesRight;
        numInternalNodes = 1 + numInternalNodesLeft + numInternalNodesRight;
    end
end % countNumNodes()