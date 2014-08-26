% function areTheSame = decisionTree2_compareTwoTrees(probeTree1, probeTree2)

% Check if two probe trees are the same. Optionally, check only the leaves.

% Example:
% areTheSame = decisionTree2_compareTwoTrees(probeTree1, probeTree2, false, false)

function areTheSame = decisionTree2_compareTwoTrees(probeTree1, probeTree2, checkOnlyLeaves, ignoreMultipleLabels)
    areTheSame = true;
    
    % If only one is a leaf, they don't match
    if (isfield(probeTree1, 'labelName') && ~isfield(probeTree2, 'labelName')) ||...
            (~isfield(probeTree1, 'labelName') && isfield(probeTree2, 'labelName'))
        
        areTheSame = false;
        keyboard
        return;
    end
    
    if isfield(probeTree1, 'labelName')
        if (length(probeTree1.labelName) ~= length(probeTree2.labelName) || length(probeTree1.labelID) ~= length(probeTree2.labelID))
            if ignoreMultipleLabels 
                return;
            else
                areTheSame = false;
                keyboard
                return;
            end
        end
        
        if ~min(strcmp(probeTree1.labelName, probeTree2.labelName)) || min(probeTree1.labelID ~= probeTree2.labelID)
            areTheSame = false;
            keyboard
        end
    else
        if ~checkOnlyLeaves
            if probeTree1.depth ~= probeTree2.depth ||...
                    probeTree1.whichProbe ~= probeTree2.whichProbe ||...
                    probeTree1.grayvalueThreshold ~= probeTree2.grayvalueThreshold ||...
                    abs(probeTree1.infoGain - probeTree2.infoGain) > 1e-4 ||...
                    abs(probeTree1.x - probeTree2.x) > 1e-4 ||...
                    abs(probeTree1.y - probeTree2.y) > 1e-4
                
                areTheSame = false;
                keyboard
            end
        end
        
        leftSame = decisionTree2_compareTwoTrees(probeTree1.leftChild, probeTree2.leftChild, checkOnlyLeaves, ignoreMultipleLabels);
        rightSame = decisionTree2_compareTwoTrees(probeTree1.rightChild, probeTree2.rightChild, checkOnlyLeaves, ignoreMultipleLabels);
        
        areTheSame = min([areTheSame, leftSame, rightSame]);
        
        if ~areTheSame
            keyboard
        end
    end
    