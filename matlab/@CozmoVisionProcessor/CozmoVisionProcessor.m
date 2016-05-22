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
        DETECT_COMMAND           = char(sscanf('AB', '%2x'));
        SET_MARKER_TO_TRACK      = char(sscanf('BC', '%2x'));
        TRACK_COMMAND            = char(sscanf('CD', '%2x'));
        ROBOT_STATE_MESSAGE      = char(sscanf('DE', '%2x'));
        DISPLAY_IMAGE_COMMAND    = char(sscanf('F0', '%2x'));
                    
        LIFT_DISTANCE = 34;  % in mm, forward from robot origin
        
        % Robot Geometry
        % TODO: read this out of cozmoConfig.h or JSON file?
        NECK_JOINT_POSITION = [-13 0 33.5]; % relative to robot origin
        HEAD_CAM_ROTATION   = [0 0 1; -1 0 0; 0 -1 0]; % rodrigues(-pi/2*[1 0 0])*rodrigues(pi/2*[0 1 0])
        %HEAD_CAM_ROTATION   = [0 1 0; 0 0 -1; -1 0 0];
        HEAD_CAM_POSITION   = [11.45 0 -6]; % relative to neck joint
        WHEEL_RADIUS        = 14.2;
        
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
        
        markerToTrack;
        
        % Camera calibration information:
        headCam;
        
        blockPose;
        robotPose;
        neckPose;
        marker3d;
        H_init;
        
        headAngleNoiseStdDev;
        
        % On simulator, no need to flip.  On robot, need to flip
        flipImage;
        
        % For tracking
        LKtracker;
        trackerType;
        trackingResolution;
        
        markerLibrary;
        detectionResolution;
        
        block;
        
        
        
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
        
        h_fig_localize;
        h_path;
        h_positionError;
        h_orientationError;
        
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
            try
            castedData = typecast(data, outputType);
            catch
                desktop
                keyboard
            end
            
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
            DetectionResolution = [320 240];
            Verbosity = 1;
            DoEndianSwap = false; % false for simulator, true with Movidius
            TIME_STEP = 30; %#ok<PROP>
            MarkerLibraryFile = [];
            HeadAngleNoiseStdDev = 1; % in degrees
            
            parseVarargin(varargin{:});
            
            if ~isempty(MarkerLibraryFile)
                temp = load(MarkerLibraryFile);
                if ~isfield(temp, 'markerLibrary') || ~isa(temp.markerLibrary, 'VisionMarkerLibrary')
                    error('No VisionMarkerLibrary found in the specified file.');
                end 
                this.markerLibrary = temp.markerLibrary;
                clear temp
            end
            
            assert(isa(SerialDevice, 'serial') || ...
                isa(SerialDevice, 'SimulatedSerial'), ...
                'SerialDevice must be a serial or SimulatedSerial object.');
            this.serialDevice = SerialDevice;
            this.serialDevice.InputBufferSize = 640*480;
            
            fclose(this.serialDevice);
            fopen(this.serialDevice);
            
            this.headAngleNoiseStdDev = HeadAngleNoiseStdDev;
            
            this.doEndianSwap = DoEndianSwap;
                        
            this.flipImage = FlipImage;
            
            this.verbosity = Verbosity;
            this.TIME_STEP = TIME_STEP; %#ok<PROP>
            
            this.LKtracker = [];
            this.trackerType = TrackerType;
            this.trackingResolution = TrackingResolution;
            this.detectionResolution = DetectionResolution;
            
            this.desiredBufferSize = this.computeDesiredBufferLength(this.detectionResolution);
           
            this.markerToTrack = [];
                        
            % Set up Robot head camera geometry
            this.robotPose = Pose(-pi/2*[0 0 1], [0;0;CozmoVisionProcessor.WHEEL_RADIUS]);
            this.neckPose = Pose([0 0 0], this.NECK_JOINT_POSITION);
            this.neckPose.parent = this.robotPose;
            
            %WIDTH = BlockMarker3D.ReferenceWidth;
            %this.marker3d = WIDTH/2 * [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
            
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
            
            
            this.h_fig_localize = namedFigure('CozmoLocalization');
            subplot(3,1,1, 'Parent', this.h_fig_localize)
            hold off, this.h_path(1) = plot(nan, nan, 'r+-');
            hold on, this.h_path(2) = plot(nan, nan, 'bx-');
            grid on
            axis equal
            title('Robot Path'), xlabel('X (mm)'), ylabel('Y (mm)')
            
            subplot(3,1,2, 'Parent', this.h_fig_localize);
            cla
            this.h_positionError = plot(nan, nan, 'ro-');
            title('Position Error'), xlabel('Time (s)'), ylabel('Error (mm)')
            
            subplot(3,1,3, 'Parent', this.h_fig_localize);
            cla
            this.h_orientationError = plot(nan, nan, 'ro-');
            title('Orientation Error'), xlabel('Time (s)'), ylabel('Error (degrees)')
            
        end % Constructor: CozmoVisionProcessor()
       
        function done = get.isDone(this)
            if isa(this.serialDevice, 'SimulatedSerial')
                done = this.escapePressed || wb_robot_step(this.TIME_STEP) == -1;
                
                %                 % Display a profile report after some simulation time has
                %                 % passed
                %                 if wb_robot_get_time() > 2
                %                     S = profile('STATUS');
                %                     if strcmp(S.ProfilerStatus, 'on')
                %                         profile report
                %                         profile off
                %                     end
                %                 end
            else
                done = this.escapePressed;
            end
           
        end
            
        
        
        function Run(this)
            
            fread(this.serialDevice, [1 this.serialDevice.BytesAvailable]);
            
            this.serialBuffer = [];
            this.escapePressed = false;

            while ~this.isDone
                %t = tic;
                
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
