function docked = dockWithBlockStep(this)

DEBUG_DISPLAY = true;
convergenceThreshold = 0.1; % in velocity
% TODO: get this from current lift position:
liftDockHeight = this.appearance.BodyHeight/2;
liftDockDistance = this.appearance.BodyLength/2 + 15;

docked = false;

% Figure out where we _want_ to see the block in order to dock with it,
% according to our head pose and the block's docking anchor.
% TODO: update this to use the docking anchor of the correct face, not the
% marker
assert(~isempty(this.virtualBlock), ...
    'Virtual (docking) block should be set by now.');

desiredFacePose = Pose([0 0 0], [0 liftDockDistance liftDockHeight+this.appearance.BodyHeight/2]);
this.virtualBlock.pose = desiredFacePose * this.virtualFace.pose.inv;
this.virtualBlock.pose.parent = this.pose;
dots3D_goal = this.virtualFace.getPosition(this.camera.pose, 'DockingDots');
[u_goal, v_goal] = this.camera.projectPoints(dots3D_goal);

if any(v_goal > 0.9*this.camera.nrows)
    fprintf('Virtual docking dots too low for current head pose.  Tilting head down...\n');
    this.tiltHead(-.01);
    return;
end

assert(all(u_goal >= 1 & v_goal >= 1 & ...
    u_goal <= this.camera.ncols & v_goal <= this.camera.nrows), ...
    'Projected virtual docking dots should be in the field of view!');

face3D = this.dockingFace.getPosition(this.camera.pose);
[u_face, v_face] = this.camera.projectPoints(face3D);
if any(u_face < 1 | u_face > this.camera.ncols | v_face < 1 | v_face > this.camera.nrows)
    % Close up tracking of docking dots and updating block pose estimate:
    
    dockingBox3D = this.dockingFace.getPosition(this.camera.pose, 'DockingDotsBoundingBox');
    [u_box, v_box] = this.camera.projectPoints(dockingBox3D);
    mask = roipoly(this.camera.image, u_box([1 2 4 3]), v_box([1 2 4 3]));
    
    % Simulate blur from defocus
    % TODO: remove this!
    img = separable_filter(this.camera.image, gaussian_kernel(sqrt(sum(mask(:)))/8));
    try
        [u,v] = findDots(img, mask);
    catch E
        warning('findDots() failed: %s', E.message);
        docked = false;
        return;
    end
    
%     % Figure out where the dots are in 3D space, in camera's coordinate frame:
%     dotsPose = this.camera.computeExtrinsics([u v], this.dockingFace.dockingTarget);
%     
%     % Get the dots' pose in World coordinates using the pose tree:
%     dotsPose = dotsPose.getWithRespectTo('World');
%     
%     % We measured the _dots_ pose.  We are updating the _block_ pose:
%     this.dockingBlock.pose = dotsPose * this.dockingFace.pose.inv;
%     this.world.updateObsBlockPose(this.dockingBlock.blockType, this.dockingBlock.pose);
    
else
    % We are still far enough away to be updating the block's pose using
    % the full marker
        
    % Find the block's docking dots 3D locations, w.r.t. our head camera:
    dots3D = this.dockingFace.getPosition(this.camera.pose, 'DockingDots');
    
    % Project them into the image:
    [u,v] = this.camera.projectPoints(dots3D);
    
    if DEBUG_DISPLAY
        img = this.camera.image;
        u_box = []; v_box = [];
    end
end

% Make sure they are (all) within view!
if ~all(u >= 1 & u <= this.camera.ncols & ...
        v >= 1 & v <= this.camera.nrows)
    error('Requested face of requested Block is not in view!');
end

[leftVelocity, rightVelocity] = DockingController([u(:) v(:)], [u_goal(:) v_goal(:)]);

fprintf('Left/right velocities = %.2f/%.2f\n', leftVelocity, rightVelocity);

this.drive(leftVelocity, rightVelocity);

docked =  abs(leftVelocity) < convergenceThreshold && ...
    abs(rightVelocity) < convergenceThreshold;

if DEBUG_DISPLAY
    h_fig = namedFigure('Docking');
    h_axes = findobj(h_fig, 'Type', 'axes');
    if isempty(h_axes)
        h_axes = axes('Parent', h_fig);
        imagesc(img, 'Parent', h_axes, 'Tag', 'DockingImage');
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
        if ~isempty(u_box)
            set(findobj(h_axes, 'Tag', 'DockingBoundingBox'), ...
                'XData', u_box([1 2 4 3 1]), 'YData', v_box([1 2 4 3 1]));
        end
    end
    title(h_axes, sprintf('Left/right velocities = %.2f/%.2f\n', ...
        leftVelocity, rightVelocity));
    
end
    
end % FUNCTION dockWithBlock()


function [leftMotorVelocity, rightMotorVelocity] = DockingController(obsTarget, goalTarget)

% High-level:
% - If the observed target is to the right of the goal location for that
%   target, we need to turn left, meaning we need the right motor to rotate
%   forwards and the left motor to rotate backwards.
% - If the observed target is above the goal location for that target, we
%   need to back up, so we need to rotate both motors backwards.

K_turn  = 0.01;
K_dist  = 0.02;
% K_scale = 0.2;

obsMean  = mean(obsTarget,1);
goalMean = mean(goalTarget,1);

horizontalError = obsMean(1) - goalMean(1);
turnVelocityLeft  = K_turn*horizontalError;
turnVelocityRight = -turnVelocityLeft;

% HACK: Only update distance if our heading is decent. I.e., turn towards
% the target and _then_ start driving to it.
if horizontalError < 10 
    %obsScale  = sqrt(sum( (obsTarget(1,:)-obsTarget(3,:)).^2 ));
    %goalScale = sqrt(sum( (goalTarget(1,:)-goalTarget(3,:)).^2 ));
    %scaleError = goalScale - obsScale;
    %distanceVelocity = K_scale*scaleError;
    verticalError = obsMean(2) - goalMean(2);
    distanceVelocity = -K_dist*verticalError;
else
    distanceVelocity = 0;
end

leftMotorVelocity  = turnVelocityLeft  + distanceVelocity;
rightMotorVelocity = turnVelocityRight + distanceVelocity;

end