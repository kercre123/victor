function [numMulticlassNodes, numVerificationNodes] = GetNumTreeNodes(tree)

if length(tree) > 1
    numMulticlassNodes = 0;
    numVerificationNodes = 0;

    for iTree = 1:length(tree)
        [numMulticlassNodes_, numVerificationNodes_] = VisionMarkerTrained.GetNumTreeNodes(tree(iTree));
        numMulticlassNodes = numMulticlassNodes + numMulticlassNodes_;
        numVerificationNodes = numVerificationNodes + numVerificationNodes_;
    end
    
    return
end

if nargin < 1
    tree = VisionMarkerTrained.ProbeTree;
end

numMulticlassNodes = GetNumNodesHelper(tree);

if all(isfield(tree, {'verifyTreeRed', 'verifyTreeBlack'}))
   numVerificationNodes = ...
       GetNumNodesHelper(tree.verifyTreeRed) + ...
       GetNumNodesHelper(tree.verifyTreeBlack);
else
    numVerificationNodes = 0;
    if isfield(tree, 'verifiers')
        for iVerify = 1:length(tree.verifiers)
            numVerificationNodes = numVerificationNodes + ...
                GetNumNodesHelper(tree.verifiers(iVerify));
        end
    end
end


if nargout < 2
    % Return sum
    numMulticlassNodes = numMulticlassNodes + numVerificationNodes;
end

end


function numNodes = GetNumNodesHelper(root, numNodes)

if nargin < 2
    numNodes = 0;
end

numNodes = numNodes + 1;
if ~isfield(root, 'labelName')
    numNodes = GetNumNodesHelper(root.leftChild, numNodes);
    numNodes = GetNumNodesHelper(root.rightChild, numNodes);
end

end % FUNCTION GetTreeNumNodes()