classdef BlockMarker3D < handle
    
    properties(GetAccess = 'public', Constant = true)
        % Width of the inside or outside of the square fiducial in mm.  
        % Sets scale for the whole world!
        Width = BlockMarker3D.setWidth(BlockMarker2D.UseOutsideOfSquare);
        
    end
    
    methods(Access = 'protected', Static = true)
        
        function width = setWidth(useOutsideOfSquare)
        % NOTE: These are measured from Webots world
        
        if useOutsideOfSquare
            width = 31.9; 
        else
            width = 24.2;
        end
        
        end % FUNCTION setWidth()
    
    end    
    
    properties(GetAccess = 'public', SetAccess = 'public')
        
        block; % handle to parent block
        
        faceType;
                       
        model;
        
        ID;
        
        pose;
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        origin;
    end
        
    properties(GetAccess = 'protected', SetAccess = 'protected')
       
        handles;
        
    end
    
    methods(Access = 'public')
       
        function this = BlockMarker3D(parentBlock, faceType_, poseInit, id)
            
            assert(isa(parentBlock, 'Block'), ...
                'parentBlock should be a Block object.');
            assert(isa(poseInit, 'Pose'), ...
                'frameInit should be a Pose object.');
            
            this.ID = id;
            %this.pose = Pose();
            
            this.block = parentBlock;
            this.faceType  = faceType_;
  
            %this.model = this.Width*[0 0 0; 0 0 -1; 1 0 0; 1 0 -1];
            this.model = this.Width/2*[-1 0 1; -1 0 -1; 1 0 1; 1 0 -1];
            %this.model = poseInit.applyTo(this.model);
            this.pose = poseInit;
            
        end
        
        function varargout = getPosition(this, poseIn)
            % Gets position of the model points (w.r.t. given pose).
            varargout = cell(1,nargout);
            if nargin < 2
                P = this.pose;
            else
                P = this.pose.getWithRespectTo(poseIn);
            end
            
            [varargout{:}] = P.applyTo(this.model);
            
        end
         
    end % METHODS (public)
   
    methods
        
        function o = get.origin(this)
            % return current origin
            o = this.pose.applyTo(this.model(1,:));
        end
        
    end
    
end % CLASSDEF BlockMarker