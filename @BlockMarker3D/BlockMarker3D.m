classdef BlockMarker3D < handle
    
    properties(GetAccess = 'public', Constant = true)
        % Width of the inside of the square fiducial
        Width = 25; % in mm, sets scale for the whole world
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        block; % handle to parent block
        
        faceType;
                       
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
        
        frameProtected = Frame();
        handles;
        
    end
    
    methods(Access = 'public')
       
        function this = BlockMarker3D(parentBlock, faceType_, frameInit)
            assert(isa(parentBlock, 'Block'), ...
                'parentBlock should be a Block object.');
            assert(isa(frameInit, 'Frame'), ...
                'frameInit should be a Frame object.');
            
            this.block = parentBlock;
            this.faceType  = faceType_;
  
            this.Pmodel = this.Width*[0 0 0; 0 0 -1; 1 0 0; 1 0 -1];
            this.Pmodel = frameInit.applyTo(this.Pmodel);
            
            this.P = this.Pmodel;
        end
         
    end % METHODS (public)
   
    methods
        function F = get.frame(this)
            F = this.frameProtected;
        end
        
        function set.frame(this, F)
            assert(isa(F, 'Frame'), 'Must provide a Frame object');
            this.frameProtected = F;
            this.P = this.frameProtected.applyTo(this.Pmodel);
        end
        
        function o = get.origin(this)
            o = this.P(1,:);
        end
    end
    
end % CLASSDEF BlockMarker