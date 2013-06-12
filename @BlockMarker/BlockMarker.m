classdef BlockMarker < handle
    
    properties(GetAccess = 'public', Constant = true)
        Width = 384; % in mm, sets scale for the whole world
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        blockType;
        faceType;
        imgCorners;
        topOrient;
                
        Pmodel;
        P;
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'public', ...
            Dependent = true)
        
        frame;
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
       
        origin;
    end
    
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        frame_;
        handles;
        topSideLUT = struct('down', [2 4], 'up', [1 3], ...
            'left', [1 2], 'right', [3 4]);
    end
    
    methods(Access = 'public')
       
        function this = BlockMarker(blockType_, faceType_, corners_, topOrient_)
            
            this.Pmodel = this.Width*[0 0 0; 1 0 0; 0 1 0; 1 1 0];
            
            this.blockType  = blockType_;
            this.faceType   = faceType_;
            this.imgCorners = corners_;
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
            
            this.imgCorners = this.imgCorners(reorder,:);
            this.Pmodel = this.Pmodel(reorder,:);
            
            this.P = this.Pmodel;
        end
         
    end % METHODS (public)
   
    methods
        function F = get.frame(this)
            F = this.frame_;
        end
        
        function set.frame(this, F)
            assert(isa(F, 'Frame'), 'Must provide a Frame object');
            this.frame_ = F;
            this.P = this.frame_.applyTo(this.Pmodel);
        end
        
        function o = get.origin(this)
            o = this.P(1,:);
        end
    end
    
end % CLASSDEF BlockMarker