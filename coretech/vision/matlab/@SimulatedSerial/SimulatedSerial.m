classdef SimulatedSerial < handle
   
    % Unused properties to make this look like a Serial object.
    properties
        BaudRate;
        Port;
    end
    
    properties(SetAccess = 'protected', Dependent = true)
        BytesAvailable;
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        buffer;
        locked;
    end
    
    methods
        function this = SimulatedSerial()
            this.locked = false;
            this.buffer = [];
        end
        
        function data = fread(this, dims, precision)

            numBytes = prod(dims);
            if this.BytesAvailable < numBytes
                data = [];
            else
                this.locked = true;
                data = reshape(this.buffer(1:numBytes), dims);
                this.buffer(1:numBytes) = [];
                this.locked = false;
                
                precisionFcn = str2func(precision);
                data = precisionFcn(data);
            end
        
        end
        
        function fwrite(this, data, precision)
            precisionFcn = str2func(precision);
            this.locked = true;
            this.buffer = [this.buffer row(precisionFcn(data))];
            this.locked = false;
        end
        
        function N = get.BytesAvailable(this)
            N = length(this.buffer);
        end
        
        function c = getChar(this, peek)
            % Get the first element of the buffer. 
            % If peek is true, the element is also removed from the buffer.  
            % Otherwise (default), the element remains in the buffer.
            
            if nargin < 2
                peek = false;
            end
            
            c = this.buffer(1);
            if ~peek
               this.buffer = this.buffer(2:end); 
            end
        end
        
        function putChar(this, data)
            % Append data to buffer
            this.buffer = [this.buffer row(char(data))];
        end
        
        function fopen(~)
            
        end
        
        function fclose(~)
            
        end
        
        function delete(~)
            
        end
    end
    
end