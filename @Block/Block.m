classdef Block < handle
       
       
    properties(GetAccess = 'public', SetAccess = 'public')
        blockType;
        
        markers;
        
        Xmodel;
        Ymodel;
        Zmodel;
        
        X;
        Y;
        Z;
        
        color;
        handle;
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        origin;
        faces;
        numMarkers;
                
    end
    
    properties(GetAccess = 'public', SetAccess = 'public', ...
            Dependent = true)
        
        frame;
        
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        frameProtected = Frame();
        
    end
    
    methods(Access = 'public')
        
        function this = Block(blockType)
            
            this.blockType = blockType;
            createModel(this);
            
            this.X = this.Xmodel;
            this.Y = this.Ymodel;
            this.Z = this.Zmodel;
            
        end
        
        function M = getFaceMarker(this, faceType)
            assert(isKey(this.markers, faceType), ...
                'Invalid face for this block.');
            M = this.markers(faceType);
        end
        
    end % METHODS (public)
    
    methods % Dependent get/set 
        
        function o = get.origin(this)
            % return current origin
            o = [this.X(1); this.Y(1); this.Z(1)];
        end
        
        function f = get.faces(this)
            f = row(this.markers.keys);
        end
        
        function N = get.numMarkers(this)
            N = this.markers.Count;
        end
        
        function F = get.frame(this)
            F = this.frameProtected;
        end
        
        function set.frame(this, F)
            assert(isa(F, 'Frame'), ...
                'Must set frame property to a Frame object.');
            this.frameProtected = F;
            
            % Update this block's position/orientation in the world:
            [this.X, this.Y, this.Z] = this.frameProtected.applyTo( ...
                this.Xmodel, this.Ymodel, this.Zmodel);
            
            % Also update the constituent faces:
            faces = this.faces;
            for i = 1:length(faces)
                M = this.markers(faces{i});
                M.frame = this.frameProtected;
            end
        end
        
    end % METHODS get/set for Dependent Properties
    
end % CLASSDEF Block