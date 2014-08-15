
% function probeTree2_drawTree(node, squareWidth)

function probeTree2_drawTree(node, squareWidth)
    if ~isfield(node, 'labelName')
        scatter(node.x*squareWidth, node.y*squareWidth);
        axis([0,squareWidth,0,squareWidth])
        hold on;
%         pause()
        probeTree2_drawTree(node.leftChild, squareWidth)
        probeTree2_drawTree(node.rightChild, squareWidth)
    end
end % drawTree()