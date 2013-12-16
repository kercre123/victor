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
        
        upperLeft = corners(sortIndex(1),:);
        upperRight = corners(sortIndex(2),:);
        
        assert(upperRight(1) > upperLeft(1), ...
            ['UpperRight corner should be to the right ' ...
            'of the UpperLeft corner.']);
        
        % Get the angle from vertical of the top bar of the
        % marker we're tracking
        L = sqrt(sum( (upperRight-upperLeft).^2) );
        angleError = -asin( (upperRight(2)-upperLeft(2)) / L);
        
        currentDistance = BlockMarker3D.ReferenceWidth * this.calibrationMatrix(1,1) / L;
        distError = currentDistance - CozmoVisionProcessor.LIFT_DISTANCE;
        
        % TODO: should i be comparing to ncols/2 or calibration center?
        midPointErr = -( (upperRight(1)+upperLeft(1))/2 - ...
            this.trackingResolution(1)/2 );
        midPointErr = midPointErr * currentDistance / this.calibrationMatrix(1,1);
        
    case 'homography'
        
        error('ComputeError() for homography not fully implemented yet.');
        
        % Note that LKtracker internally does recentering -- we
        % need to factor that in here as well. Thus the C term.
        C = [1 0 -this.LKtracker.xcen; 0 1 -this.LKtracker.ycen; 0 0 1];
        H = this.calibrationMatrix\(C\this.LKtracker.tform*C*this.calibrationMatrix*this.H_init);
        
        % This computes the equivalent H, but is more expensive
        % since it involves an SVD internally to compute the
        % homography instead of just chaining together the
        % initial homography with the current one already found
        % by the tracker, as above.
        %H = compute_homography(this.K\[LKtracker.corners';ones(1,4)], ...
        %        this.marker3d(:,1:2)');
        
        this.updateBlockPose(H);
        
        if this.drawPose
            this.drawCamera();
            this.drawRobot();
        end
        
        blockWRTrobot = this.blockPose.getWithRespectTo(this.robotPose);
        distError     = blockWRTrobot.T(1) - CozmoDocker.LIFT_DISTANCE;
        midPointErr   = blockWRTrobot.T(2);
        angleError    = atan2(blockWRTrobot.Rmat(2,1), blockWRTrobot.Rmat(1,1)) + pi/2;
        
    otherwise
        error(['Not sure how to compute an error signal ' ...
            'when using a %s tracker.'], this.trackerType);
end % SWITCH(trackerType)

%{
            % Update the error displays
            set(this.h_angleError, 'XData', [0 angleError*180/pi]);
            h = get(this.h_angleError, 'Parent');
            title(h, sprintf('AngleErr = %.1fdeg', angleError*180/pi), 'Back', 'w');
            
            set(this.h_distError, 'YData', [0 distError]);
            h = get(this.h_distError, 'Parent');
            title(h, sprintf('DistToGo = %.1fmm', distError), 'Back', 'w');
            
            set(this.h_leftRightError, 'XData', [0 midPointErr]);
            h = get(this.h_leftRightError, 'Parent');
            title(h, sprintf('LeftRightErr = %.1fmm', midPointErr), 'Back', 'w');
%}

end % FUNCTION computeError()