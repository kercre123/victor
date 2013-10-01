function draw(this, varargin)

AxesHandle = [];

parseVarargin(varargin{:});

if isempty(AxesHandle)
    AxesHandle = gca;
end

cla(AxesHandle(1))

world = this.robot.world;
camera = this.robot.camera;

[nrows,ncols] = size(this.image);

hold(AxesHandle(1), 'off')
imagesc(this.image, 'Parent', AxesHandle(1)), axis(AxesHandle(1), 'image', 'off')
hold(AxesHandle(1), 'on')

if length(AxesHandle) > 1 && ~isempty(this.matImage)
    cla(AxesHandle(2))
    imagesc(this.matImage, 'Parent', AxesHandle(2))
    axis(AxesHandle(2), 'image', 'off')
end

% Reproject the blocks and the markers onto the image:
for i_blockType = 1:BlockWorld.MaxBlockTypes
    
for i_block = 1:length(world.blocks{i_blockType})
    block = world.blocks{i_blockType}{i_block};
    if ~isempty(block)
        % Get the position of the block w.r.t. the camera, ready for
        % projection:
        pos = getPosition(block, camera.pose);
                
        X = reshape(pos(:,1), 4, []);
        Y = reshape(pos(:,2), 4, []);
        Z = reshape(pos(:,3), 4, []);
        
        [u,v] = camera.projectPoints(X, Y, Z);%, camera.pose);
        
        if any(u(:)>=1) || any(u(:)<=ncols) || ...
                any(v(:)>=1) || any(v(:)<=nrows)
            
            w = zeros(size(u));
            patch(u,v,w, block.color, 'FaceAlpha', 0.3, ...
                'LineWidth', 3, 'Parent', AxesHandle(1));
            
            % Plot block origin:
            plot(u(1), v(1), 'o', 'MarkerSize', 12, ...
                'MarkerEdgeColor', 'k', 'MarkerFaceColor', block.color, ...
                'LineWidth', 2, ...
                'Parent', AxesHandle(1));
            
        end
                
        for i_marker = 1:block.numMarkers
            marker = block.markers{i_marker};
            
            % Get the position of the marker w.r.t. the camera, ready for
            % projection:
            P = getPosition(marker, camera.pose);
                        
            [u,v] = camera.projectPoints(P);
            
            if any(u(:)>=1) || any(u(:)<=ncols) || ...
                    any(v(:)>=1) || any(v(:)<=nrows)
                
                w = .01*ones(size(u)); % put slightly in front of blocks
                patch(u([1 3 4 2]),v([1 3 4 2]),w, 'w', ...
                    'FaceColor', 'none', 'EdgeColor', 'w', ...
                    'LineWidth', 3, 'Parent', AxesHandle(1));
                
                % Plot marker origin:
                plot3(u(1), v(1), w(1), 'o', 'MarkerSize', 12, ...
                    'MarkerEdgeColor', 'k', 'MarkerFaceColor', 'w', ...
                    'LineWidth', 2, ...
                    'Parent', AxesHandle(1));
                
                % Plot docking dots:
                dots = marker.getPosition(camera.pose, 'DockingTarget');
                [u,v] = camera.projectPoints(dots);
                w = .01*ones(size(u));
                plot3(u, v, w, 'k.', 'MarkerSize', 10, 'Parent', AxesHandle(1));
                
            end
        end
    
    end % IF not empty block
    
end % FOR each block

end % FOR each block type


end %FUNCTION Observation/draw()