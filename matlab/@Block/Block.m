classdef Block < handle
              
    properties(GetAccess = 'public', SetAccess = 'public')
        blockType;
        
        markers;
        faceTypeToIndex;
        
        model;
        
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
        
        pose;
    end
        
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        poseProtected;
        
    end
    
    
    methods(Access = 'public')
        
        function this = Block(blockType, firstMarkerID)
            
            this.poseProtected = Pose();
            this.blockType = blockType;
                        
            createModel(this, firstMarkerID);
            
        end
        
        function varargout = getPosition(this, poseIn)
            varargout = cell(1,nargout);
            if nargin < 2
                [varargout{:}] = this.pose.applyTo(this.model);
            else
                [varargout{:}] = poseIn.applyTo(this.model);
            end
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
            o = this.pose.applyTo(this.model(1,:));
        end
                
        function N = get.numMarkers(this)
            N = length(this.markers);
        end
        
        function P = get.pose(this)
            P = this.poseProtected;
        end
        
        function set.pose(this, P)
            assert(isa(P, 'Pose'), ...
                'Must set pose property to a Pose object.');
            this.poseProtected = P;
            
            % Also update the constituent faces' poses:
            for i = 1:length(this.markers)
                M = this.markers{i};
                M.pose = this.poseProtected;
            end
        end
        
    end % METHODS get/set for Dependent Properties
    
end % CLASSDEF Block