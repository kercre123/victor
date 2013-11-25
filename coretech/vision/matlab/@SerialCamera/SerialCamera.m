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
        
        % Resolution bytes:
        % (These should match what is defined in hal.h)
        VGA_RESOLUTION = char(sscanf('BA', '%2x'));      % 640 x 480
        QVGA_RESOLUTION = char(sscanf('BC', '%2x'));     % 320 x 240
        QQVGA_RESOLUTION = char(sscanf('B8', '%2x'));    % 160 x 120
        QQQVGA_RESOLUTION = char(sscanf('BD', '%2x'));   %  80 x  60
        QQQQVGA_RESOLUTION = char(sscanf('B7', '%2x'));  %  40 x  30
       
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
            
            this.serialObj = serial(this.port, 'BaudRate', BaudRate);
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
            
            bytesToRead = this.framelength*2-length(this.buffer);
            if bytesToRead > 0
                
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
                
            end
                        
            headerIndex = strfind(this.buffer, this.header);
            footerIndex = strfind(this.buffer, this.footer);
            
            if isempty(headerIndex) || isempty(footerIndex) 
                    
                % Buffer is full of stuff we can't use. Just dump it
                this.buffer = '';
                
            else
                
                % Find a header followed by a matching footer
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
                     
                    if max(headerIndex) > max(footerIndex)
                        % Header in the buffer is a header, so we may get
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
                                    
                    headerIndex = headerIndex(i_header-1);
                    footerIndex = footerIndex(i_footer-1);

                    % Get rid of stuff before the header which we can't use
                    this.buffer(1:(headerIndex-1)) = '';
                    
                    assert(strcmp(this.buffer(1:length(this.header)), this.header), ...
                        'Expecting the buffer to begin with the header at this point.');
                    
                    firstDataIndex = length(this.header)+1;
                    readLength = footerIndex-headerIndex-firstDataIndex;
                    
                    switch(foundHFpair)
                        case SerialCamera.MESSAGE_SUFFIX
                            % Read and ASCII message data from index(1) to
                            % index(2)
                            this.message = this.buffer(firstDataIndex + (1:readLength));
                                                       
                        case this.formatSuffix
                            % Read frame from index(1) to index(2)
                            if readLength ~= this.framelength
                                %set(h_axes, 'XColor', 'r', 'YColor', 'r');
                                %set(h_img, 'CData', zeros(60,80));
                                this.numDropped = this.numDropped + 1;
                            else
                                assert(readLength == this.framelength);
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
        
        function changeResolution(this, newResolution, frameRate)
            % Send magic command to change the resolution
            switch(newResolution)
                case 'VGA' % [640 480]
                    this.framesize = [640 480]; 
                    this.formatSuffix = SerialCamera.VGA_RESOLUTION;
                    if nargin < 3
                        frameRate = 1;
                    end
                    
                case 'QVGA' % [320 240]
                    this.framesize = [320 240]; 
                    this.formatSuffix = SerialCamera.QVGA_RESOLUTION;
                    if nargin < 3
                        frameRate = 5;
                    end
                    
                case 'QQVGA' % [160 120]
                    this.framesize = [160 120]; 
                    this.formatSuffix = SerialCamera.QQVGA_RESOLUTION;
                    if nargin < 3
                        frameRate = 15;
                    end
                    
                case 'QQQVGA' % [80 60]
                    this.framesize = [80 60];
                    this.formatSuffix = SerialCamera.QQQVGA_RESOLUTION;
                    if nargin < 3
                        frameRate = 30;
                    end
                    
                case 'QQQQVGA' % [40 30]
                    this.framesize = [40 30]; 
                    this.formatSuffix = SerialCamera.QQQQVGA_RESOLUTION;
                    if nargin < 3
                        frameRate = 60;
                    end
                    
                otherwise
                    error('Unrecognized resolution specified.');
                    
            end % SWITCH(newResolution)
            
            fwrite(this.serialObj, [SerialCamera.CHANGE_RES_CMD this.formatSuffix], 'char');
            this.framelength = prod(this.framesize);
            %this.header = [SerialCamera.BEEFFOFF char(formatSuffix)];
            this.fps = frameRate;
            
            fclose(this.serialObj);
            this.serialObj.InputBufferSize = 4*this.framelength;
            fopen(this.serialObj);
            
            while ~strcmp(this.serialObj.Status, 'open')
                pause(.01);
            end
            
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