classdef BlockMarker3D < handle
    
    properties(GetAccess = 'public', Constant = true)
        % Width of the inside or outside of the square fiducial in mm.  
        % Sets scale for the whole world!
        TotalWidth          = 38; % in mm
        FiducialSquareWidth = 4; % in mm
        FiducialSpacing     = 3; % spacing around fiducial square in mm
        
        SquareWidthOutside = BlockMarker3D.TotalWidth - 2*BlockMarker3D.FiducialSpacing;
        SquareWidthInside = BlockMarker3D.SquareWidthOutside - 2*BlockMarker3D.FiducialSquareWidth;
        
        ReferenceWidth = ...
            double( BlockMarker2D.UseOutsideOfSquare) * BlockMarker3D.SquareWidthOutside + ...
            double(~BlockMarker2D.UseOutsideOfSquare) * BlockMarker3D.SquareWidthInside;
        
        CodeSquareWidth = (BlockMarker3D.TotalWidth - ...
            4*BlockMarker3D.FiducialSpacing - ...
            2*BlockMarker3D.FiducialSquareWidth) / ...
            size(BlockMarker2D.Layout, 1);
                
        DockingTarget = 'FourDots'; % 'SingleDot' or 'FourDots'
        
        DockingDotSpacing = BlockMarker3D.setDockingDotSpacing(BlockMarker3D.DockingTarget);
        DockingDotWidth   = BlockMarker3D.setDockingDotWidth(BlockMarker3D.DockingTarget);
        
        DockingDistance = 100;
    end
    
    methods(Access = 'protected', Static = true)
        function spacing = setDockingDotSpacing(targetType)
            switch(targetType)
                case 'FourDots'
                    spacing = BlockMarker3D.CodeSquareWidth/2;
                case 'SingleDot'
                    spacing = 0;
                otherwise
                    error('Unrecognized docking target type "%s".', ...
                        targetType);
            end
        end
        
        function width = setDockingDotWidth(targetType)
            switch(targetType)
                case 'FourDots'
                    width = BlockMarker3D.DockingDotSpacing/2;
                case 'SingleDot'
                    width = BlockMarker3D.CodeSquareWidth/2;
                otherwise
                    error('Unrecognized docking target type "%s".', ...
                        targetType);
            end
        end        
    end
    
    properties(GetAccess = 'public', SetAccess = 'public')
        
        block; % handle to parent block
        
        faceType;
                       
        model;
        dockingTarget;
        dockingTargetBoundingBox;
        
        preDockPose;
        
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
  
            % A square with corners (+/- 1, +/- 1):
            canonicalSquare = [-1 0 1; -1 0 -1; 1 0 1; 1 0 -1];
            
            %this.model = this.Width*[0 0 0; 0 0 -1; 1 0 0; 1 0 -1];
            this.model = this.ReferenceWidth/2*canonicalSquare;
            %this.model = poseInit.applyTo(this.model);
            this.pose = poseInit;
            
            this.preDockPose = Pose([0 0 0], [0 -this.DockingDistance 0]);
            this.preDockPose.parent = this.pose;
            
            switch(BlockMarker3D.DockingTarget)
                case 'FourDots'
                    this.dockingTarget = this.DockingDotSpacing/2*canonicalSquare;
                case 'SingleDot'
                    this.dockingTarget = [0 0 0];
                otherwise
                    error('Unrecognized docking target type "%s".', ...
                        BlockMarker3D.DockingTarget);
            end
                        
            this.dockingTargetBoundingBox = ...
                this.CodeSquareWidth/2*canonicalSquare;
        end
        
        function varargout = getPosition(this, poseIn, whichPart)
            % Gets position of the model points (w.r.t. given pose).
            varargout = cell(1,nargout);
            
            if nargin < 3
                whichPart = 'marker';
            end
            
            if nargin < 2
                P = this.pose;
            else
                P = this.pose.getWithRespectTo(poseIn);
            end
            
            switch(lower(whichPart))
                case 'marker'
                    modelPts = this.model;
                case 'dockingtarget'
                    modelPts = this.dockingTarget;
                case 'dockingtargetboundingbox'
                    modelPts = this.dockingTargetBoundingBox;
                otherwise
                    error('Unrecognized BlockMarker3D part to get position of.');
            end
            
            [varargout{:}] = P.applyTo(modelPts);
            
        end
                
    end % METHODS (public)
   
    methods
        
        function o = get.origin(this)
            % return current origin
            o = this.pose.applyTo(this.model(1,:));
        end
        
    end
    
end % CLASSDEF BlockMarker