classdef Block < handle
              
    properties(GetAccess = 'public', SetAccess = 'public')
        blockType;
        
        markers;
        faceTypeToIndex;
        
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
        
        function this = Block(blockType, firstMarkerID)
            
            this.blockType = blockType;
            createModel(this, firstMarkerID);
            
            this.X = this.Xmodel;
            this.Y = this.Ymodel;
            this.Z = this.Zmodel;
            
        end
        
        function M = getFaceMarker(this, faceType)
            assert(isKey(this.faceTypeToIndex, faceType), ...
                'Invalid face for this block.');
            index = this.faceTypeToIndex(faceType);
            M = this.markers{index};
        end
        
    end % METHODS (public)
    
    methods % Dependent get/set 
        
        function o = get.origin(this)
            % return current origin
            o = [this.X(1); this.Y(1); this.Z(1)];
        end
                
        function N = get.numMarkers(this)
            N = length(this.markers);
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
            for i = 1:length(this.markers)
                M = this.markers{i};
                M.frame = this.frameProtected;
            end
        end
        
    end % METHODS get/set for Dependent Properties
    
end % CLASSDEF Block