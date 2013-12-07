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
        DETECT_COMMAND           = char(sscanf('AB', '%2x'));
        INIT_TRACK_COMMAND       = char(sscanf('BC', '%2x'));
        TRACK_COMMAND            = char(sscanf('CD', '%2x'));
        MAT_ODOMETRY_COMMAND     = char(sscanf('DE', '%2x'));
        MAT_LOCALIZATION_COMMAND = char(sscanf('EF', '%2x'));
                    
    end % Constant Properties
    
    properties
        serialDevice;
        serialBuffer;
        
        messageIDs;
        
        h_fig;
        h_img;
        h_axes;
        h_title;
    end
    
    methods
        
        function this = CozmoVisionProcessor(varargin)
            SerialDevice = [];
            
            parseVarargin(varargin{:});
            
            assert(isa(SerialDevice, 'serial') || ...
                isa(SerialDevice, 'SimulatedSerial'), ...
                'SerialDevice must be a serial or SimulatedSerial object.');
            this.serialDevice = SerialDevice;
            fopen(this.serialDevice);
            
            this.h_fig = namedFigure('CozmoVisionProcessor');
            this.h_axes = subplot(1,1,1, 'Parent', this.h_fig);
            this.h_img = imagesc(zeros(320,240), 'Parent', this.h_axes);
            this.h_title = title(this.h_axes, 'Initialized');
            
            hold(this.h_axes, 'on');
            colormap(this.h_fig, 'gray');
            
        end % Constructor: CozmoVisionProcessor()
       
        function Update(this)
            
            % Read whatever is available in the serial port:
            newData = row(uint8(fread(this.serialDevice)));
            
            if isempty(newData)
                return;
            end
            
            this.serialBuffer = [this.serialBuffer newData];
           
            fprintf('Received %d bytes. Buffer now %d bytes long.\n', ...
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
        
        function ProcessPacket(this, command, packet)
            
            switch(command)
                case this.MESSAGE_COMMAND
                    fprintf('MessagePacket: %s\n', packet);
                    return;
                    
                case this.MESSAGE_ID_DEFINITION
                    msgID = packet(1);
                    name  = char(packet(2:end));
                    this.messageIDs.(name) = msgID;
                    fprintf('Registered "%s" as msgID=%d.\n', ...
                        name, msgID);
                    return;
                    
                case this.DETECT_COMMAND
                    delete(findobj(this.h_axes, 'Tag', 'BlockMarker2D'));
                    
                    img = CozmoVisionProcessor.PacketToImage(packet);
                    if ~isempty(img)
                        
                        markers = simpleDetector(img);
                        
                        % Send a message about each marker we found
                        for i = 1:length(markers)
                            markers{i}.draw('where', this.h_axes);
                            
                            msgStruct = struct( ...
                                'msgID', uint8(this.messageIDs.CozmoMsg_ObservedBlockMarker), ...
                                'blockType', uint16(markers{i}.blockType), ...
                                'faceType', uint8(markers{i}.faceType), ...
                                'upDirection', uint8(markers{i}.upDirection), ...
                                'headAngle', single(0), ... ???
                                'x_imgUpperLeft', single(markers{i}.corners(1,1)), ...
                                'y_imgUpperLeft', single(markers{i}.corners(1,2)), ...
                                'x_imgLowerLeft', single(markers{i}.corners(2,1)), ...
                                'y_imgLowerLeft', single(markers{i}.corners(2,2)), ...
                                'x_imgUpperRight', single(markers{i}.corners(3,1)), ...
                                'y_imgUpperRight', single(markers{i}.corners(3,2)), ...
                                'x_imgLowerRight', single(markers{i}.corners(4,1)), ...
                                'y_imgLowerRight', single(markers{i}.corners(4,2)) );
                            
                            packet = CozmoVisionProcessor.SerializeMessageStruct(msgStruct);
                            this.SendPacket(packet);
                        end
                        
                        set(this.h_title, 'String', ...
                            sprintf('Detected %d Markers', length(markers)));
                        
                        % Send a message indicating there are no more block
                        % marker messages coming
                        msgStruct = struct( ...
                            'msgID', uint8(this.messageIDs.CozmoMsg_TotalBlocksDetected), ...
                            'numBlocks', uint8(length(markers)) );
                        
                        packet = CozmoVisionProcessor.SerializeMessageStruct(msgStruct);
                        this.SendPacket(packet);
                        
                    end
                    
                case this.INIT_TRACK_COMMAND
                    img = CozmoVisionProcessor.PacketToImage(packet);
                    if ~isempty(img)
                        set(this.h_title, 'String', 'Tracking Initialization');
                        
                    end
                    
                case this.TRACK_COMMAND
                    img = CozmoVisionProcessor.PacketToImage(packet);
                    if ~isempty(img)
                        set(this.h_title, 'String', 'Tracking');
                        
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
            drawnow
         
        end % FUNCTION ProcessPacket()
        
        function SendPacket(this, packet)
            assert(isa(packet, 'uint8'), 'Expecting UINT8 packet.');
            
            assert(packet(1) == length(packet), ...
                'Expecting first byte of packet to be its size.');
            
            fwrite(this.serialDevice, [uint8(this.HEADER) row(packet)]);
        end
        
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
        
            function packet = SerializeMessageStruct(msgStruct)
                names = fieldnames(msgStruct);
                packet = cell(1, length(names));
                for i = 1:length(names)
                    packet{i} = typecast(swapbytes(msgStruct.(names{i})), 'uint8');
                end
                packet = [packet{:}];
                
                % Add a size byte to the beginnging:
                packet = [uint8(length(packet)+1) packet];
            end
        
    end % Static Methods
    
end % CLASSDEF CozmoVisionProcessor