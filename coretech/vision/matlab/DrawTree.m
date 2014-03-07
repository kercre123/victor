    
% Example:
% DrawTree(probeTree, 0, true, Inf);
    function DrawTree(root, x, useShortLabels, maxDepth)
        if ~exist('maxDepth', 'var')
            maxDepth = Inf;
        end
        
        if ~exist('useShortLabels', 'var')
            useShortLabels = false;
        end        
    
        if maxDepth == 0
            return;
        end
        
        maxDepth = maxDepth - 1;
        
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
            if useShortLabels
                text(x+.01,-y, createSimpleLabelString(root), 'Hor', 'c');
            else
                text(x,-(y+.5), root.labelName, 'Hor', 'c');
            end
        elseif isfield(root, 'infoGain')
            if useShortLabels
                text(x+.01,-y, createSimpleLabelString(root));
            else
                %text(x+.01,-y, sprintf('(X=%.2f,Y=%.2f, IG=%.3f)', root.x, root.y, root.infoGain), 'Hor', 'left');
                text(x+.01,-y, sprintf('(X=%.2f,Y=%.2f, [%s])', root.x, root.y, root.remainingLabels), 'Hor', 'left');
            end
        else 
            text(x+.01,-y, 'Abandoned');
        end
        
        xspacing = 0.5^(root.depth-1);
                
        if isfield(root, 'rightChild')
            %plot(x+0.5, -(y+1), 'o');
            plot([x x+xspacing], -[y y+1]);
            
            DrawTree(root.rightChild, x + xspacing, useShortLabels, maxDepth);
        end
        
        if isfield(root, 'leftChild')
            %plot(x-xspacing, -(y+1), 'o');
            plot([x x-xspacing], -[y y+1]);
            DrawTree(root.leftChild, x-xspacing, useShortLabels, maxDepth);
        end
        
    end
    
    function textLabel = createSimpleLabelString(root)    
        % Take the long string of labels from the root node, and create a
        % numbers-only version
        if isfield(root, 'x')
            textLabel = sprintf('(.%d,.%d) ', round(root.x*100), round(root.y*100));
        else
            textLabel = '';
        end
        
        if isfield(root, 'remainingLabels')
            remainingLabels = root.remainingLabels;
        else
            remainingLabels = root.labelName;
        end
                
        typeStrings = {'ALL_BLACK','ALL_WHITE','angryFace','ankiLogo','batteries','bullseye','fire','squarePlusCorners'};

        originalLabelString = [remainingLabels, '    '];
        shortLabelString = '';
        for iType = 1:length(typeStrings)
            inds = strfind(originalLabelString, typeStrings{iType});
            
            if isempty(inds)
                continue;
            end
            
            shortDegreesString = '_{';
            withRotation = true;
            for i = inds
                degreesStart = i + length(typeStrings{iType});
                if originalLabelString(degreesStart) == ' '
                    withRotation = false;
                    assert(length(inds) == 1);
                else % == '_'
                    degreeString = originalLabelString((degreesStart+1):(degreesStart+3));
                    if strcmpi(degreeString, '000')
                        shortDegreesString = [shortDegreesString, '0'];
                    elseif strcmpi(degreeString, '090')
                        shortDegreesString = [shortDegreesString, '1'];
                    elseif strcmpi(degreeString, '180')
                        shortDegreesString = [shortDegreesString, '2'];
                    elseif strcmpi(degreeString, '270')
                        shortDegreesString = [shortDegreesString, '3'];
                    end
                end
            end
            shortDegreesString = [shortDegreesString, '}'];
            
            if withRotation
                shortLabelString = [shortLabelString, ' ', sprintf('%d',iType-1), shortDegreesString];
            else
                shortLabelString = [shortLabelString, ' ', sprintf('%d',iType-1)];
            end
        end
        
        textLabel = [textLabel, sprintf('\n'), shortLabelString];
    end

    
    
    