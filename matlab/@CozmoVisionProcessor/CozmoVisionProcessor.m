classdef CozmoVisionProcessor < handle
    
    properties(Constant = true)
        TIME_STEP = 64;
        
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
        CALIBRATION              = char(sscanf('CC', '%2x'));
        DETECT_COMMAND           = char(sscanf('AB', '%2x'));
        SET_DOCKING_BLOCK        = char(sscanf('BC', '%2x'));
        TRACK_COMMAND            = char(sscanf('CD', '%2x'));
        MAT_ODOMETRY_COMMAND     = char(sscanf('DE', '%2x'));
        MAT_LOCALIZATION_COMMAND = char(sscanf('EF', '%2x'));
                    
        LIFT_DISTANCE = 34;  % in mm, forward from robot origin
        
    end % Constant Properties
    
    properties
        serialDevice;
        serialBuffer;
        doEndianSwap;
        
        verbosity;
        
        % LUT for enumerated message ID values 
        messageIDs;
        
        dockingBlock;
        
        calibrationMatrix;
        
        % For tracking
        LKtracker;
        trackerType;
        trackingResolution;
        
        % Display handles
        h_fig;
        h_img;
        h_axes;
        h_title;
        h_templateAxes;
        h_template;
        h_track;
    end
    
    properties(SetAccess = 'protected', Dependent = true)
        isDone;
    end
        
    methods
        
        function castedData = Cast(this, data, outputType)
         
            if this.doEndianSwap
               data = swapbytes(data); 
            end
                
            castedData = typecast(data, outputType);
        end
        
        function this = CozmoVisionProcessor(varargin)
            SerialDevice = [];
            TrackerType = 'affine';
            TrackingResolution = [80 60];
            Verbosity = 1;
            DoEndianSwap = false; % false for simulator, true with Movidius
            
            parseVarargin(varargin{:});
            
            assert(isa(SerialDevice, 'serial') || ...
                isa(SerialDevice, 'SimulatedSerial'), ...
                'SerialDevice must be a serial or SimulatedSerial object.');
            this.serialDevice = SerialDevice;
            fopen(this.serialDevice);
            
            this.doEndianSwap = DoEndianSwap;
            
            this.verbosity = Verbosity;
            
            this.LKtracker = [];
            this.trackerType = TrackerType;
            this.trackingResolution = TrackingResolution;
            
            this.dockingBlock = 0;
            
            this.h_fig = namedFigure('CozmoVisionProcessor');
            this.h_axes = subplot(1,1,1, 'Parent', this.h_fig);
            this.h_img = imagesc(zeros(320,240), 'Parent', this.h_axes);
            this.h_title = title(this.h_axes, 'Initialized');
            
            this.h_templateAxes = axes('Position', [0 .8 .15 .15], ...
                'Parent', this.h_fig, 'Visible', 'off');
            this.h_template = imagesc(zeros(1), 'Parent', this.h_templateAxes);
            
            hold(this.h_axes, 'on');
            this.h_track = plot(nan, nan, 'r', 'LineWidth', 3, ...
                'Parent', this.h_axes);
            hold(this.h_axes, 'off');
            
            hold(this.h_axes, 'on');
            colormap(this.h_fig, 'gray');
            
        end % Constructor: CozmoVisionProcessor()
       
        function done = get.isDone(this)
            if isa(this.serialDevice, 'SimulatedSerial')
                done = wb_robot_step(this.TIME_STEP) == -1;
            else
                done = false;
            end
        end
            
        function Update(this)
                        
            % Read whatever is available in the serial port:
            newData = row(uint8(fread(this.serialDevice)));
            
            if isempty(newData)
                return;
            end
            
            this.serialBuffer = [this.serialBuffer newData];
           
            this.StatusMessage(3, 'Received %d bytes. Buffer now %d bytes long.\n', ...
                length(newData), length(this.serialBuffer));
            
            % Find the headers and footers
            headerIndex = strfind(this.serialBuffer, this.HEADER);
            footerIndex = strfind(this.serialBuffer, this.FOOTER);
            
            % Process all the packets in the serial buffer
            i_footer = 1;
            i_header = 1;
            while i_header <= length(headerIndex) && i_footer <= length(footerIndex)
                headerSuffix = this.serialBuffer(headerIndex(i_header) + length(this.HEADER));
                
                % Find the next footer that is after this header
                while footerIndex(i_footer) < headerIndex(i_header) && ...
                        i_footer <= length(footerIndex)
                    
                    i_footer = i_footer + 1;
                end
                
                if i_footer <= length(footerIndex)

                    footerSuffix = this.serialBuffer(footerIndex(i_footer) + length(this.FOOTER));
                    if headerSuffix == footerSuffix
                        % If header and footer must have same suffix, this must
                        % be a valid packet.  Process it.
                        packetStart = headerIndex(i_header) + length(this.HEADER) + 1;
                        packetEnd   = footerIndex(i_footer) - 1;
                        packet = this.serialBuffer(packetStart:packetEnd);
                        
                        this.ProcessPacket(headerSuffix, packet);
                        
                    else
                        % If they don't match, we've got garbage between this
                        % header/footer pair, so just move on to the next
                        % header...
                        
                    end
                end
                
                i_header = i_header + 1;
            end % FOR each header
            
            % Kill what's in the buffer, whether we processed it above or
            % ignored it as garbage.  
            if ~isempty(headerIndex) && ~isempty(footerIndex) && ...
                    headerIndex(end) > footerIndex(end)
                
                % If we've got a final header that had no footer after it, 
                % we potentially have a partial message.  Keep that in the 
                % buffer, to hopefully have its remainder appended on the 
                % next Update.
                this.serialBuffer(1:(headerIndex(end)-1)) = [];
            else
                this.serialBuffer = [];
            end
            
        end % FUNCTION: Update()
        
        function Run(this)
           while ~this.isDone
               this.Update();
           end
           
        end % FUNCTION Run()
        
        function ProcessPacket(this, command, packet)
            
            switch(command)
                case this.MESSAGE_COMMAND
                    % Just display a text message
                    fprintf('MessagePacket: %s\n', packet);
                    return;
                    
                case this.MESSAGE_ID_DEFINITION
                    % Add an entry to the LUT
                    msgID = packet(1);
                    name  = char(packet(2:end));
                    this.messageIDs.(name) = uint8(msgID);
                    fprintf('Registered "%s" as msgID=%d.\n', ...
                        name, msgID);
                    return;
                    
                case this.CALIBRATION
                    % Expecting:
                    % struct {
                    %   f32 focalLength_x, focalLength_y, fov_ver;
                    %   f32 center_x, center_y;
                    %   f32 skew;
                    %   u16 nrows, ncols;
                    % }
                    %
                    
                    f    = this.Cast(packet(1:8),   'single');
                    c    = this.Cast(packet(13:20), 'single');
                    dims = this.Cast(packet(25:28), 'uint16');
                    
                    assert(length(f) == 2, ...
                        'Expecting two single precision floats for focal lengths.');
                    
                    assert(length(c) == 2, ...
                        'Expecting two single precision floats for camera center.');
                    
                    assert(length(dims) == 2, ...
                        'Expecting two 16-bit integers for calibration dimensions.');
                    
                    this.calibrationMatrix = double([f(1) 0 c(1); 0 f(2) c(2); 0 0 1]);
                                         
                    % Compare the calibration resolution to the tracking
                    % resolution and since it will only be used on data at
                    % tracking resolution, adjust it accordingly
                    adjFactor = this.trackingResolution ./ double([dims(2) dims(1)]);
                    assert(adjFactor(1) == adjFactor(2), ...
                        'Expecting calibration scaling factor to match for X and Y.');
                    this.calibrationMatrix = this.calibrationMatrix * adjFactor(1);
                    
                    this.StatusMessage(1, 'Camera calibration set (scaled by %.1f): \n', adjFactor(1));
                    disp(this.calibrationMatrix);
                    
                    return
                    
                case this.SET_DOCKING_BLOCK
                    this.dockingBlock = this.Cast(packet, 'uint16');
                    this.StatusMessage(1, 'Setting docking block to %d.\n', ...
                        this.dockingBlock);
                    return;
                    
                case this.DETECT_COMMAND
                    % Detect block markers
                    
                    delete(findobj(this.h_axes, 'Tag', 'BlockMarker2D'));
                    set(this.h_templateAxes, 'Visible', 'off');
                    set(this.h_track, 'XData', nan, 'YData', nan);
                    
                    img = CozmoVisionProcessor.PacketToImage(packet);
                    if ~isempty(img)
                        
                        markers = simpleDetector(img);
                        
                        % Send a message about each marker we found
                        for i = 1:length(markers)
                            markers{i}.draw('where', this.h_axes);
                            
                            msgStruct = struct( ...
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
                    
                            end
                        end
                        
                        set(this.h_title, 'String', ...
                            sprintf('Detected %d Markers', length(markers)));
                        
                        % Send a message indicating there are no more block
                        % marker messages coming
                        msgStruct = struct('numBlocks', uint8(length(markers)) );
                        
                        packet = this.SerializeMessageStruct(msgStruct);
                        this.SendPacket('CozmoMsg_TotalBlocksDetected', packet);
                        
                    end
                    
                case this.TRACK_COMMAND
                    assert(~isempty(this.calibrationMatrix), ...
                        'Must receive calibration message before tracking.');
                    
                    img = CozmoVisionProcessor.PacketToImage(packet);
                    if ~isempty(img)
                                                      
                        % Update the tracker with this frame
                        converged = this.LKtracker.track(img, ...
                            'MaxIterations', 50, ...
                            'ConvergenceTolerance', .25,...
                            'ErrorTolerance', 0.5);
                        
                        if converged
                            set(this.h_title, 'String', 'Tracking Converged');
                            
                            corners = this.LKtracker.corners;
                            
                            set(this.h_track, ...
                                'XData', corners([1 2 4 3 1],1), ...
                                'YData', corners([1 2 4 3 1],2));
                                                        
                            [distError, midPointErr, angleError] = computeError(this);
                            msgStruct = struct( ...
                                'x_distErr', single(distError), ...
                                'y_horErr',  single(midPointErr), ...
                                'angleErr',  single(angleError), ...
                                'didTrackingSucceed', uint8(true));
                            
                        else
                            set(this.h_title, 'String', 'Tracking Failed');
                            msgStruct = struct( ...
                                'x_distErr', single(0), ...
                                'y_horErr',  single(0), ...
                                'angleErr',  single(0), ...
                                'didTrackingSucceed', uint8(false));
                        end
                              
                        packet = this.SerializeMessageStruct(msgStruct);
                        this.SendPacket('CozmoMsg_DockingErrorSignal', packet);
                        
                    end % IF img not empty
                    
                otherwise
                    warning('Unknown command %d for packet. Skipping.');
                
            end % SWITCH(command)
              
            if isempty(img)
                img = 0;
            end
            set(this.h_img, 'CData', img);
            set(this.h_axes, ...
                'XLim', [.5 size(img,2)+.5], ...
                'YLim', [.5 size(img,1)+.5]);
            drawnow
         
        end % FUNCTION ProcessPacket()
        
        function SendPacket(this, messageName, packet)
            assert(isa(packet, 'uint8'), 'Expecting UINT8 packet.');
            assert(isfield(this.messageIDs, messageName), ...
                'Unknown message ID for "%s".', messageName);           
            
            % Send the header, followed by message ID, followed by the
            % actual message data
            msgID = this.messageIDs.(messageName);
            fwrite(this.serialDevice, [uint8(this.HEADER) msgID row(packet)]);
        end % FUNCTION: SendPacket()
        
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
        
           
    end % Methods
    
    methods(Static = true)
        
        function img = PacketToImage(packet)
            img = [];
            
            if ~CozmoVisionProcessor.RESOLUTION_LUT.isKey(char(packet(1)))
                warning('Unrecognized resolution byte!');
            else
                resolution = CozmoVisionProcessor.RESOLUTION_LUT(char(packet(1)));
                
                if length(packet)-1 ~= prod(resolution)
                    warning(['Image packet length did not match its ' ...
                        'specified resolution!']);
                else
                    img = reshape(packet(2:end), resolution)';
                end
            end
        end % FUNCTION PacketToImage()
        
    end % Static Methods
    
end % CLASSDEF CozmoVisionProcessor