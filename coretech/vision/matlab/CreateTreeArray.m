function [array, maxDepth, leafLabels] = CreateTreeArray(root)
% Convert node-with-child-pointers tree to flat array.
%
% [array, maxDepth, leafLabels] = CreateTreeArray(root)
%
%
% 
% --------------
% Andrew Stein
%


% Get the max depth
maxDepth = GetTreeMaxDepth(root);

array = GetEmptyNode();
[array, leafLabels] = CreateTreeArrayHelper(root, array, 1, []);

% Sanity check:
numNodes = GetTreeNumNodes(root);
if length(array) ~= numNodes
    error('Array has unexpected number of nodes (%d vs %d).', ...
        length(array), numNodes);
end

if nargout == 0
    DrawArray(array, 1, [0 0]);
end

end

function node = GetEmptyNode()

node = struct('x', [], 'y', [], 'leftIndex', [], 'label', []);

end

function [array, leafLabels] = CreateTreeArrayHelper(root, array, index, leafLabels)
    
assert(length(array)>=index, 'This node should have already been created.');
    
if isfield(root, 'labelID')
    % This is a leaf node
    if isscalar(root.labelID)
        % This leaf has a single label
        array(index).x = 0;
        array(index).y = 0;
        array(index).label = root.labelID;
    else
        % This leaf has multiple labels
        array(index).x = length(leafLabels) + 1;
        array(index).label = -1;
        leafLabels = [leafLabels row(root.labelID)];
        array(index).y = length(leafLabels) + 1;        
    end
else
    assert(all(isfield(root, {'leftChild', 'rightChild'})));
    
    % Figure out where left child will appear in the array
    leftIndex  = length(array)+1;
        
    % Fill in this node's information 
    array(index).x = root.x;
    array(index).y = root.y;
    array(index).leftIndex = leftIndex;
    
    % Create empty nodes for this node's children
    rightIndex = leftIndex+1;
    array(leftIndex) = GetEmptyNode();
    array(rightIndex) = GetEmptyNode();
   
    % Recurse on each child
    [array, leafLabels] = CreateTreeArrayHelper(root.leftChild,  array, leftIndex,  leafLabels);
    [array, leafLabels] = CreateTreeArrayHelper(root.rightChild, array, rightIndex, leafLabels);
   
end
    
    
end % FUNCTION CreateTreeArrayHelper()


function maxDepth = GetTreeMaxDepth(root, maxDepth)

if nargin < 2
    maxDepth = 1;
end

if isfield(root, 'labelName')
    maxDepth = max(maxDepth, root.depth);
else
    maxDepth = GetTreeMaxDepth(root.leftChild, maxDepth);
    maxDepth = GetTreeMaxDepth(root.rightChild, maxDepth);
end

end % FUNCTION GetTreeMaxDepth()

function numNodes = GetTreeNumNodes(root, numNodes)

if nargin < 2
    numNodes = 0;
end

numNodes = numNodes + 1;
if ~isfield(root, 'labelName')
    numNodes = GetTreeNumNodes(root.leftChild, numNodes);
    numNodes = GetTreeNumNodes(root.rightChild, numNodes);
end

end % FUNCTION GetTreeNumNodes()

function DrawArray(array, index, pos)

if index==1
    hold off;
end
plot(pos(1), -pos(2), 'o');
if index==1
    hold on
end

if isempty(array(index).leftIndex)
    text(pos(1), -(pos(2)+.1), num2str(array(index).label), 'Hor', 'c');
else
    xspacing = 0.5^pos(2);
    
    leftPos  = [pos(1)-xspacing pos(2)+1];
    rightPos = [pos(1)+xspacing pos(2)+1];
    
    plot([pos(1) leftPos(1)], -[pos(2) leftPos(2)]);
    plot([pos(1) rightPos(1)], -[pos(2) rightPos(2)]);
    
    DrawArray(array, array(index).leftIndex, leftPos);
    DrawArray(array, array(index).leftIndex+1, rightPos);
    
end

end