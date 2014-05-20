function [numMulticlassNodes, numVerificationNodes] = GetNumTreeNodes()


numMulticlassNodes = GetNumNodesHelper(VisionMarkerTrained.ProbeTree);

if all(isfield(VisionMarkerTrained.ProbeTree, {'verifyTreeRed', 'verifyTreeBlack'}))
   numVerificationNodes = ...
       GetNumNodesHelper(VisionMarkerTrained.ProbeTree.verifyTreeRed) + ...
       GetNumNodesHelper(VisionMarkerTrained.ProbeTree.verifyTreeBlack);
else
    numVerificationNodes = 0;
    for iVerify = 1:length(VisionMarkerTrained.ProbeTree.verifiers)
        numVerificationNodes = numVerificationNodes + ...
            GetNumNodesHelper(VisionMarkerTrained.ProbeTree.verifiers(iVerify));
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