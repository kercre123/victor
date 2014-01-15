function DetectBlocks(this, img, timestamp)

markers = simpleDetector(img);

delete(findobj(this.h_axes, 'Tag', 'DetectedMarkers'));

cozmoNode = wb_supervisor_node_get_from_def('Cozmo');
T = wb_supervisor_field_get_sf_vec3f(wb_supervisor_node_get_field(cozmoNode, 'translation'));
T = 1000*[T(1) -T(3) T(2)];
R = wb_supervisor_field_get_sf_rotation(wb_supervisor_node_get_field(cozmoNode, 'rotation'));
wb_supervisor_set_label(1, ...
    sprintf('True Pose (mm): (%.1f, %.1f, %.1f), %.1fdeg@(%.1f,%.1f,%.1f)', ...
    T(1), T(2), T(3), R(4)*180/pi, R(1), -R(3), R(2)), ...
    .5, 0.09, 0.08, [1 0 0], 0);

% Send a message about each marker we found
for i = 1:length(markers)
    
    switch(class(markers{i}))
        
        case 'VisionMarker'
            matchName = 'UNKNOWN';
            
            if ~isempty(this.markerLibrary)
                match = this.markerLibrary.IdentifyMarker(markers{i});
                if ~isempty(match)
                    matchName = match.Name;
                end
            end
                
            markers{i}.Draw('Parent', this.h_axes, 'String', matchName, 'Tag', 'DetectedMarkers');
            
            if strncmp(matchName, 'ANKI-MAT', 8)
                
               %desktop
               %keyboard
               
               % This is a Mat marker, figure out where we are relative to it
               match.Origin(3) = -CozmoVisionProcessor.WHEEL_RADIUS;
               
               if false
                   markerCornersWorld = [-.5 -.5 0; -.5 .5 0; .5 -.5 0; .5 .5 0]*match.Size;
               else
                   % Needed when using simulator??
                   match.Origin(2) = -match.Origin(2);
                   markerCornersWorld = [-.5 .5 0; -.5 -.5 0; .5 .5 0; .5 -.5 0]*match.Size;
               end
               
               markerWorldPose = Pose(match.Angle*pi/180*[0 0 1], match.Origin);
               markerCornersWorld = markerWorldPose.applyTo(markerCornersWorld);
               
               % Head cam is currently calibrated at 80x60, so need to
               % createa temporary camera calibrated at correct resolution
               tempCam = Camera('resolution', [320 240], ...
                   'calibration', struct('fc', 4*[this.headCalibrationMatrix(1,1) this.headCalibrationMatrix(2,2)], ...
                   'cc', 4*[this.headCalibrationMatrix(1,3) this.headCalibrationMatrix(2,3)], ...
                   'kc', zeros(5,1), ...
                   'alpha_c', 0));
               tempCam.pose = this.headCam.pose;

               markerPose_wrt_camera = tempCam.computeExtrinsics( ...
                   markers{i}.corners, markerCornersWorld);
               
               newPose = this.robotPose.getWithRespectTo(markerPose_wrt_camera);
               this.robotPose.update(newPose.Rmat, newPose.T + [0;0;CozmoVisionProcessor.WHEEL_RADIUS]);
               
               wb_supervisor_set_label(0, ...
                   sprintf('Est. Pose (mm): (%.1f, %.1f, %.1f), %.1fdeg@(%.1f,%.1f,%.1f)', ...
                   this.robotPose.T(1), this.robotPose.T(2), this.robotPose.T(3), ...
                   this.robotPose.angle*180/pi, ...
                   this.robotPose.axis(1), this.robotPose.axis(2), this.robotPose.axis(3)), ...
                   .5, 0, 0.08, [1 0 0], 0);
               
            end
            
            
        case 'BlockMarker2D'
            markers{i}.draw('where', this.h_axes, 'Tag', 'DetectedMarkers');
            
            msgStruct = struct( ...
                'timestamp', timestamp, ...
                'headAngle', single(0), ... ???
                'x_imgUpperLeft',  single(markers{i}.corners(1,1)), ...
                'y_imgUpperLeft',  single(markers{i}.corners(1,2)), ...
                'x_imgLowerLeft',  single(markers{i}.corners(2,1)), ...
                'y_imgLowerLeft',  single(markers{i}.corners(2,2)), ...
                'x_imgUpperRight', single(markers{i}.corners(3,1)), ...
                'y_imgUpperRight', single(markers{i}.corners(3,2)), ...
                'x_imgLowerRight', single(markers{i}.corners(4,1)), ...
                'y_imgLowerRight', single(markers{i}.corners(4,2)), ...
                'blockType',       uint16(markers{i}.blockType), ...
                'faceType',        uint8(markers{i}.faceType), ...
                'upDirection',     uint8(markers{i}.upDirection));
            
            packet = this.SerializeMessageStruct(msgStruct);
            this.SendPacket('CozmoMsg_BlockMarkerObserved', packet);
            
            if ~isempty(this.dockingBlock) && ...
                    this.dockingBlock > 0 && ...
                    markers{i}.blockType == this.dockingBlock
                
                % Initialize the tracker
                this.LKtracker = LucasKanadeTracker(img, ...
                    markers{i}.unorderedCorners([3 1 4 2],:), ...
                    'Type', this.trackerType, 'RidgeWeight', 1e-3, ...
                    'DebugDisplay', false, 'UseBlurring', false, ...
                    'UseNormalization', true, ...
                    'TrackingResolution', this.trackingResolution);
                
                if strcmp(this.trackerType, 'homography')
                    this.H_init = compute_homography( ...
                        this.headCalibrationMatrix\[this.LKtracker.corners';ones(1,4)], ...
                        this.marker3d(:,1:2)');
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
            end
            
        otherwise
            error('Unknown detected marker type "%s".', class(markers{i}));
            
    end % SWITCH(class(markers{i}))
    
end % FOR each marker

set(this.h_title, 'String', ...
    sprintf('Detected %d Markers', length(markers)));

% Send a message indicating there are no more block
% marker messages coming
msgStruct = struct('numBlocks', uint8(length(markers)) );

packet = this.SerializeMessageStruct(msgStruct);
this.SendPacket('CozmoMsg_TotalBlocksDetected', packet);


end % FUNCTION DetectBlocks()