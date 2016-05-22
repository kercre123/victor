% function [labelName, labelID] = decisionTree2_queryFast(tree, img, samplePositions, blackValue, whiteValue)

function [labelName, labelID] = decisionTree2_queryFast(tree, img, samplePositions, blackValue, whiteValue)
    
    curNode = tree;
    
    if whiteValue ~= 255 || blackValue ~= 0
        while ~isfield(curNode, 'labelName')
            index = samplePositions(curNode.whichFeature);
            
            minSubtractedValue = int32(img(index)) - blackValue;
            normalizedPixelValue = (minSubtractedValue * 255) / (whiteValue - blackValue);
            normalizedPixelValue = uint8(normalizedPixelValue);
            
            if normalizedPixelValue < curNode.u8Threshold
                curNode = curNode.leftChild;
            else
                curNode = curNode.rightChild;
            end
        end
    else
        while ~isfield(curNode, 'labelName')
            index = samplePositions(curNode.whichFeature);
            
%             assert(index == curNode.whichFeature);
            
            if img(index) < curNode.u8Threshold
                curNode = curNode.leftChild;
            else
                curNode = curNode.rightChild;
            end
        end
    end
    
    labelName = curNode.labelName;
    labelID   = curNode.labelID;
