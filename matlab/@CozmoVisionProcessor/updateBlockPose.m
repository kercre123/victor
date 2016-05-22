function updateBlockPose(this, H)

% Turning this on doesn't seem to be working well
computeInitialPose = false;

if computeInitialPose
    % Compute pose of block w.r.t. camera
    % De-embed the initial 3D pose from the homography:
    scale = mean([norm(H(:,1));norm(H(:,2))]);
    %scale = H(3,3);
    H = H/scale;
    
    u1 = H(:,1);
    u1 = u1 / norm(u1);
    u2 = H(:,2) - dot(u1,H(:,2)) * u1;
    u2 = u2 / norm(u2);
    u3 = cross(u1,u2);
    R  = [u1 u2 u3];
    %Rvec = rodrigues(Rmat);
    T  = H(:,3);
    
    initialPose = Pose(R,T);
else
    initialPose = [];
end

% Now refine that initial estimate with some nonlinear
% optimization (reprojection error) goodness...
% (Need to work in camera resolution vs. tracker resolution)
scale = this.headCam.ncols / this.trackingResolution(1);
this.block.pose = this.headCam.computeExtrinsics(...
    scale*this.LKtracker.corners, this.marker3d, ...
    'initialPose', initialPose);

%{
            this.block.pose = Pose(R,T);
            this.block.pose.parent = this.headCam.pose;
    %}
    
    %{
            desktop
            keyboard
                                    
            % Draw the block reprojected in the camera
            pos = getPosition(this.block);
            
            X = reshape(pos(:,1), 4, []);
            Y = reshape(pos(:,2), 4, []);
            Z = reshape(pos(:,3), 4, []);
            
            [u,v] = this.headCam.projectPoints(X, Y, Z);
            
            if any(u(:)>=1) || any(u(:)<=this.trackingResolution(1)) || ...
                    any(v(:)>=1) || any(v(:)<=this.trackingResolution(2))
                
                w = zeros(size(u));
                h_block = findobj(this.h_axes, 'Tag', 'Block');
                if isempty(h_block)
                    patch(u,v,w, this.block.color, 'FaceAlpha', 0.3, ...
                        'LineWidth', 3, 'Parent', this.h_axes, 'Tag', 'Block');
                else
                    set(h_block, 'XData', u, 'YData', v, 'ZData', w);
                end
                
                % Plot block origin:
                h_origin = findobj(this.h_axes, 'Tag', 'BlockOrigin');
                if isempty(h_origin)
                    plot(u(1), v(1), 'o', 'MarkerSize', 12, ...
                        'MarkerEdgeColor', 'k', 'MarkerFaceColor', this.block.color, ...
                        'LineWidth', 2, ...
                        'Parent', this.h_axes, 'Tag', 'BlockOrigin');
                else
                    set(h_origin, 'XData', u(1), 'YData', v(1));
                end
                
            end
            
            % Draw the block's markers reprojected in the camera
            for i_marker = 1:this.block.numMarkers
                marker = this.block.markers{i_marker};
                                
                % Get the position of the marker w.r.t. the camera, ready for
                % projection:
                P = getPosition(marker, this.headCam.pose);
                
                [u,v] = this.headCam.projectPoints(P);
                
                if any(u(:)>=1) || any(u(:)<=this.trackingResolution(1)) || ...
                        any(v(:)>=1) || any(v(:)<=this.trackingResolution(2))
                    
                    w = .01*ones(size(u)); % put slightly in front of blocks
                    TagStr = sprintf('Marker%d', i_marker);
                    h_marker = findobj(this.h_axes, 'Tag', TagStr);
                    if isempty(h_marker)
                        patch(u([1 3 4 2]),v([1 3 4 2]),w, 'w', ...
                            'FaceColor', 'none', 'EdgeColor', 'w', ...
                            'LineWidth', 3, 'Parent', this.h_axes, 'Tag', TagStr);
                    else
                        set(h_marker, 'XData', u([1 3 4 2]), ...
                            'YData', v([1 3 4 2]), 'ZData', w)
                    end
                    
                end
            end % FOR each marker
    %}
    
    %desktop
    %keyboard
    
    %{
    % Now switch to block with respect to World, instead of camera
    this.block.pose = this.block.pose.getWithRespectTo('World');
    
    blockNode = wb_supervisor_node_get_from_def('FuelBlock');
    wb_supervisor_field_set_sf_vec3f( ...
        wb_supervisor_node_get_field(blockNode, 'obsTranslation'), ...
        [this.block.pose.T(1) -this.block.pose.T(3) this.block.pose.T(2)]/1000);
    
    Raxis = this.block.pose.axis;
    Rangle = this.block.pose.angle;
    wb_supervisor_field_set_sf_rotation( ...
        wb_supervisor_node_get_field(blockNode, 'obsRotation'), ...
        [Raxis(1) -Raxis(3) Raxis(2) Rangle]);
    
    wb_supervisor_field_set_sf_float( ...
        wb_supervisor_node_get_field(blockNode, 'obsTransparency'), 0.0);
    %}
    this.block.pose = this.block.pose.getWithRespectTo(this.robotPose);
    
end % updateBlockPose()