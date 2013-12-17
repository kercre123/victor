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
        
    case this.HEAD_CALIBRATION
        
        this.SetCalibration(packet, 'headCalibrationMatrix', this.trackingResolution);
        return
        
    case this.MAT_CALIBRATION
        
        %this.SetCalibration(packet, 'matCalibrationMatrix', this.matLocalizationResolution);
        assert(length(packet)==4, ...
            'Expecting matCamPixPerMM packet to contain 4 bytes.');
        
        this.matCamPixPerMM = double(this.Cast(packet(1:4), 'single'));
        
        return
        
    case this.SET_DOCKING_BLOCK
        
        this.dockingBlock = this.Cast(packet, 'uint16');
        this.StatusMessage(1, 'Setting docking block to %d.\n', ...
            this.dockingBlock);
        return;
        
    case this.DETECT_COMMAND
        
        delete(findobj(this.h_axes, 'Tag', 'BlockMarker2D'));
        set(this.h_templateAxes, 'Visible', 'off');
        set(this.h_track, 'XData', nan, 'YData', nan);
        
        [img, timestamp, valid] = CozmoVisionProcessor.PacketToImage(packet);
        if ~isempty(img)
            % Detect block markers
            this.DetectBlocks(img, timestamp);
        end
        
    case this.TRACK_COMMAND
        assert(~isempty(this.headCalibrationMatrix), ...
            'Must receive calibration message before tracking.');
        
        [img, timestamp, valid] = CozmoVisionProcessor.PacketToImage(packet);
        if valid
            this.Track(img, timestamp);
        else
            set(this.h_title, 'String', 'Invalid Image');
        end % IF valid
        
    case this.MAT_LOCALIZATION_COMMAND
        
        assert(~isempty(this.matCamPixPerMM), ...
            ['Must receive mat camera calibration (matCamPixPerMM) ' ...
            'using mat localization.']);
        
        [img, timestamp, valid] = CozmoVisionProcessor.PacketToImage(packet);
        if valid
            this.MatLocalize(img, timestamp);
        else
           set(this.h_title, 'String', 'Invalid Image'); 
        end
        
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
%axis(this.h_axes, 'image');
drawnow

end % FUNCTION ProcessPacket()