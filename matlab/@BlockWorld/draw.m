function draw(this, varargin)

drawReprojection = true;
drawWorld = true;
drawOverheadMap = false;
pathHistory = 1000; %#ok<NASGU>

parseVarargin(varargin{:});

%% Draw the 3D World
if drawWorld
    
    AxesWorld = subplot(1,1,1, 'Parent', namedFigure('BlockWorld 3D'));
    for i_robot = 1:this.numRobots
        draw(this.robots{i_robot}, 'AxesHandle', AxesWorld);
    end
    
    for i_blockType = 1:BlockWorld.MaxBlockTypes
        if ~isempty(this.blocks{i_blockType})
            for i_block = 1:length(this.blocks{i_blockType})
                draw(this.blocks{i_blockType}{i_block}, 'AxesHandle', AxesWorld);
            end
        end
    end
    
    for i_block = 1:length(this.groundTruthBlocks)
        draw(this.groundTruthBlocks{i_block}, 'AxesHandle', AxesWorld, ...
            'DrawMarkers', false, 'FaceAlpha', .25, 'EdgeAlpha', .5);
    end
    
    for i_robot = 1:length(this.groundTruthRobots)
        draw(this.groundTruthRobots{i_robot}, 'AxesHandle', AxesWorld, ...
            'Alpha', .25);
    end
    
    axis(AxesWorld, 'equal');
    grid(AxesWorld, 'on');
    
end % IF drawWorld


%% Draw the reprojection error
if drawReprojection
    numCols = 1 + double(this.hasMat);
    AxesReproject = zeros(this.numRobots, numCols);
    h_fig = namedFigure('BlockWorld Reproject');
    for i = 1:this.numRobots
        AxesReproject(i,1) = subplot(this.numRobots,numCols,i, ...
            'Parent', h_fig);
        if this.hasMat
            AxesReproject(i,2) = subplot(this.numRobots, numCols, 2*i, ...
                'Parent', h_fig);
        end
    end
    
    for i_robot = 1:this.numRobots
        draw(this.robots{i_robot}.currentObservation, ...
            'AxesHandle', AxesReproject(i_robot,:));
        
        title(AxesReproject(i_robot,1), sprintf('Robot %d''s View', i_robot));
        
        if this.hasMat
            title(AxesReproject(i_robot,2), sprintf('Robot %d''s Mat View', i_robot));
        end
        
    end % FOR each robot
    
end % IF drawReprojection


%% Draw a simple overhead 2D map
if drawOverheadMap
    AxesMap = subplot(1,1,1, 'Parent', namedFigure('BlockWorldOverheadMap')); %#ok<UNRCH>
    
    h_truePath = findobj(AxesMap, 'Tag', 'TruePath');
    if isempty(h_truePath)
        hold(AxesMap, 'off');
        h_truePath      = zeros(1, this.numRobots);
        h_estimatedPath = zeros(1, this.numRobots);
        for i_robot = 1:this.numRobots
            poseTrue = this.getGroundTruthRobotPose(this.robots{i_robot}.name);
            h_truePath(i_robot) = plot(poseTrue.T(1), poseTrue.T(2), ...
                'r.-', 'MarkerSize', 10, 'Parent', AxesMap, 'Tag', 'TruePath');
            hold(AxesMap, 'on');
            
            poseEst = this.robots{i_robot}.pose;
            h_estimatedPath(i_robot) = plot(poseEst.T(1), poseEst.T(2), ...
                'bx-', 'Parent', AxesMap, 'Tag', 'EstimatedPath');
            if i_robot == 1
                legend(AxesMap, 'True Path', 'Estimated Path');
            end
        end % FOR each robot
        grid(AxesMap, 'on');
        axis(AxesMap, 'equal');
    else
        h_estimatedPath = findobj(AxesMap, 'Tag', 'EstimatedPath');
        
        for i_robot = 1:this.numRobots
            poseTrue = this.getGroundTruthRobotPose(this.robots{i_robot}.name);
            xTrue = get(h_truePath(i_robot), 'XData');
            yTrue = get(h_truePath(i_robot), 'YData');
            xTrue = xTrue(max(1,end-pathHistory):end);
            yTrue = yTrue(max(1,end-pathHistory):end);
            set(h_truePath(i_robot), ...
                'XData', [xTrue poseTrue.T(1)], ...
                'YData', [yTrue poseTrue.T(2)]);
            
            poseEst = this.robots{i_robot}.pose;
            xEst = get(h_estimatedPath(i_robot), 'XData');
            yEst = get(h_estimatedPath(i_robot), 'YData');
            xEst = xEst(max(1,end-pathHistory):end);
            yEst = yEst(max(1,end-pathHistory):end);
            set(h_estimatedPath(i_robot), ...
                'XData', [xEst poseEst.T(1)], ...
                'YData', [yEst poseEst.T(2)]);
            
            title(AxesMap, sprintf('RMS Path Error = %.3fmm', ...
                sqrt(mean( (xTrue(1:end-1)-xEst(2:end)).^2 + (yTrue(1:end-1)-yEst(2:end)).^2 ))), ...
                'FontWeight', 'b', 'FontSize', 12);
            
        end % FOR each robot
    end
        
end % IF drawOverheadMap

drawnow

end % FUNCTION draw()