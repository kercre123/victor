classdef CozmoTestWorldCreator
    
    properties(SetAccess = 'protected')
        worldFile;
        usbChannel;
        useOffboardVision;
        checkRobotPose;
        blockTypeLUT;        
    end
    
    methods
        
        function this = CozmoTestWorldCreator(varargin)
            WorldFile = '';
            UseOffboardVision = false;
            USBChannel = 10;
            CheckRobotPose = true;
            
            parseVarargin(varargin{:});
            
            this.worldFile = WorldFile;
            this.usbChannel = USBChannel;
            this.useOffboardVision = UseOffboardVision;
            this.checkRobotPose = CheckRobotPose;

            StartWorldFile(this);
            
        end % Constructor CozmoTestWorld
        
        StartWorldFile(this);
        
        AddRobot(this, varargin);
        
        AddBlock(this, varargin);
       
        AddRamp(this, varargin);
        
        function Run(this)
           system(['open ' this.worldFile]); 
        end
    end
    
end % classdef CozmoTestWorldCreator








