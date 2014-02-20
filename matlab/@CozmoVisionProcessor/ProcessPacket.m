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
        
                
    case this.ROBOT_STATE_MESSAGE
        % NOTE: packet(1:4) is the timestamp
        newHeadAngle = this.Cast(packet(5:8), 'single');
        this.setHeadAngle(newHeadAngle);
        
        this.StatusMessage(1, 'Updating head angle to: %.1f degrees\n', ...
            newHeadAngle*180/pi);
        return;
        
    case this.HEAD_CALIBRATION
        
        %this.SetCalibration(packet, 'headCalibrationMatrix', this.trackingResolution);
        
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
        
        this.headCam = Camera('resolution', double(fliplr(dims)), ...
            'calibration', struct('fc', double(f), 'cc', double(c), ...
            'kc', zeros(5,1), 'alpha_c', 0));  
        
        return
        
    case this.SET_MARKER_TO_TRACK
        
        this.markerToTrack = this.Cast(packet, 'uint8');
        this.StatusMessage(1, 'Setting marker to track to block to: %s\n', ...
            num2str(this.markerToTrack));
        
        % TODO: instantiate block from markerToTrack? (really this should
        % be on basestation)
        this.block = Block(60);
        
        return;
        
    case this.DISPLAY_IMAGE_COMMAND
        
        img = this.PacketToImage(packet);
        
    case this.DETECT_COMMAND
        
        delete(findobj(this.h_axes, 'Tag', 'BlockMarker2D'));
        set(this.h_templateAxes, 'Visible', 'off');
        set(this.h_track, 'XData', nan, 'YData', nan);
        
        [img, timestamp, valid] = this.PacketToImage(packet);
        if ~isempty(img)
            % Detect block markers
            this.DetectBlocks(img, timestamp);
        end
        
    case this.TRACK_COMMAND
        assert(~isempty(this.headCam), ...
            'Must receive calibration message before tracking.');
        
        [img, timestamp, valid] = this.PacketToImage(packet);
        if valid
            this.Track(img, timestamp);
        else
            set(this.h_title, 'String', 'Invalid Image');
        end % IF valid
        
    otherwise
        warning('Unknown command %d for packet. Skipping.', command);
        return;
        
end % SWITCH(command)

if isempty(img)
    img = 0;
end
set(this.h_img, 'CData', img);
set(this.h_axes, ...
    'XLim', [.5 size(img,2)+.5], ...
    'YLim', [.5 size(img,1)+.5]);
%axis(this.h_axes, 'image');
drawnow

end % FUNCTION ProcessPacket()