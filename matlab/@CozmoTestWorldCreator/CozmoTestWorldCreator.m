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
            
            % NOTE: the BlockTypes used below must match those defined in BlockDefinitions.h!
            % TODO: auto build this LUT ased on BlockDefinitions.h
            this.blockTypeLUT = containers.Map('keyType', 'char', 'valueType', 'double');
            
            this.blockTypeLUT('bullseye2') = 1;
            this.blockTypeLUT('fire')      = 2;
            this.blockTypeLUT('sqtarget')  = 3;
            this.blockTypeLUT('angryFace') = 4;
            this.blockTypeLUT('star5')     = 5;
            this.blockTypeLUT('dice')      = 6;

            StartWorldFile(this);
            
        end % Constructor CozmoTestWorld
        
        StartWorldFile(this);
        
        AddRobot(this, varargin);
        
        AddBlock(this, varargin);
       
        function Run(this)
           system(['open ' this.worldFile]); 
        end
    end
    
end % classdef CozmoTestWorldCreator








