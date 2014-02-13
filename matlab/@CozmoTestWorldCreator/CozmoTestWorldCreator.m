classdef CozmoTestWorldCreator
    
    properties(SetAccess = 'protected')
        worldFile;
        usbChannel;
        useOffboardVision;
    end
    
    methods
        
        function this = CozmoTestWorldCreator(varargin)
            WorldFile = '';
            UseOffboardVision = false;
            USBChannel = 10;
            
            parseVarargin(varargin{:});
            
            this.worldFile = WorldFile;
            this.usbChannel = USBChannel;
            this.useOffboardVision = UseOffboardVision;
            
            StartWorldFile(this);
            
        end % Constructor CozmoTestWorld
        
        StartWorldFile(this);
        
        AddRobot(this, varargin);
        
        AddBlock(this, varargin);
       
    end
    
end % classdef CozmoTestWorldCreator








