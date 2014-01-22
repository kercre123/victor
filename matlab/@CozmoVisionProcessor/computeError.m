function [distError, midPointErr, angleError] = computeError(this)

% Compute the error signal according to the current tracking result
switch(this.trackerType)
    case 'affine'
        corners = this.LKtracker.corners;
        
        % TODO: why do i still need this rotation fix code? I'm
        % using the unordered marker corners above and i
        % thought that would fix it...
        %
        % Block may be rotated with top side of marker not
        % facing up, so reorient to make sure we got top
        % corners
        [th,~] = cart2pol(corners(:,1)-mean(corners(:,1)), ...
            corners(:,2)-mean(corners(:,2)));
        [~,sortIndex] = sort(th);
        
        lineLeft = corners(sortIndex(4),:);
        lineRight = corners(sortIndex(3),:);
        
        assert(lineRight(1) > lineLeft(1), ...
            ['lineRight corner should be to the right ' ...
            'of the lineLeft corner.']);
        
        % Get the angle from vertical of the top bar of the
        % marker we're tracking
        L = sqrt(sum( (lineRight-lineLeft).^2) );
        angleError = -asin( (lineRight(2)-lineLeft(2)) / L);

        % Scale angle by some amount to get a better approximation of the actual angular
        % deviation from normal that we want. 
        % This should probably be based on things like camera parameters, 
        % the height difference between the camera and the expected height of the line, 
        % and the current distance to the block, but for now just use a magic number that 
        % seems to mostly work for blocks at ground level for typical docking distances.
        angleError = angleError * 4;
        
        fx = this.headCam.focalLength(1) * this.trackingResolution(1)/this.headCam.ncols;
        currentDistance = BlockMarker3D.ReferenceWidth * fx / L;
        distError = currentDistance; % - CozmoVisionProcessor.LIFT_DISTANCE;
        
        % TODO: should i be comparing to ncols/2 or calibration center?
        midPointErr = -( (lineRight(1)+lineLeft(1))/2 - ...
            this.trackingResolution(1)/2 );
        midPointErr = midPointErr * currentDistance / fx;
        
    case 'homography'
        
        % Note that LKtracker internally does recentering -- we
        % need to factor that in here as well. Thus the C term.
             
        %scale = this.headCam.ncols/this.trackingResolution(1);
        C = [1 0 -this.LKtracker.xcen; 0 1 -this.LKtracker.ycen; 0 0 1];
        K = this.headCam.calibrationMatrix;
        H = K\(C\this.LKtracker.tform*C*K*this.H_init);
        
        % This computes the equivalent H, but is more expensive
        % since it involves an SVD internally to compute the
        % homography instead of just chaining together the
        % initial homography with the current one already found
        % by the tracker, as above.
        %H = compute_homography(this.K\[LKtracker.corners';ones(1,4)], ...
        %        this.marker3d(:,1:2)');
        
        this.updateBlockPose(H);
        
        blockWRTrobot = this.block.pose.getWithRespectTo(this.robotPose);
        distError     = blockWRTrobot.T(1);
        midPointErr   = blockWRTrobot.T(2);
        angleError    = atan2(blockWRTrobot.Rmat(2,1), blockWRTrobot.Rmat(1,1));
        
    otherwise
        error(['Not sure how to compute an error signal ' ...
            'when using a %s tracker.'], this.trackerType);
end % SWITCH(trackerType)


% Update the error displays
set(this.h_angleError, 'XData', [0 angleError*180/pi]);
h = get(this.h_angleError, 'Parent');
title(h, sprintf('AngleErr = %.1fdeg', angleError*180/pi), 'Back', 'w');

set(this.h_distError, 'YData', [0 distError]);
h = get(this.h_distError, 'Parent');
title(h, sprintf('DistToGo = %.1fmm', distError), 'Back', 'w');

set(this.h_leftRightError, 'XData', [0 -midPointErr]); % sign flip b/c negative y direction is to the right
h = get(this.h_leftRightError, 'Parent');
title(h, sprintf('LeftRightErr = %.1fmm', midPointErr), 'Back', 'w');


end % FUNCTION computeError()


