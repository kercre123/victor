classdef VisionMarkerTrained
        
    properties(Constant = true)
        
        %TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/lettersWithFiducials';
        TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/symbolsWithFiducials';
        
        ProbeParameters = struct( ...
            'Radius', 0.03, ...  % As a fraction of a canonical unit square 
            'NumAngles', 8, ...       % How many samples around ring to sample
            'Method', 'mean');        % How to combine points in a probe
                
        MinContrastRatio = 1.25;  % bright/dark has to be at least this
        
        SquareWidthFraction = 0.1; % as a fraction of the marker image width
        FiducialPaddingFraction = 1; % as a Fraction of SquareWidth
        
        ProbePattern = VisionMarkerTrained.CreateProbePattern();
        
        ProbeTree = VisionMarkerTrained.LoadProbeTree();
        
        % X-Z Plane: 
        Canonical3dCorners = [-1 0 1; -1 0 -1; 1 0 1; 1 0 -1];
        % X-Y Plane: Canonical3dCorners = [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
        
    end % Constant Properties
    
    methods(Static = true)
        
        img = AddFiducial(img, varargin);
        AddFiducialBatch(inputDir, outputDir, varargin);
        TrainProbeTree(varargin);
        [squareWidth_pix, padding_pix] = GetFiducialPixelSize(imageSize);
        
    end % Static Methods
    
    methods(Static = true, Access = 'protected')
        
        pattern = CreateProbePattern();
        
        tree = LoadProbeTree();
        
        outputString = GenerateEmbeddedMarkerDefinitionCode(varargin);
                
    end % Protected Static Methods
    
    properties(SetAccess = 'protected')
     
        codeID;
        codeName;
        
        corners;
        H;
        
        name;
        pose;
        fiducialSize;
        
        isValid;
        
    end % Properties
                
    methods 

        function this = VisionMarkerTrained(img, varargin)
            
            Corners = [];
            Pose = [];
            Size = 1;
                        
            parseVarargin(varargin{:});
            
            assert(~isempty(VisionMarkerTrained.ProbeTree), 'No probe tree loaded.');
    
%             if isempty(Corners)
%                 Corners = [
%                 [nrows,ncols,~] = size(img);
%                 assert(nrows==ncols, 'Image must be square if no corners are provided.');
%                 [~, padding] = VisionMarkerTrained.GetFiducialPixelSize(nrows);
%                 Corners = [padding padding ncols-padding ncols-padding; 
%                     padding nrows-padding padding nrows-padding]';
%             end
            
            maxVal = max(img(:));
            minVal = min(img(:));
            if maxVal < VisionMarkerTrained.MinContrastRatio*minVal
                warning('VisionMarkerTrained:TooLittleContrast', ...
                    'Not enough contrast to create VisionMarkerTrained.');
                this.isValid = false;
            else
                threshold = (min(img(:)) + max(img(:)))/2;

                if isempty(Corners)
                    Corners = [0 0; 0 1; 1 0; 1 1];
                end
                    
                tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
                this.H = tform.tdata.T';
                [this.codeName, this.codeID] = TestTree( ...
                    VisionMarkerTrained.ProbeTree, img, tform, threshold, ...
                    VisionMarkerTrained.ProbePattern);
                
                this.corners = Corners;
                this.isValid = ~strcmp(this.codeName, 'UNKNOWN');
            end
            
            this.pose = Pose;
            this.fiducialSize = Size;
            this.name = this.codeName;
            
        end % Constructor VisionMarkerTrained()
       
        h = Draw(this, varargin);
        
        % For backwards compatibility to lowercase "draw" in Marker2D
        function h = draw(this, varargin)
            where = [];
            
            DrawArgs = parseVarargin(varargin{:});
            
            h = Draw(this, 'Parent', where, DrawArgs{:});
        end
        
         
        function pos3d = GetPosition(this, wrtPose)
           
            % A square with corners (+/- 1, +/- 1):
            canonicalSquare = this.size/2 * VisionMarkerTrained.Canonical3dCorners;
            if nargin < 2
                P = this.pose;
            else
                P = this.pose.getWithRespectTo(wrtPose);
            end
            
            pos3d = P.applyTo(canonicalSquare);
            
        end

        
    end % Public Methods
    
end % CLASSDEF TrainedVisionMarker
