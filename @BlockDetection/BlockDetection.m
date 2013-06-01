classdef BlockDetection
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        blockType;
        faceType;
        corners;
        topOrient;
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        topSideLUT = struct('down', [2 4], 'up', [1 3], ...
            'left', [1 2], 'right', [3 4]);
    end
    
    methods(Access = 'public')
       
        function this = BlockDetection(blockType_, faceType_, corners_, topOrient_)
            
            this.blockType = blockType_;
            this.faceType  = faceType_;
            this.corners   = corners_;
            this.topOrient = topOrient_;
            
        end
         
    end
   
    
end % CLASSDEF BlockDetection