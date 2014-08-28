% function areTheSame = decisionTree2_compareTwoTrees(tree1, tree2, checkOnlyLeaves, ignoreMultipleLabels)

% Check if two probe trees are the same. Optionally, check only the leaves.

% Example:
% areTheSame = decisionTree2_compareTwoTrees(tree1, tree2, false, false)

function areTheSame = decisionTree2_compareTwoTrees(tree1, tree2, checkOnlyLeaves, ignoreMultipleLabels)
    areTheSame = true;
    
    % If only one is a leaf, they don't match
    if (isfield(tree1, 'labelName') && ~isfield(tree2, 'labelName')) ||...
            (~isfield(tree1, 'labelName') && isfield(tree2, 'labelName'))
        
        areTheSame = false;
        keyboard
        return;
    end
    
    if isfield(tree1, 'labelName')
        if (length(tree1.labelName) ~= length(tree2.labelName) || length(tree1.labelID) ~= length(tree2.labelID))
            if ignoreMultipleLabels 
                return;
            else
                areTheSame = false;
                keyboard
                return;
            end
        end
        
        if ~min(strcmp(tree1.labelName, tree2.labelName)) || min(tree1.labelID ~= tree2.labelID)
            areTheSame = false;
            keyboard
        end
    else
        if ~checkOnlyLeaves
            if tree1.depth ~= tree2.depth ||...
                    tree1.whichFeature ~= tree2.whichFeature ||...
                    tree1.u8Threshold ~= tree2.u8Threshold ||...
                    abs(tree1.infoGain - tree2.infoGain) > 1e-4 ||...
                    abs(tree1.x - tree2.x) > 1e-4 ||...
                    abs(tree1.y - tree2.y) > 1e-4
                
                areTheSame = false;
                keyboard
            end
        end
        
        leftSame = decisionTree2_compareTwoTrees(tree1.leftChild, tree2.leftChild, checkOnlyLeaves, ignoreMultipleLabels);
        rightSame = decisionTree2_compareTwoTrees(tree1.rightChild, tree2.rightChild, checkOnlyLeaves, ignoreMultipleLabels);
        
        areTheSame = min([areTheSame, leftSame, rightSame]);
        
        if ~areTheSame
            keyboard
        end
    end
    