function docked = dockWithBlockStep(this)

DEBUG_DISPLAY = true;
K_headTilt = 0.0005;
convergenceThreshold = 0.1; % in velocity
% TODO: get this from current lift position:

docked = false;

% Project the virtual block's docking target into the camera image
dots3D_goal = this.virtualFace.getPosition(this.camera.pose, 'DockingTarget');
[u_goal, v_goal] = this.camera.projectPoints(dots3D_goal);

% If it's above/below the camera, tilt the head up/down until we can see it
if any(v_goal > 0.9*this.camera.nrows)
    fprintf('Virtual docking dots too low for current head pose.  Tilting head down...\n');
    
    % Simple proportional control:
    e = max(v_goal) - 0.9*this.camera.nrows;
    this.tiltHead(-K_headTilt*e);
    
elseif any(v_goal < 0.1*this.camera.nrows)
    fprintf('Virtual docking dots too high for current head pose.  Tilting head up...\n');
    
    % Simple proportional control:
    e = 0.1*this.camera.nrows - min(v_goal);
    this.tiltHead(K_headTilt*e);
    
else
    
    % assert(all(u_goal >= 1 & v_goal >= 1 & ...
    %     u_goal <= this.camera.ncols & v_goal <= this.camera.nrows), ...
    %     'Projected virtual docking dots should be in the field of view!');
    if ~all(u_goal >= 1 & v_goal >= 1 & ...
            u_goal <= this.camera.ncols & v_goal <= this.camera.nrows)
        
        desktop
        keyboard
        error('Projected virtual docking dots should be in the field of view!');
    end
    
    face3D = this.dockingFace.getPosition(this.camera.pose);
    [u_face, v_face] = this.camera.projectPoints(face3D);
    if any(u_face < 1 | u_face > this.camera.ncols | v_face < 1 | v_face > this.camera.nrows)
        % We can't see the whole marker anymore, so start using the close-up
        % docking target:
        
        % NOTE: the extra three returns here are only for debug display below!
        [u,v, u_box,v_box,img] = this.dockingFace.findDockingTarget(this.camera);
        
        % Figure out where the dots are in 3D space, in camera's coordinate frame:
        dots3D = this.dockingFace.getPosition(this.dockingBlock.pose, 'DockingTarget');
        dotsPose = this.camera.computeExtrinsics([u(:) v(:)], dots3D);
        
        %     % Get the dots' pose in World coordinates using the pose tree:
        %     this.dockingBlock.pose = dotsPose.getWithRespectTo('World');
        %
        %     this.world.updateObsBlockPose(this.dockingBlock.blockType, ...
        %         this.dockingBlock.pose, this.world.SelectedBlockColor);
        
    else
        % We are still far enough away to be using the full marker to determine
        % where the docking target is
        
        % Find the block's docking dots 3D locations, w.r.t. our head camera:
        dots3D = this.dockingFace.getPosition(this.camera.pose, 'DockingTarget');
        dotsPose = this.dockingFace.pose.getWithRespectTo(this.camera.pose);
        
        % Project them into the image:
        [u,v] = this.camera.projectPoints(dots3D);
        
        if DEBUG_DISPLAY
            img = this.camera.image;
            u_box = nan; v_box = nan;
        end
    end
    
    % Make sure they are (all) within view!
    if ~all(u >= 1 & u <= this.camera.ncols & ...
            v >= 1 & v <= this.camera.nrows)
        error('Requested face of requested Block is not in view!');
    end
    
    % Get the angle between the horizontal (in-plane) rotation of the block's
    % face we are docking with and the robot's heading (in camera coordinates)
    % TODO: verify this is correct
    headingError = acos(dotsPose.Rmat(3,2));
    
    % Compute a control signal based on the observed position of the
    % block's docking target vs. the virtual (desired) position of that
    % target...
    [leftVelocity, rightVelocity] = DockingController([u(:) v(:)], [u_goal(:) v_goal(:)], headingError);
    
    % ...and use that control signal to move the robot.
    this.drive(leftVelocity, rightVelocity);
    
    % Are we there yet?
    docked =  abs(leftVelocity) < convergenceThreshold && ...
        abs(rightVelocity) < convergenceThreshold;
    
    if DEBUG_DISPLAY
        h_fig = namedFigure('Docking');
        if docked
            close(h_fig);
        else
            h_axes = findobj(h_fig, 'Type', 'axes');
            if isempty(h_axes)
                h_axes = axes('Parent', h_fig);
                imagesc(img, 'Parent', h_axes, 'Tag', 'DockingImage');
                colormap(h_fig, gray)
                hold(h_axes, 'on')
                plot(u_goal, v_goal, 'bo', 'Parent', h_axes, 'Tag', 'DockingGoal');
                plot(u, v, 'rx', 'Parent', h_axes, 'Tag', 'DockingCurrent');
                plot(u_box, v_box, 'r--', 'Parent', h_axes, 'Tag', 'DockingBoundingBox');
            else
                set(findobj(h_axes, 'Tag', 'DockingImage'), ...
                    'CData', img);
                set(findobj(h_axes, 'Tag', 'DockingGoal'), ...
                    'XData', u_goal, 'YData', v_goal);
                set(findobj(h_axes, 'Tag', 'DockingCurrent'), ...
                    'XData', u, 'YData', v);
                if ~isnan(u_box)
                    set(findobj(h_axes, 'Tag', 'DockingBoundingBox'), ...
                        'XData', u_box([1 2 4 3 1]), 'YData', v_box([1 2 4 3 1]));
                end
            end
            title(h_axes, sprintf('Left/right velocities = %.2f/%.2f\n', ...
                leftVelocity, rightVelocity));
        end
    end % IF DEBUG_DISPLAY
    
end % IF virtual docking dots are in view
    
end % FUNCTION dockWithBlock()


function [leftMotorVelocity, rightMotorVelocity] = DockingController(obsTarget, goalTarget, headingError)

% High-level:
% - If the observed target is to the right of the goal location for that
%   target, we need to turn left, meaning we need the right motor to rotate
%   forwards and the left motor to rotate backwards.
% - If the observed target is above the goal location for that target, we
%   need to back up, so we need to rotate both motors backwards.

% TODO: incorporate heading error?

K_turn  = 0.01;
K_dist  = 0.015;
maxSpeed = 7;

% K_scale = 0.2;

% Note this is simply using the centroid of the target, irrespective of
% whether it is a single dot or multiple dots!
obsMean  = mean(obsTarget,1);
goalMean = mean(goalTarget,1);

horizontalError = obsMean(1) - goalMean(1);
turnVelocityLeft  = K_turn*horizontalError;
turnVelocityRight = -turnVelocityLeft;

% HACK: Only update distance if our heading is decent. I.e., turn towards
% the target and _then_ start driving to it.
if abs(horizontalError) < 10 
    %obsScale  = sqrt(sum( (obsTarget(1,:)-obsTarget(3,:)).^2 ));
    %goalScale = sqrt(sum( (goalTarget(1,:)-goalTarget(3,:)).^2 ));
    %scaleError = goalScale - obsScale;
    %distanceVelocity = K_scale*scaleError;
    verticalError = obsMean(2) - goalMean(2);
    distanceVelocity = -K_dist*verticalError;
else
    distanceVelocity = 0;
end

leftMotorVelocity  = max(-maxSpeed, min(maxSpeed, turnVelocityLeft  + distanceVelocity));
rightMotorVelocity = max(-maxSpeed, min(maxSpeed, turnVelocityRight + distanceVelocity));

end % FUNCTION DockingController()