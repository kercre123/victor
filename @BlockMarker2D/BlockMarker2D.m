classdef BlockMarker2D < handle
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        blockType;
        faceType;
        corners;
        topOrient;       
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        handles;
        topSideLUT = struct('down', [2 4], 'up', [1 3], ...
            'left', [1 2], 'right', [3 4]);
    end
    
    methods(Access = 'public')
       
        function this = BlockMarker2D(blockType_, faceType_, corners_, topOrient_)
          
            this.blockType  = blockType_;
            this.faceType   = faceType_;
            this.corners    = corners_;
            this.topOrient  = topOrient_;
            
            switch(this.topOrient)
                case 'up'
                    reorder = 1:4;
                case 'right'
                    reorder = [3 1 4 2];
                case 'left'
                    reorder = [2 4 1 3];
                case 'down'
                    reorder = [4 3 2 1];
                otherwise
                    error('Unrecognized topOrient "%s"', this.topOrient);
            end
            
            this.corners = this.corners(reorder,:);
           
        end
         
    end % METHODS (public)
       
end % CLASSDEF BlockMarker2D