function draw(this, varargin)

% AxesWorld = [];
% AxesReproject = [];
% 
% parseVarargin(varargin{:});

% if isempty(AxesWorld)
%     if isempty(AxesProject)
        
AxesWorld = subplot(1,1,1, 'Parent', namedFigure('BlockWorld 3D'));
AxesReproject = zeros(1,this.numRobots);
for i = 1:this.numRobots
    AxesReproject(i) = subplot(this.numRobots,1,i, ...
        'Parent', namedFigure('BlockWorld Reproject'));
end    
%     end
% end

%% Draw the 3D World
for i_robot = 1:this.numRobots
    draw(this.robots{i_robot}, 'AxesHandle', AxesWorld); 
end

for i_block = 1:this.MaxBlocks
    if ~isempty(this.blocks{i_block})
        draw(this.blocks{i_block}, 'AxesHandle', AxesWorld);
    end
end

for i_marker = 1:numel(this.markers)
    if ~isempty(this.markers{i_marker})
        draw(this.markers{i_marker}, 'Mode', 'world', 'where', AxesWorld);
    end
end

axis equal
grid on

%% Draw the reprojection error

for i_robot = 1:this.numRobots
    camera = this.robots{i_robot}.camera;
    invRobotFrame = inv(this.robots{i_robot}.frame);
    
    [nrows,ncols] = size(camera.image);
    
    hold(AxesReproject(i_robot), 'off')
    imshow(camera.image, 'Parent', AxesReproject(i_robot));
    title(sprintf('Robot %d''s View', i_robot), 'Parent', AxesReproject(i_robot));
    hold(AxesReproject(i_robot), 'on')
    
    % Reproject the blocks and the markers onto the image:
    for i_block = 1:this.MaxBlocks
        block = this.blocks{i_block};
        if ~isempty(block)
            [X,Y,Z] = invRobotFrame.applyTo(block.X, block.Y, block.Z);
            [u,v] = camera.projectPoints(X, Y, Z);
            
            if any(u(:)>=1) || any(u(:)<=ncols) || ...
                    any(v(:)>=1) || any(v(:)<=nrows)
                
                w = zeros(size(u));
                patch(u,v,w, block.color, 'FaceAlpha', 0.3, ...
                    'LineWidth', 3, 'Parent', AxesReproject(i_robot));
                
                % Plot block origin:
                plot(u(1), v(1), 'o', 'MarkerSize', 12, ...
                    'MarkerEdgeColor', 'k', 'MarkerFaceColor', block.color, ...
                    'LineWidth', 2, ...
                    'Parent', AxesReproject(i_robot));
                
            end
        end
    end % FOR each block
    
    for i_marker = 1:numel(this.markers)
        marker = this.markers{i_marker};
        if ~isempty(marker)
            
            P = invRobotFrame.applyTo(marker.P);
            [u,v] = camera.projectPoints(P);
            
            if any(u(:)>=1) || any(u(:)<=ncols) || ...
                    any(v(:)>=1) || any(v(:)<=nrows)
                
                w = .01*ones(size(u)); % put slightly in front of blocks
                patch(u([1 3 4 2]),v([1 3 4 2]),w, 'w', ...
                    'FaceColor', 'none', 'EdgeColor', 'w', ...
                    'LineWidth', 3, 'Parent', AxesReproject(i_robot));
               
            end
            
        end
    end % FOR each marker
   
    
end % FOR each robot

drawnow

end % FUNCTION draw()