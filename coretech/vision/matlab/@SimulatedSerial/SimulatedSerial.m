classdef SimulatedSerial < handle
   
    % Unused properties to make this look like a Serial object.
    properties
        BaudRate;
        InputBufferSize;
    end
    
    properties(SetAccess = 'protected')
        Status;
    end
    
    properties(Dependent = true)
        Port;
    end
    
    properties(SetAccess = 'protected', Dependent = true)
        BytesAvailable; % in the read buffer
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        txDevice;
        rxDevice;
        timeStep;
        
        rxBuffer;
    end
    
    methods
        function this = SimulatedSerial(varargin)
            RX = [];
            TX = [];
            TIME_STEP = 30;
            Port = [];
            BaudRate = -1; %#ok<PROP>
                        
            parseVarargin(varargin{:});
            
            if isempty(RX) || isempty(TX)
                error('RX and TX must both be provided.');
            end
            
            this.rxDevice = RX;
            this.txDevice = TX;
            
            if ~isempty(Port)
                this.Port = Port;
            else 
                assert(wb_receiver_get_channel(this.rxDevice) == ...
                    wb_emitter_get_channel(this.txDevice), ...
                    'Expecint receiver and emitter to be set to same channel.');
            end
            
            this.BaudRate = BaudRate; %#ok<PROP>
            this.timeStep = TIME_STEP;
            
            this.rxBuffer = [];
                       
            this.Status = 'closed';
        end
                
        function data = fread(this, dims, precision)
            if ~strcmp(this.Status, 'open')
                error(['SimulatedSerial object must be opened with ' ...
                    'fopen before reading.']);
            end
            
            if nargin < 3 
                precision = [];
                if nargin < 2
                    dims = [];
                end
            end
            
            bytesAvailable = this.BytesAvailable;
            
            if isempty(dims)
                data = this.rxBuffer;
                this.rxBuffer = [];
            else
                % Read out of the RX buffer
                numBytes = prod(dims);
                if bytesAvailable < numBytes
                    data = [];
                else
                    data = reshape(this.rxBuffer(1:numBytes), dims);
                    this.rxBuffer(1:numBytes) = [];
                end
            end
            
            if ~isempty(precision)
                precisionFcn = str2func(precision);
                data = precisionFcn(data);
            end
        end
        
        function fwrite(this, data, precision)
            if ~strcmp(this.Status, 'open')
                error(['SimulatedSerial object must be opened with ' ...
                    'fopen before writing.']);
            end
            
            % Write to the TX buffer
            if nargin > 2 && ~isempty(precision)
                precisionFcn = str2func(precision);
                data = precisionFcn(data);
            end
            wb_emitter_send(this.txDevice, data);
        end
        
        function N = get.BytesAvailable(this)
            % Suck all the packets out of the queue
            while wb_receiver_get_queue_length(this.rxDevice) > 0
                packet = wb_receiver_get_data(this.rxDevice, 'uint8');
                this.rxBuffer = [this.rxBuffer row(packet)];
                wb_receiver_next_packet(this.rxDevice);
            end
            N = length(this.rxBuffer);
            
            % fprintf('%d bytes available on channel %d\n', N, ...
            %    wb_receiver_get_channel(this.rxDevice));
            
        end
                
        function fopen(this)
            wb_receiver_enable(this.rxDevice, this.timeStep);
            this.Status = 'open';
        end
        
        function fclose(this)
            wb_receiver_disable(this.rxDevice);
            this.Status = 'closed';
        end
        
        function delete(~)
            
        end
        
        function port = get.Port(this)
           port = wb_receiver_get_channel(this.rxDevice);
        end
        
        function set.Port(this, newPort)
           if strcmp(this.Status, 'open') 
               error('You cannot change the Port while the device is open.');
           end
           
           wb_receiver_set_channel(this.rxDevice, newPort);
           wb_emitter_set_channel(this.txDevice, newPort);
        end
    end
    
end