function DrawTree(root, varargin)
% Draw a probe tree.
%
% DrawTree(root, <Param1, value1>, <Param2, value2>, ...)
%
%  Draws binary decision tree, showing label names at each leaf node and
%  probe location plus remaining labels at each decision node.
%  
%  Available parameters [default]:
%
%  'maxDepth' [inf]
%    Only draw nodes up to this tree depth.
%
%  'useShortLabels' [false]
%    Use label IDs instead of label names (should match C code label IDs)
%
% ------------
% Andrew Stein
%

maxDepth = inf;
useShortLabels = false;

parseVarargin(varargin{:});

if useShortLabels
    subscriptIndex = regexp(root.labels, '(_\d\d\d$)');
    labelNames = root.labels;
    for i = 1:length(labelNames)
        if ~isempty(subscriptIndex{i})
            labelNames{i} = labelNames{i}(1:(subscriptIndex{i}-1));
        end
    end
    % Note: this call to unique also sorts the names, but i think that's ok
    % because they are sorted in the C code too.
    labelNames = unique(labelNames);     
    labelFcn = @(node,x,y)ShortLabelTextFcn(node, x, y, labelNames);
else
    labelFcn = @LongLabelTextFcn;
end

cla
DrawTreeHelper(root, 0, labelFcn, maxDepth);
set(gca, 'XTick', [], 'YTick', []);

end % DrawTree()


function LongLabelTextFcn(node, xpos, ypos)

if isfield(node, 'labelName')
    text(xpos+.01, -ypos, AngleSubScripts(node.labelName, false), 'Hor', 'c');
elseif isfield(node, 'infoGain')
    text(xpos+.01,-ypos, sprintf('(X=%.2f,Y=%.2f, [%s])', node.x, node.y, AngleSubScripts(node.remainingLabels, false)), 'Hor', 'left');
else
    text(x+.01,-y, 'Abandoned');
end

end % LongLabelTextFcn()


function ShortLabelTextFcn(node, xpos, ypos, labelNames)

if any(isfield(node, {'labelName', 'infoGain'}))
    
    if isfield(node, 'labelName')
        underscoreIndex = find(node.labelName=='_', 1, 'last');
        if isempty(underscoreIndex)
            underscoreIndex = length(node.labelName)+1;
        end
        
        labelID = find(strcmp(labelNames, node.labelName(1:(underscoreIndex-1))));
        
        labelText = sprintf('%d%s', labelID-1, node.labelName(underscoreIndex:end));
    else
        labelText = ['[ ' node.remainingLabels ']'];
        
        for labelID = 1:length(labelNames)
            labelText = strrep(labelText, [' ' labelNames{labelID}], [' ' num2str(labelID-1)]);
        end
    end
        
    text(xpos+.01,-ypos, AngleSubScripts(labelText, true), 'Hor', 'c');
else
    text(x+.01,-y, 'Abandoned');
end

end % ShortLabelTextFcn()

function subscripted = AngleSubScripts(input, short)
if short
    % Replace 3-digit angles with 1-digit subscripts 0-3
    subscripted = strrep(strrep(strrep(strrep(input, '_000', '_0'), '_090', '_1'), '_180', '_2'), '_270', '_3');
else
    % Replace 3-digit angles with subscripted full angles (using braces)
    subscripted = strrep(strrep(strrep(strrep(input, '_000', '_0'), '_090', '_{90}'), '_180', '_{180}'), '_270', '_{270}');
end
end % AngleSubScripts()


function DrawTreeHelper(root, x, labelFcn, maxDepth)

if maxDepth == 0
    return;
end

maxDepth = maxDepth - 1;

y = root.depth;
plot(x,-y, 'o'); hold on

labelFcn(root, x, y);

xspacing = 0.5^(root.depth-1);

if isfield(root, 'rightChild')
    %plot(x+0.5, -(y+1), 'o');
    plot([x x+xspacing], -[y y+1]);
    
    DrawTreeHelper(root.rightChild, x + xspacing, labelFcn, maxDepth);
end

if isfield(root, 'leftChild')
    %plot(x-xspacing, -(y+1), 'o');
    plot([x x-xspacing], -[y y+1]);
    DrawTreeHelper(root.leftChild, x-xspacing, labelFcn, maxDepth);
end

end % DrawTreeHelper()

    