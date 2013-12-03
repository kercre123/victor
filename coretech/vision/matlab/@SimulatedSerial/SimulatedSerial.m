classdef SimulatedSerial < handle
   
    % Unused properties to make this look like a Serial object.
    properties
        BaudRate;
        Port;
    end
    
    properties(SetAccess = 'protected', Dependent = true)
        BytesAvailable; % in the read buffer
        TxBytesAvailable;
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        tx_buffer;
        rx_buffer;
        %locked;
    end
    
    methods
        function this = SimulatedSerial()
            %this.locked = false;
            this.tx_buffer = [];
            this.rx_buffer = [];
        end
        
        function data = fread(this, dims, precision)
            % Read out of the RX buffer
            numBytes = prod(dims);
            if this.BytesAvailable < numBytes
                data = [];
            else
                %this.locked = true;
                data = reshape(this.buffer(1:numBytes), dims);
                this.rx_buffer(1:numBytes) = [];
                %this.locked = false;
                
                precisionFcn = str2func(precision);
                data = precisionFcn(data);
            end
        
        end
        
        function fwrite(this, data, precision)
            % Write to the TX buffer
            precisionFcn = str2func(precision);
            this.tx_buffer = [this.tx_buffer row(precisionFcn(data))];
        end
        
        function N = get.BytesAvailable(this)
            N = length(this.rx_buffer);
        end
        
        function N = get.TxBytesAvailable(this)
            N = length(this.tx_buffer);
        end
        
        function c = getCharFrom(this, peek)
            % Get a char sent by this object, i.e. get the first element 
            % of the TX buffer. 
            % If peek is true, the element is also removed from the buffer.  
            % Otherwise (default), the element remains in the buffer.
            
            if nargin < 2
                peek = false;
            end
            
            c = this.tx_buffer(1);
            if ~peek
               this.tx_buffer = this.tx_buffer(2:end); 
            end
        end
        
        function sendCharTo(this, data)
            % Send a char to this object, i.e. append a char to its RX
            % buffer
            this.rx_buffer = [this.rx_buffer row(char(data))];
        end
        
        function fopen(~)
            
        end
        
        function fclose(~)
            
        end
        
        function delete(~)
            
        end
    end
    
end