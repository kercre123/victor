classdef VisionMarkerTrained
        
    properties(Constant = true)
        
        %TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/lettersWithFiducials';
        TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/symbolsWithFiducials/rotated';
        
        ProbeParameters = struct( ...
            'Radius', 0.02, ...  % As a fraction of a canonical unit square 
            'NumAngles', 8, ...       % How many samples around ring to sample
            'Method', 'mean');        % How to combine points in a probe
                
        MinContrastRatio = 1.25;  % bright/dark has to be at least this
        
        SquareWidthFraction = 0.1; % as a fraction of the fiducial width
        FiducialPaddingFraction = 0.1; % as a fraction of the fiducial width
        
        ProbePattern = VisionMarkerTrained.CreateProbePattern();
        
        ProbeTree = VisionMarkerTrained.LoadProbeTree();
        
        DarkProbes   = VisionMarkerTrained.CreateProbes('dark'); 
        BrightProbes = VisionMarkerTrained.CreateProbes('bright');
        
        % X-Z Plane: 
        Canonical3dCorners = [-1 0 1; -1 0 -1; 1 0 1; 1 0 -1];
        % X-Y Plane: Canonical3dCorners = [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
        
    end % Constant Properties
    
    methods(Static = true)
        
        img = AddFiducial(img, varargin);
        AddFiducialBatch(inputDir, outputDir, varargin);
        TrainProbeTree(varargin);
        [squareWidth_pix, padding_pix] = GetFiducialPixelSize(imageSize, imageSizeType);
        corners = GetFiducialCorners(imageSize);
        threshold = ComputeThreshold(img, tform);
        outputString = GenerateHeaderFile(varargin);
    end % Static Methods
    
    methods(Static = true, Access = 'protected')
        
        pattern = CreateProbePattern();
        
        tree = LoadProbeTree();
        
        probes = CreateProbes(probeType); 
                
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
            
            if isempty(Corners)
                Corners = [0 0; 0 1; 1 0; 1 1];
            end
            
            this.corners = Corners;
            
            tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
            this.H = tform.tdata.T';
            
            threshold = VisionMarkerTrained.ComputeThreshold(img, tform);
            
            if threshold < 0
                %warning('VisionMarkerTrained:TooLittleContrast', ...
                %    'Not enough contrast to create VisionMarkerTrained.');
                this.isValid = false;
            else
                [this.codeName, this.codeID] = TestTree( ...
                    VisionMarkerTrained.ProbeTree, img, tform, threshold, ...
                    VisionMarkerTrained.ProbePattern);
                
                if any(strcmp(this.codeName, {'UNKNOWN', 'ALL_WHITE', 'ALL_BLACK'}))
                    this.isValid = false;
                else
                    [verificationResult, verifiedID] = TestTree( ...
                        VisionMarkerTrained.ProbeTree.verifiers(this.codeID), ...
                        img, tform, threshold, VisionMarkerTrained.ProbePattern);
                    
                    this.isValid = verifiedID == 2;
                    if this.isValid
                        assert(strcmp(verificationResult, this.codeName));
                        
                        underscoreIndex = find(this.codeName == '_');
                        if ~isempty(underscoreIndex)
                            assert(length(underscoreIndex) == 1, ...
                                'There should be no more than 1 underscore in the code name: "%s".', this.codeName);
                            
                            angleStr = this.codeName((underscoreIndex+1):end);
                            this.codeName = this.codeName(1:(underscoreIndex-1));
                            
                            % Reorient the corners if there's an angle in
                            % the name.  After this the line between the 
                            % first and third corner will be the top side.
                            reorder = [1 3; 2 4]; % canonical corner ordering
                            switch(angleStr)
                                case '000'
                                    % nothing to do
                                case '090'
                                    reorder = rot90(rot90(rot90(reorder)));
                                    %reorder = rot90(reorder);
                                case '180'
                                    reorder = rot90(rot90(reorder));
                                case '270'
                                    %reorder = rot90(rot90(rot90(reorder)));
                                    reorder = rot90(reorder);
                                otherwise                                    
                                    error('Unrecognized angle string "%s"', angleStr);
                            end
                            
                            this.corners = this.corners(reorder(:),:);
                            
                        end
                    end
                end
            end % IF threshold < 0
            
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
