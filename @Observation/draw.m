function draw(this, varargin)

AxesHandle = [];

parseVarargin(varargin{:});

if isempty(AxesHandle)
    AxesHandle = gca;
end

cla(AxesHandle)

world = this.robot.world;
camera = this.robot.camera;
invRobotFrame = inv(this.robot.frame);

[nrows,ncols] = size(this.image);

hold(AxesHandle, 'off')
imagesc(this.image, 'Parent', AxesHandle), axis(AxesHandle, 'image', 'off')
hold(AxesHandle, 'on')

% Reproject the blocks and the markers onto the image:
for i_block = 1:world.MaxBlocks
    block = world.blocks{i_block};
    if ~isempty(block)
        [X,Y,Z] = invRobotFrame.applyTo(block.X, block.Y, block.Z);
        [u,v] = camera.projectPoints(X, Y, Z);
        
        if any(u(:)>=1) || any(u(:)<=ncols) || ...
                any(v(:)>=1) || any(v(:)<=nrows)
            
            w = zeros(size(u));
            patch(u,v,w, block.color, 'FaceAlpha', 0.3, ...
                'LineWidth', 3, 'Parent', AxesHandle);
            
            % Plot block origin:
            plot(u(1), v(1), 'o', 'MarkerSize', 12, ...
                'MarkerEdgeColor', 'k', 'MarkerFaceColor', block.color, ...
                'LineWidth', 2, ...
                'Parent', AxesHandle);
            
        end
        
        
        faces = block.faces;
        for i_face = 1:length(faces)
            marker = block.getFaceMarker(faces{i_face});
            P = invRobotFrame.applyTo(marker.P);
            [u,v] = camera.projectPoints(P);
            
            if any(u(:)>=1) || any(u(:)<=ncols) || ...
                    any(v(:)>=1) || any(v(:)<=nrows)
                
                w = .01*ones(size(u)); % put slightly in front of blocks
                patch(u([1 3 4 2]),v([1 3 4 2]),w, 'w', ...
                    'FaceColor', 'none', 'EdgeColor', 'w', ...
                    'LineWidth', 3, 'Parent', AxesHandle);
                
                % Plot marker origin:
                plot3(u(1), v(1), w(1), 'o', 'MarkerSize', 12, ...
                    'MarkerEdgeColor', 'k', 'MarkerFaceColor', 'w', ...
                    'LineWidth', 2, ...
                    'Parent', AxesHandle);
            end
        end
    
    end % IF not empty block
    
end % FOR each block

end %FUNCTION Observation/draw()