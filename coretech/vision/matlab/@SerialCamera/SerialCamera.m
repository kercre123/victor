% SerialCamera Object
%
% Serial UART interface to robot cameras, e.g. through FTDI chip.
% NOTE: Because of the high baud rate generally required, this may only 
%       work on Windows.
%
% ------------
% Andrew Stein
%
classdef SerialCamera < handle
    
    properties(Constant = true)
        % Default header and footer:
        BEEFFOFF = char(sscanf('BEEFF0FF', '%2x'))'; 
        FFOFFEEB = char(sscanf('FF0FFEEB', '%2x'))';
        
        % Store the frame size, header format byte, and default frame rate
        % for each resolution.
        RESOLUTION_INFO = struct( ...
            'VGA',     struct('frameSize', [640 480], 'frameRate',  1, 'fmtSuffix', char(sscanf('BA', '%2x'))), ...
            'QVGA',    struct('frameSize', [320 240], 'frameRate',  5, 'fmtSuffix', char(sscanf('BC', '%2x'))), ...
            'QQVGA',   struct('frameSize', [160 120], 'frameRate', 15, 'fmtSuffix', char(sscanf('B8', '%2x'))), ...
            'QQQVGA',  struct('frameSize', [ 80  60], 'frameRate', 30, 'fmtSuffix', char(sscanf('BD', '%2x'))), ...
            'QQQQVGA', struct('frameSize', [ 40  30], 'frameRate', 60, 'fmtSuffix', char(sscanf('B7', '%2x'))));
       
        CHANGE_RES_CMD = char(sscanf('CA', '%2x')); 
        
        % Byte following header that indicates a message
        MESSAGE_SUFFIX = char(sscanf('DD', '%2x'));
    end
    
    properties(SetAccess = 'protected')
        port;
        header;
        footer;
        formatSuffix; % image format to be looking for
        framesize;
        framelength;
        serialObj;
        fps;
        buffer;
        timeout;
        doVerticalFlip;
        
        numFrames;
        numDropped;
        
        message;
    end
    
    
    methods
        function this = SerialCamera(portIn, varargin)
            
            %Header    = char(SerialCamera.BEEFFOFF);
            Resolution = 'QQQVGA';
            BaudRate  = 1000000;
            FPS       = 30;
            TimeOut   = 5; % seconds
            DoVerticalFlip = false;
            
            parseVarargin(varargin{:});
            
            this.port = portIn;
            this.fps        = FPS;
            this.numFrames  = 0;
            this.numDropped = 0;
            this.buffer     = '';
            this.timeout    = TimeOut;
            this.doVerticalFlip = DoVerticalFlip;
            
            % TODO: More elegant way to do this?
            instruments = instrfind;
            if ~isempty(instruments)
                fclose(instrfind);
                delete(instrfind);
            end
            
            this.header = SerialCamera.BEEFFOFF;
            this.footer = SerialCamera.FFOFFEEB;
            this.message = '';
            
            if isa(this.port, 'SimulatedSerialCamera') || ...
                    isa(this.port, 'serial')
                this.serialObj = this.port;
            else
                this.serialObj = serial(this.port, 'BaudRate', BaudRate);
            end
            
            fopen(this.serialObj);
            this.changeResolution(Resolution);
            
        end % Constructor: SerialCamera()
        
        function delete(this)
            fclose(this.serialObj);
            delete(this.serialObj);
        end
        
        function img = getFrame(this)
            
            img = [];
            this.message = '';
            
            % Read enough bytes to fill the buffer with two frames worth of
            % data
            bytesToRead = this.framelength*2-length(this.buffer);
            if bytesToRead > 0
                
                % If the serial object doesn't have enough bytes available,
                % what until it does.
                t = tic;
                while this.serialObj.BytesAvailable < bytesToRead
                    pause(1/this.fps);
                    if toc(t) > this.timeout
                        warning('Serial object timed out waiting to acquire %d bytes!', bytesToRead);
                        return;
                    end
                end
                
                %valuesReadBefore = this.serialObj.ValuesReceived;
                this.buffer = [this.buffer fread(this.serialObj, ...
                    [1 bytesToRead], 'char')];
                %valuesReadAfter = this.serialObj.ValuesReceived;
                
                %if valuesReadAfter - valuesReadBefore ~= bytesToRead
                %    disp('Unexpected number of bytes read.');
                %    keyboard
                %end
                
            end % IF bytesToRead > 0
                
            % Find all the header and footer bytes
            headerIndex = strfind(this.buffer, this.header);
            footerIndex = strfind(this.buffer, this.footer);
            
            if isempty(headerIndex) || isempty(footerIndex) 
                    
                % Buffer is full of stuff we can't use. Just dump it
                this.buffer = '';
                
            else
                
                % Find a header followed by a matching footer
                % (There's probably a prettier way to do this)
                foundHFpair = '';
                i_header = 1;
                while isempty(foundHFpair) && i_header <= length(headerIndex)
                    headerType = this.buffer(headerIndex(i_header)+length(this.header));
                    
                    i_footer = 1;
                    while isempty(foundHFpair) && i_footer <= length(footerIndex)
                        % Footer must be after this header, but before next
                        % header (if this isn't the last header)
                        if footerIndex(i_footer) > headerIndex(i_header) && ...
                                (i_header == length(headerIndex) || ...
                                footerIndex(i_footer) < headerIndex(i_header+1))
                            
                            % Footer must have same type byte as header
                            footerType = this.buffer(footerIndex(i_footer)+length(this.footer));
                            if headerType == footerType
                                foundHFpair = headerType;
                            end
                        end
                        i_footer = i_footer + 1;
                    end
                    i_header = i_header + 1;
                end
                
                if isempty(foundHFpair)
                    % No header/footer pair found
                     
                    if max(headerIndex) > max(footerIndex)
                        % Last index in the buffer is a header, so we may get
                        % the rest of the message later.  So just dump
                        % everything up to that last header (which
                        % apparently was corrupted, since we found no
                        % matching pairs)
                        this.buffer(1:(headerIndex(end)-1)) = [];
                    else
                        % Nothing useful in the buffer, just dump it all
                        % and start over.
                        this.buffer = '';
                    end
                    
                else
                    % Get the selected header/footer pair
                    headerIndex = headerIndex(i_header-1);
                    footerIndex = footerIndex(i_footer-1);

                    assert(headerIndex < footerIndex, ...
                        'Header index should be before footer index.');
                    
                    % We can't really use anything before the header
                    this.buffer(1:(headerIndex-1)) = '';
                    
                    assert(strcmp(this.buffer(1:length(this.header)), this.header), ...
                        'Expecting the buffer to begin with the header at this point.');
                    
                    firstDataIndex = length(this.header)+1;
                    readLength = footerIndex-headerIndex-firstDataIndex;
                    
                    switch(foundHFpair)
                        case SerialCamera.MESSAGE_SUFFIX
                            % Read an ASCII message data from the data
                            % between header and footer
                            this.message = this.buffer(firstDataIndex + (1:readLength));
                                                       
                        case this.formatSuffix
                            % Read frame data from buffer between header
                            % and footer
                            if readLength ~= this.framelength
                                %set(h_axes, 'XColor', 'r', 'YColor', 'r');
                                %set(h_img, 'CData', zeros(60,80));
                                this.numDropped = this.numDropped + 1;
                            else
                                %assert(readLength == this.framelength);
                                img = uint8(this.buffer(firstDataIndex+(1:readLength)));
                                img = reshape(fliplr(img), this.framesize)';
                                if this.doVerticalFlip
                                    img = flipud(img);
                                end
                                %set(h_img, 'CData', reshape(img, [80 60])');
                                %set(h_axes, 'XColor', 'g', 'YColor', 'g');
                                this.numFrames = this.numFrames + 1;
                            end
                            
                        otherwise
                            warning('Unrecognized header suffix found.');
                    end
                       
                    % Remove everything we just read (header, format byte,
                    % message/image data, footer, and format byte) from the
                    % buffer
                    this.buffer(1:(readLength+firstDataIndex+length(this.footer)+1)) = [];
                
                end % IF / ELSE found a matching header/footer pair
                
            end % IF / ELSE isempty(index)
            
        end % FUNCTION getFrame()
        
        function reset(this)
            fclose(this.serialObj);
            pause(.1);
            fopen(this.serialObj);
            this.buffer = '';
        end % reset()
        
        function changeResolution(this, newResolution, newFrameRate)
            
            if ~isfield(SerialCamera.RESOLUTION_INFO, newResolution)
                error('Unrecognized resolution.');
            end
              
            % Get the settings for the specified resolution
            resInfo = SerialCamera.RESOLUTION_INFO.(newResolution);
            
            this.formatSuffix = resInfo.fmtSuffix;
            this.framesize = resInfo.frameSize;
            if nargin < 3
                this.fps = resInfo.frameRate;
            else
                this.fps = newFrameRate;
            end
            
            % Send the change-resolution command over the serial line
            fwrite(this.serialObj, [SerialCamera.CHANGE_RES_CMD ...
                resInfo.fmtSuffix], 'char');
            
            this.framelength = prod(this.framesize);
                        
            fclose(this.serialObj);
            this.serialObj.InputBufferSize = 4*this.framelength;
            fopen(this.serialObj);
            
            %while ~strcmp(this.serialObj.Status, 'open')
            %    pause(.01);
            %end
            
            % clear the serial input buffer?
            if this.serialObj.BytesAvailable > 0
                fread(this.serialObj, this.serialObj.BytesAvailable, 'char');
            end
            
            this.buffer = '';

        end % FUNCTION changeResolution()
        
        function sendMessage(this, msg)
            assert(isa(msg, 'uint8'), 'Message must be a uint8 array.');
        
            fwrite(this.serialObj, msg, 'uint8'); 
        end
    end % public methods
    
end % classdef SerialCamera