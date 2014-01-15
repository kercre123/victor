classdef CozmoVisionProcessor < handle
    
    properties(Constant = true)
        
        % Default header and footer:
        HEADER = char(sscanf('BEEFF0FF', '%2x'))'; 
        FOOTER = char(sscanf('FF0FFEEB', '%2x'))';
        
        % LUT for converting image format byte to resolution
        RESOLUTION_LUT = containers.Map( ...
            {char(sscanf('BA', '%2x')), ...
            char(sscanf('BC', '%2x')), ...
            char(sscanf('B8', '%2x')), ...
            char(sscanf('BD', '%2x')), ... 
            char(sscanf('B7', '%2x'))}, ...
            {[640 480], [320 240], [160 120], [80 60], [40 30]});    
        
        % Processing Commands
        % (These should match the USB_VISION_COMMANDs defined in hal.h)
        MESSAGE_COMMAND          = char(sscanf('DD', '%2x'));
        MESSAGE_ID_DEFINITION    = char(sscanf('D0', '%2x'));
        HEAD_CALIBRATION         = char(sscanf('C1', '%2x'));
        MAT_CALIBRATION          = char(sscanf('C2', '%2x'));
        DETECT_COMMAND           = char(sscanf('AB', '%2x'));
        SET_DOCKING_BLOCK        = char(sscanf('BC', '%2x'));
        TRACK_COMMAND            = char(sscanf('CD', '%2x'));
        MAT_ODOMETRY_COMMAND     = char(sscanf('DE', '%2x'));
        MAT_LOCALIZATION_COMMAND = char(sscanf('EF', '%2x'));
        DISPLAY_IMAGE_COMMAND    = char(sscanf('F0', '%2x'));
                    
        LIFT_DISTANCE = 34;  % in mm, forward from robot origin
        
        % Robot Geometry
        NECK_JOINT_POSITION = [-15.5  0  25]; % relative to robot origin
        HEAD_CAM_ROTATION   = [0 0 1; -1 0 0; 0 -1 0]; % rodrigues(-pi/2*[1 0 0])*rodrigues(pi/2*[0 1 0])
        %HEAD_CAM_ROTATION   = [0 1 0; 0 0 -1; -1 0 0];
        HEAD_CAM_POSITION   = [ 25  0  15]; % relative to neck joint
        WHEEL_RADIUS        = 15;
        
    end % Constant Properties
    
    properties
        TIME_STEP;
        serialDevice;
        serialBuffer;
        doEndianSwap;
        desiredBufferSize;
        
        verbosity;
        
        % LUT for enumerated message ID values 
        messageIDs;
        
        dockingBlock;
        
        % Camera calibration information:
        headCam;
        headCalibrationMatrix;
        %matCalibrationMatrix;
        matCamPixPerMM;
        
        blockPose;
        %headCamPose;
        robotPose;
        neckPose;
        marker3d;
        H_init;
        
        % On simulator, no need to flip.  On robot, need to flip
        flipImage;
        
        % For tracking
        LKtracker;
        trackerType;
        trackingResolution;
        
        detectionResolution;
        
        block;
        
        % For mat localization
        matLocalizationResolution;
        
        % Display handles
        h_fig;
        h_img;
        h_axes;
        h_title;
        h_templateAxes;
        h_template;
        h_track;
        h_distError;
        h_leftRightError;
        h_angleError;
        
        escapePressed;
    end
    
    properties(SetAccess = 'protected', Dependent = true)
        isDone;
    end
        
    methods
        
        function castedData = Cast(this, data, outputType)
         
            if ~isa(data, 'uint8') && this.doEndianSwap
               data = swapbytes(data); 
            end
                
            castedData = typecast(data, outputType);
            
            if ~strcmp(outputType, 'uint8') && this.doEndianSwap
                castedData = swapbytes(castedData);
            end
        end
        
        function L = computeDesiredBufferLength(this, expectedResolution)
           L = prod(expectedResolution) + ... actual image data 
               length(this.HEADER) + 1 + ... header bytes + command byte
               length(this.FOOTER) + 1 + ... footer bytes + command byte
               1 + ... resolution byte
               4; % timestamp bytes
        end
        
        function this = CozmoVisionProcessor(varargin)
            SerialDevice = [];
            FlipImage = true;
            TrackerType = 'affine';
            TrackingResolution = [80 60];
            MatLocalizationResolution = [320 240];
            DetectionResolution = [320 240];
            Verbosity = 1;
            DoEndianSwap = false; % false for simulator, true with Movidius
            TIME_STEP = 30; %#ok<PROP>

            parseVarargin(varargin{:});
            
            assert(isa(SerialDevice, 'serial') || ...
                isa(SerialDevice, 'SimulatedSerial'), ...
                'SerialDevice must be a serial or SimulatedSerial object.');
            this.serialDevice = SerialDevice;
            this.serialDevice.InputBufferSize = 640*480;
            
            fclose(this.serialDevice);
            fopen(this.serialDevice);
            
            this.doEndianSwap = DoEndianSwap;
                        
            this.flipImage = FlipImage;
            
            this.verbosity = Verbosity;
            this.TIME_STEP = TIME_STEP; %#ok<PROP>
            
            this.LKtracker = [];
            this.trackerType = TrackerType;
            this.trackingResolution = TrackingResolution;
            this.detectionResolution = DetectionResolution;
            
            this.desiredBufferSize = this.computeDesiredBufferLength(this.detectionResolution);
            
            this.matLocalizationResolution = MatLocalizationResolution;
            this.dockingBlock = 0;
                        
            % Set up Robot head camera geometry
            this.robotPose = Pose();
            this.neckPose = Pose([0 0 0], this.NECK_JOINT_POSITION);
            this.neckPose.parent = this.robotPose;
            
            WIDTH = BlockMarker3D.ReferenceWidth;
            %this.marker3d = WIDTH/2 * [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
            this.marker3d = WIDTH/2 * [0 1 1; 0 1 -1; 0 -1 1; 0 -1 -1];
            
                        
            this.h_fig = namedFigure('CozmoVisionProcessor', ...
                'KeypressFcn', @(src,edata)this.keyPressCallback(src,edata));
            this.h_axes = subplot(1,1,1, 'Parent', this.h_fig);
            this.h_img = imagesc(zeros(320,240), 'Parent', this.h_axes);
            this.h_title = title(this.h_axes, 'Initialized');
            
            this.h_templateAxes = axes('Position', [0 .8 .15 .15], ...
                'Parent', this.h_fig, 'Visible', 'off');
            this.h_template = imagesc(zeros(1), 'Parent', this.h_templateAxes);
            
            hold(this.h_axes, 'on');
            this.h_track = plot(nan, nan, 'r', 'LineWidth', 3, ...
                'Parent', this.h_axes);
         
            axis(this.h_axes, 'image')
            axis(this.h_templateAxes, 'image')
            
            % Angle Error Plot
            ERRORBAR_LINEWIDTH = 25;
            h = axes('Position', [0 0 .2 .2], 'Parent', this.h_fig);
            title(h, 'Angle Error')
            this.h_angleError = plot([0 0], [0 0], 'r', 'Parent', h, 'LineWidth', ERRORBAR_LINEWIDTH);
            set(h, 'YTick', [], 'XTick', -20:5:20, ...
                'XTickLabel', [], 'YTickLabel', [], ...
                'XLim', 20*[-1 1], 'YLim', .25*[-1 1], 'XGrid', 'on');
            
            % Distance Error Plot
            h = axes('Position', [.8 0 .2 .2], 'Parent', this.h_fig);
            title(h, 'Distance');
            this.h_distError = plot([0 0], [0 0], 'r', 'Parent', h, 'LineWidth', ERRORBAR_LINEWIDTH);
            set(h, 'XTick', [], 'YTick', 0:20:250, ...
                'XTickLabel', [], 'YTickLabel', [], ...
                'XLim', .25*[-1 1], 'YLim', [0 250], 'YGrid', 'on');
            
            % Horizontal Error Plot
            h = axes('Position', [.8 .75 .2 .2], 'Parent', this.h_fig);
            title(h, 'Horizontal Error');
            %this.h_leftRightError = barh(0, 'Parent', h);
            this.h_leftRightError = plot([0 0], [0 0], 'r', 'Parent', h, 'LineWidth', ERRORBAR_LINEWIDTH);
            set(h, 'XTick', -30:5:30, 'YTick', [], ...
                'XTickLabel', [], 'YTickLabel', [], ...
                'XLim', [-30 30], 'YLim', .25*[-1 1], 'XGrid', 'on');
            
            colormap(this.h_fig, 'gray');
            
        end % Constructor: CozmoVisionProcessor()
       
        function done = get.isDone(this)
            if isa(this.serialDevice, 'SimulatedSerial')
                done = this.escapePressed || wb_robot_step(this.TIME_STEP) == -1;
            else
                done = this.escapePressed;
            end
           
        end
            
        
        
        function Run(this)
            
            fread(this.serialDevice, [1 this.serialDevice.BytesAvailable]);
            
            this.serialBuffer = [];
            this.escapePressed = false;
            
            while ~this.isDone
                t = tic;
                
                this.Update();

                % pause(max(0, this.TIME_STEP/1000 - toc(t)));
            end

        end % FUNCTION Run()
        
        
        
        function SendPacket(this, messageName, packet)
            assert(isa(packet, 'uint8'), 'Expecting UINT8 packet.');
            
            if ~isfield(this.messageIDs, messageName)
                warning('Unknown message ID for "%s". Not sending.', messageName);
                return;
            end
            
            % Send the header, followed by message ID, followed by the
            % actual message data
            msgID = this.messageIDs.(messageName);
            fwrite(this.serialDevice, [uint8(this.HEADER) msgID row(packet)]);
            fprintf('Sent "%s" packet with msgID=%d and %d bytes (time=%f).\n', ...
		    messageName, msgID, length(packet), wb_robot_get_time());
        end % FUNCTION: SendPacket()
        
        
        
        function StatusMessage(this, verbosityLevel, varargin)
            if this.verbosity >= verbosityLevel
                fprintf(varargin{:});
            end
        end
        
                
        function packet = SerializeMessageStruct(this, msgStruct)
            assert(isstruct(msgStruct), 'Expecting a message STRUCT as input.');
            names = fieldnames(msgStruct);
            packet = cell(1, length(names));
            for i = 1:length(names)
                packet{i} = this.Cast(msgStruct.(names{i}), 'uint8');
            end
            packet = [packet{:}];
                       
        end % FUNCTION SerializeMessageStruct()
        
        function keyPressCallback(this, ~,edata)
            if isempty(edata.Modifier)
                switch(edata.Key)
                    case 'space'
                        this.Run();
                        
                    case 'escape'
                        this.escapePressed = true;
                end
            end
        end % keyPressCallback()
                   
        function [img, timestamp, valid] = PacketToImage(this, packet)
            valid = false;
            img = [];
            timestamp = uint32(0);
            
            if ~CozmoVisionProcessor.RESOLUTION_LUT.isKey(char(packet(1)))
                warning('Unrecognized resolution byte!');
                
            else
                resolution = CozmoVisionProcessor.RESOLUTION_LUT(char(packet(1)));
                
                imgPacketLength = length(packet)-5;
                
                if imgPacketLength > 0
                    % Read the timestamp
                    timestamp = this.Cast(packet(2:5), 'uint32');
                    
                    if this.flipImage
                        packet(6:end) = fliplr(packet(6:end));
                    end
                    
                    if imgPacketLength ~= prod(resolution)
                        warning(['Image packet length did not match its ' ...
                            'specified resolution!']);
                        img = zeros(resolution);
                        if imgPacketLength < prod(resolution)
                            img(1:imgPacketLength) = packet(6:end);
                        end
                        img = img';
                        
                    else
                        img = reshape(packet(6:end), resolution)';
                        valid = true;
                    end
                end
            end
        end % FUNCTION PacketToImage()
    
        
        function updateBlockPose(this, H)
            
            this.block.pose = this.headCam.computeExtrinsics(...
                this.LKtracker.corners, this.marker3d);
            this.block.pose.parent = this.headCam.pose;
            
            %{
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
            R = [u1 u2 u3];
            %Rvec = rodrigues(Rmat);
            T    = H(:,3);
            
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
            
            % Now switch to block with respect to robot, instead of camera
            this.block.pose = this.block.pose.getWithRespectTo(this.robotPose);
            
        end % updateBlockPose()

        function setHeadAngle(this, newHeadAngle)
            
            % Get rotation matrix for the current head pitch, rotating
            % around the Y axis (which points to robot's LEFT!)
            Rpitch = rodrigues(-newHeadAngle*[0 1 0]);
                        
            this.headCam.pose = Pose( ...
                Rpitch * this.HEAD_CAM_ROTATION,  ...
                Rpitch * this.HEAD_CAM_POSITION(:));
            this.headCam.pose.parent = this.neckPose;
            
        end
        
    end % public methods
    
end % CLASSDEF CozmoVisionProcessor
