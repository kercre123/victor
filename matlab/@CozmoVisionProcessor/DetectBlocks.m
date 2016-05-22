function DetectBlocks(this, img, timestamp)

markers = simpleDetector(img);

delete(findobj(this.h_axes, 'Tag', 'DetectedMarkers'));

cozmoName = wb_robot_get_controller_arguments();
cozmoNode = wb_supervisor_node_get_from_def(cozmoName);

if cozmoNode.isNull
    error('No CozmoBot node DEF "%s" found.', cozmoName);
end

T = 1000*wb_supervisor_field_get_sf_vec3f(wb_supervisor_node_get_field(cozmoNode, 'translation'));
R_true = wb_supervisor_field_get_sf_rotation(wb_supervisor_node_get_field(cozmoNode, 'rotation'));
wb_supervisor_set_label(1, ...
    sprintf('True Pose (mm): (%.1f, %.1f, %.1f), %.1fdeg@(%.1f,%.1f,%.1f)', ...
    T(1), T(2), T(3), R_true(4)*180/pi, R_true(1), R_true(2), R_true(3)), ...
    .5, 0.09, 0.08, [1 0 0], 0);
x_true = [get(this.h_path(1), 'XData') T(1)];
y_true = [get(this.h_path(1), 'YData') T(2)];
set(this.h_path(1), 'XData', x_true, 'YData', y_true);

% Send a message about each marker we found
for i = 1:length(markers)
    
    switch(class(markers{i}))
            
        case {'VisionMarker', 'VisionMarkerTrained'}
            
            if isa(markers{i}, 'VisionMarker')
                matchName = 'UNKNOWN';
                
                if ~isempty(this.markerLibrary)
                    match = this.markerLibrary.IdentifyMarker(markers{i});
                    if ~isempty(match)
                        matchName = match.name;
                    end
                end
                displayName = matchName;
                
                code = markers{i}.byteArray';
            else
                displayName = markers{i}.codeName;
                if isfield(this.messageIDs, displayName)
                    code = uint16(this.messageIDs.(displayName));
                else
                    code = uint16(0);
                end
                
            end
                
            markers{i}.Draw('Parent', this.h_axes, 'String', displayName, 'Tag', 'DetectedMarkers');
            
            % IMPORTANT NOTE:
            % The corner coordinates are being sent at the resolution of
            % the calibration information! (The basestation will have 
            % received the same calibration information but does not need
            % to know that detection actually occured in a scaled image
            % when we use that calibration data to compute pose.)
            scale = this.headCam.ncols / this.detectionResolution(1);
            scaledCorners = scale * markers{i}.corners;
            msgStruct = struct( ...
                'timestamp', timestamp, ...
                'x_imgUpperLeft',  single(scaledCorners(1,1)), ...
                'y_imgUpperLeft',  single(scaledCorners(1,2)), ...
                'x_imgLowerLeft',  single(scaledCorners(2,1)), ...
                'y_imgLowerLeft',  single(scaledCorners(2,2)), ...
                'x_imgUpperRight', single(scaledCorners(3,1)), ...
                'y_imgUpperRight', single(scaledCorners(3,2)), ...
                'x_imgLowerRight', single(scaledCorners(4,1)), ...
                'y_imgLowerRight', single(scaledCorners(4,2)), ...
                'code', code);
            
            packet = this.SerializeMessageStruct(msgStruct);
            this.SendPacket('CozmoMsg_VisionMarker', packet);
            
            if isa(markers{i}, 'VisionMarker')
                
                if strncmp(matchName, 'ANKI-MAT', 8)
                    % TODO: this localization computation should get moved to basestation
                    
                    %desktop
                    %keyboard
                    
                    % This is a Mat marker, figure out where we are relative to it
                    %                match.Origin(3) = -CozmoVisionProcessor.WHEEL_RADIUS;
                    
                    %                if false
                    %                    markerCornersWorld = [-.5 -.5 0; -.5 .5 0; .5 -.5 0; .5 .5 0]*match.Size;
                    %                else
                    %                    % Needed when using simulator??
                    %                    match.Origin(2) = -match.Origin(2);
                    %                    markerCornersWorld = [-.5 .5 0; -.5 -.5 0; .5 .5 0; .5 -.5 0]*match.Size;
                    %                end
                    
                    %markerWorldPose = Pose(match.Angle*pi/180*[0 0 1], match.Origin);
                    %markerCornersWorld = markerWorldPose.applyTo(markerCornersWorld);
                    
                    markerCornersWorld = match.GetPosition('World');
                    
                    if false % in simulator
                        markerCornersWorld(:,2) = -markerCornersWorld(:,2);
                    end
                    
                    % Simulate noise on the head angle with std. dev. of 1
                    % degree
                    this.setHeadAngle(-.251); % + this.headAngleNoiseStdDev*randn*pi/180);
                    
                    %{
               % Focal length / camera center noise
               errFrac = .005; % +/- 0.5% error
               fnoise = 1 + (2*errFrac*rand(1,2)-errFrac);
               cnoise = 1 + (2*errFrac*rand(1,2)-errFrac);
                    %}
                    
                    % Note that we use corners scaled to be at the resolution the
                    % camera calibration information was given at
                    markerPose_wrt_camera = this.headCam.computeExtrinsics( ...
                        scaledCorners, markerCornersWorld);
                    
                    newPose = this.robotPose.getWithRespectTo(markerPose_wrt_camera);
                    this.robotPose.update(newPose.Rmat, newPose.T + [0;0;CozmoVisionProcessor.WHEEL_RADIUS]);
                    
                    wb_supervisor_set_label(0, ...
                        sprintf('Est. Pose (mm): (%.1f, %.1f, %.1f), %.1fdeg@(%.1f,%.1f,%.1f)', ...
                        this.robotPose.T(1), this.robotPose.T(2), this.robotPose.T(3), ...
                        this.robotPose.angle*180/pi, ...
                        this.robotPose.axis(1), this.robotPose.axis(2), this.robotPose.axis(3)), ...
                        .5, 0, 0.08, [1 0 0], 0);
                    
                    x_est = [get(this.h_path(2), 'XData') this.robotPose.T(1)];
                    y_est = [get(this.h_path(2), 'YData') this.robotPose.T(2)];
                    set(this.h_path(2), 'XData', x_est, 'YData', y_est);
                    
                    t   = [get(this.h_positionError, 'XData') wb_robot_get_time()];
                    err = [get(this.h_positionError, 'YData') ...
                        sqrt((x_est(end)-x_true(end))^2 + (y_est(end)-y_true(end))^2)];
                    set(this.h_positionError, 'XData', t, 'YData', err);
                    
                    err = [get(this.h_orientationError, 'YData') ...
                        angleDiff(abs(R_true(4)), abs(this.robotPose.angle))*180/pi];
                    set(this.h_orientationError, 'XData', t, 'YData', err);
                    
                end
                
                
                if ~isempty(this.markerToTrack) && ...
                        isequal(markers{i}.byteArray(:), this.markerToTrack(:))
                    
                    this.marker3d = match.size/2 * [0 1 1; 0 1 -1; 0 -1 1; 0 -1 -1];
                    
                    % Initialize the tracker
                    this.LKtracker = LucasKanadeTracker(img, ...
                        markers{i}.corners, ...
                        'Type', this.trackerType, 'RidgeWeight', 1e-3, ...
                        'DebugDisplay', false, 'UseBlurring', false, ...
                        'UseNormalization', true, ...
                        'TrackingResolution', this.trackingResolution, ...
                        'SampleNearEdges', true);
                    
                    if strcmp(this.trackerType, 'homography')
                        scale = this.trackingResolution(1) / this.headCam.ncols;
                        K = diag([scale scale 1])*this.headCam.calibrationMatrix;
                        %K = this.headCam.calibrationMatrix;
                        %scaledCorners = scale*(this.LKtracker.corners-1)+1;
                        this.H_init = compute_homography( ...
                            K\[this.LKtracker.corners'; ones(1,4)], ...
                            this.marker3d(:,2:3)');
                    end
                    
                    % Let the robot know we've initialized the
                    % tracker
                    msgStruct = struct('success', uint8(true));
                    
                    packet = this.SerializeMessageStruct(msgStruct);
                    this.SendPacket('CozmoMsg_TemplateInitialized', packet);
                    
                    % Show the tracking template
                    template = this.LKtracker.target{this.LKtracker.finestScale};
                    set(this.h_template, 'CData', template);
                    set(this.h_templateAxes, 'Visible', 'on', ...
                        'XLim', [0.5 size(template,2)+.5], ...
                        'YLim', [0.5 size(template,1)+.5]);
                    
                    % We are about to switch to tracking mode, so reduce the desired
                    % buffer read size
                    this.desiredBufferSize = this.computeDesiredBufferLength(this.trackingResolution);
                    
                end % if we have a marker to track and this one matches it
                
            end % IF this is a vision marker
            
        otherwise
            error('Unknown detected marker type "%s".', class(markers{i}));
            
    end % SWITCH(class(markers{i}))
    
end % FOR each marker

set(this.h_title, 'String', ...
    sprintf('Detected %d Markers', length(markers)));

% Send a message indicating there are no more marker messages coming
msgStruct = struct('numMarkers', uint8(length(markers)) );

packet = this.SerializeMessageStruct(msgStruct);
this.SendPacket('CozmoMsg_TotalVisionMarkersSeen', packet);


end % FUNCTION DetectBlocks()