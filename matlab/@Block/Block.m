classdef Block < handle
              
    properties(GetAccess = 'public', SetAccess = 'public')
        blockType;
        
        markers;
        faceTypeToIndex;
        
        model;
        dims;
        
        color;
        handle;
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        origin;
        mindim;
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
            % Gets position w.r.t. root Pose:
            varargout = cell(1,nargout);
            if nargin < 2
                P = this.pose;
            else
                P = poseIn;
            end
            
            % Chain poses together up to root node:
            while ~isempty(P.parent)
                %P = P * P.parent;
                newParent = P.parent.parent;
                P = P * P.parent;
                P.parent = newParent;
            end
            
            [varargout{:}] = P.applyTo(this.model);
            
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
        
        function d = get.mindim(this)
            d = min(this.dims);
        end
        
    end % METHODS get/set for Dependent Properties
    
end % CLASSDEF Block