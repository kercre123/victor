    function DrawTree(root, x)
        y = root.depth;
        if y == 0
            hold off
        end
        
        plot(x,-y, 'o'); hold on
        
        if y==0
            hold on
            set(gca, 'XTick', [], 'YTick', []);
        end
        
        if isfield(root, 'labelName')
            text(x,-(y+.5), root.labelName, 'Hor', 'c');
        elseif isfield(root, 'infoGain')
            %text(x+.01,-y, sprintf('(X=%.2f,Y=%.2f, IG=%.3f)', root.x, root.y, root.infoGain), 'Hor', 'left');
            text(x+.01,-y, sprintf('(X=%.2f,Y=%.2f, [%s])', root.x, root.y, root.remainingLabels), 'Hor', 'left');
        else 
            text(x+.01,-y, 'Abandoned');
        end
        
        xspacing = 0.5^(root.depth-1);
                
        if isfield(root, 'rightChild')
            %plot(x+0.5, -(y+1), 'o');
            plot([x x+xspacing], -[y y+1]);
            
            DrawTree(root.rightChild, x + xspacing);
        end
        
        if isfield(root, 'leftChild')
            %plot(x-xspacing, -(y+1), 'o');
            plot([x x-xspacing], -[y y+1]);
            DrawTree(root.leftChild, x-xspacing);
        end
        
    end