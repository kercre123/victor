function [array, maxDepth, leafLabels] = CreateTreeArray(root, probeRegion, gridSize)
% Convert node-with-child-pointers tree to flat array.
%
% [array, maxDepth, leafLabels] = CreateTreeArray(root)
%
%
% 
% --------------
% Andrew Stein
%

if isobject(root)
  assert(isa(root, 'ClassificationTree') || isa(root, 'classreg.learning.classif.CompactClassificationTree'), ...
    'If tree is an object, expecting it to be a [Compact]ClassificationTree.');
  
  assert(nargin > 2, 'ProbeRegion and GridSize must be passed in as second argument when tree is an object.');
    
  [xgrid,ygrid] = meshgrid(linspace(probeRegion(1), probeRegion(2), gridSize));
  
  probeIndex = cellfun(@(x)str2double(x(2:end)), root.CutVar);
  
  x = zeros(size(probeIndex));
  y = zeros(size(probeIndex));
  x(root.IsBranch) = xgrid(probeIndex(root.IsBranch));
  y(root.IsBranch) = ygrid(probeIndex(root.IsBranch));
  
  leftIndex = root.Children(:,1);
  leftIndex = num2cell(leftIndex);
  
  [leftIndex{~root.IsBranch}] = deal([]); % TODO: Use this for a discriminativity score for the leaf: max_score / second_best_score
  
  label = cellfun(@(name)str2double(name), root.NodeClass, 'UniformOutput', false);
  [label{root.IsBranch}] = deal([]);
  
  N = length(x);
  assert(length(y)==N, 'x and y should be the same size.');
  assert(length(leftIndex)==N, 'leftIndex should be same size as x and y.');
  assert(length(label)==N, 'label should be same size as leftIndex, x, and y.');
  assert(N == root.NumNodes, 'Expecting length of arrays to match NumNodes in tree object.');
  
  array = struct( ...
    'x', num2cell(x), ...
    'y', num2cell(y), ...
    'leftIndex',leftIndex, ...
    'label', label);
  
  maxDepth = GetTreeMaxDepth2(root);
  leafLabels = [];
  
else
  
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
        leafLabels = [leafLabels row(unique(root.labelID))];
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


function depth = GetTreeMaxDepth2(root, currentNode, currentDepth)

if nargin==1
  currentNode = 1;
  currentDepth = 0;
end

if ~root.IsBranch(currentNode)
  depth = currentDepth + 1;
  
else
  depth = max( ...
    GetTreeMaxDepth2(root, root.Children(currentNode,1), currentDepth + 1), ...
    GetTreeMaxDepth2(root, root.Children(currentNode,2), currentDepth + 1));
end

end % FUNCTION GetTreeMaxDepth2()


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

if isempty(array(index).leftIndex) || array(index).leftIndex==0
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