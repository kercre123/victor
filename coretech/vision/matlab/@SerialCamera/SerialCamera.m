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
        % Default header (final 'BD' is for 80x60), converted from hex to a
        % byte array.
        BEEFFOFF = char(sscanf('BEEFF0FF', '%2x'))'; 
    end
    
    properties(SetAccess = 'protected')
        port;
        header;
        framesize;
        framelength;
        serialObj;
        fps;
        buffer;
        timeout;
        doVerticalFlip;
        
        numFrames;
        numDropped;
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
            
            index = strfind(this.buffer, this.header);
            
            if isempty(index)
                % Buffer is full of stuff we can't use. Just dump it
                this.buffer = '';
            else
                % Get rid of stuff before the first index which we can't use
                this.buffer(1:(index(1)-1)) = '';
                
                % Wait until we have two headers read
                if length(index) >= 2
                    assert(strcmp(this.buffer(1:length(this.header)), this.header), ...
                        'Expecting the buffer to begin with the header at this point.');
                    
                    % Read frame from index(1) to index(2)
                    readLength = min(this.framelength, index(2)-index(1)-length(this.header));
                    
                    if readLength < this.framelength
                        %set(h_axes, 'XColor', 'r', 'YColor', 'r');
                        %set(h_img, 'CData', zeros(60,80));
                        this.numDropped = this.numDropped + 1;
                    else
                        assert(readLength == this.framelength);
                        img = uint8(this.buffer(length(this.header)+(1:readLength)));
                        img = reshape(fliplr(img), this.framesize)';
                        if this.doVerticalFlip
                            img = flipud(img);
                        end
                        %set(h_img, 'CData', reshape(img, [80 60])');
                        %set(h_axes, 'XColor', 'g', 'YColor', 'g');
                        this.numFrames = this.numFrames + 1;
                    end
                    this.buffer(1:(readLength+length(this.header))) = [];
                end % IF at least 2 indexes
            end % IF / ELSE isempty(index)
            
        end % FUNCTION getFrame()
        
        function reset(this)
            fclose(this.serialObj);
            pause(.1);
            fopen(this.serialObj);
            this.buffer = '';
        end % reset()
        
        function success = changeResolution(this, newResolution, frameRate)
            % Send magic command to change the resolution
            switch(newResolution)
                case 'VGA' % [640 480]
                    error('VGA unsupported.');
                case 'QVGA' % [320 240]
                    this.framesize = [320 240]; 
                    serialCmd = 'X';
                    formatSuffix = sscanf('B8', '%x');
                    if nargin < 3
                        frameRate = 10;
                    end
                case 'QQVGA' % [160 120]
                    error('QQVGA unsupported.');
                case 'QQQVGA' % [80 60]
                    this.framesize = [80 60];
                    serialCmd = 'Z';
                    formatSuffix = sscanf('BD', '%x');
                    if nargin < 3
                        frameRate = 30;
                    end
                otherwise
                    error('Unrecognized resolution specified.');
            end
            
            fwrite(this.serialObj, serialCmd, 'char');
            this.framelength = prod(this.framesize);
            this.header = [SerialCamera.BEEFFOFF char(formatSuffix)];
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
          
            success = true;
        end % FUNCTION changeResolution()
        
    end % public methods
    
end % classdef SerialCamera