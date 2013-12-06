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
        
    end % Constant Properties
    
    properties
        serialDevice;
        serialBuffer;
        
        h_fig;
        h_img;
        h_axes;
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
            
        end % Constructor: CozmoVisionProcessor()
       
        function Update(this)
            
            % Read whatever is available in the serial port:
            this.serialBuffer = [this.serialBuffer row(uint8(fread(this.serialDevice)))];
            
            if ~isempty(this.serialBuffer)
            desktop
            keyboard
            end
            
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
                    img = [];
                    fprintf('MessagePacket: %s\n', packet);
                    
                case this.DETECT_COMMAND
                    img = PacketToImage(packet);
                    if ~isempty(img)
                        set(h_title, 'String', 'Detection');
                        
                        markers = simpleDetector(img);
                        
                        % Send a message about each marker we found
                        for i = 1:length(markers)
                            markers{i}.draw('where', this.h_axes);
                        end
                        
                        % Send a message indicating there are no more block
                        % marker messages coming
                        
                    end
                    
                case this.INIT_TRACK_COMMAND
                    img = PacketToImage(packet);
                    if ~isempty(img)
                        set(h_title, 'String', 'Tracking Initialization');
                        
                    end
                    
                case this.TRACK_COMMAND
                    img = PacketToImage(packet);
                    if ~isempty(img)
                        set(h_title, 'String', 'Tracking');
                        
                    end
                    
                otherwise
                    warning('Unknown command %d for packet. Skipping.');
                
            end % SWITCH(command)
              
            set(this.h_img, 'CData', img);
            set(this.h_axes, ...
                'XLim', [.5 size(img,2)+.5], ...
                'YLim', [.5 size(img,1)+.5]);
            drawnow
         
        end % FUNCTION ProcessPacket()
        
        function SendPacket(this, packetType, packet)
            
        end
    end % Methods
    
    methods(Static = true)
        
        function img = PacketToImage(packet) 
            img = [];
            
            if ~CozmoVisionProcess.RESOLUTION_LUT.isKey(char(packet(1)))
                warning('Unrecognized resolution byte!');
            else
                resolution = CozmoVisionProcess.RESOLUTION_LUT(char(packet(1)));
                
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