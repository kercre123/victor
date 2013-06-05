classdef BlockMarker
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        blockType;
        faceType;
        corners;
        topOrient;
        marker;
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        topSideLUT = struct('down', [2 4], 'up', [1 3], ...
            'left', [1 2], 'right', [3 4]);
    end
    
    methods(Access = 'public')
       
        function this = BlockMarker(blockType_, faceType_, corners_, topOrient_)
            this.marker = [0 0; 1 0; 0 1; 1 1];
            
            this.blockType = blockType_;
            this.faceType  = faceType_;
            this.corners   = corners_;
            this.topOrient = topOrient_;
            
            switch(this.topOrient)
                case 'up'
                    % nothing to do
                case 'right'
                    this.corners = this.corners([3 1 4 2],:);
                    this.marker = this.marker([3 1 4 2],:);
                case 'left'
                    this.corners = this.corners([2 4 1 3],:);
                    this.marker = this.marker([2 4 1 3],:);
                case 'down'
                    this.corners = this.corners([4 3 2 1],:);
                    this.marker = this.marker([4 3 2 1],:);
                otherwise
                    error('Unrecognized topOrient "%s"', this.topOrient);
            end
            
        end
         
    end
   
    
end % CLASSDEF BlockMarker