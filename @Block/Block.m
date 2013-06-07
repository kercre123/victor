classdef Block < handle
       
       
    properties(GetAccess = 'public', SetAccess = 'public')
        % TODO: these should all probably be set-protected
        position;
        orientation;
        
        modelX;
        modelY;
        modelZ;
        
        X;
        Y;
        Z;
        
        color;
        handle;
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        origin;
        
    end
    
    methods(Access = 'public')
        
        function this = Block(marker, frame)
            [this.modelX, this.modelY, this.modelZ, this.color] = getModel( ...
                marker.blockType, marker.faceType, marker.topOrient);
            
            if nargin < 2 || isempty(frame)
                this.X = this.modelX;
                this.Y = this.modelY;
                this.Z = this.modelZ;
            else
                [this.X, this.Y, this.Z] = frame.applyTo( ...
                    this.modelX, this.modelY, this.modelZ);
            end
            
        end
        
    end % METHODS (public)
    
    methods % Dependent get/set 
        
        function o = get.origin(this)
            % return current origin
            o = [this.X(1); this.Y(1); this.Z(1)];
        end
        
    end % METHODS get/set for Dependent Properties
    
end % CLASSDEF Block