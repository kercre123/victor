classdef BlockMarker3D < handle
    
    properties(GetAccess = 'public', Constant = true)
        % Width of the inside of the square fiducial
        Width = 25; % in mm, sets scale for the whole world
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
            
            this.block = parentBlock;
            this.faceType  = faceType_;
  
            this.model = this.Width*[0 0 0; 0 0 -1; 1 0 0; 1 0 -1];
            this.model = poseInit.applyTo(this.model);
            
        end
        
        function varargout = getPosition(this, poseIn)
            varargout = cell(1,nargout);
            if nargin < 2
                [varargout{:}] = this.pose.applyTo(this.model);
            else
                [varargout{:}] = poseIn.applyTo(this.model);
            end
        end
         
    end % METHODS (public)
   
    methods
        
        function o = get.origin(this)
            % return current origin
            o = this.pose.applyTo(this.model(1,:));
        end
        
    end
    
end % CLASSDEF BlockMarker