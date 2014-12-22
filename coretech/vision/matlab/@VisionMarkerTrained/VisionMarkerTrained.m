classdef VisionMarkerTrained
        
    properties(Constant = true)
        
        %TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/lettersWithFiducials/rotated';
        %TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/symbolsWithFiducials/unpadded/rotated';
        
        TrainingImageDir = { ...
            '~/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/rotated', ... '~/Box Sync/Cozmo SE/VisionMarkers/matWithFiducials/unpadded/rotated', ...
            '~/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/rotated', ...
            '~/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/rotated', ...}; % '~/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/rotated', ...
            '~/Box Sync/Cozmo SE/VisionMarkers/creepTest/withFiducials/rotated'};
                
        ProbeParameters = struct( ...
            'GridSize', 32, ...            %'Radius', 0.02, ...  % As a fraction of a canonical unit square 
            'NumAngles', 4, ...       % How many samples around ring to sample
            'Method', 'mean');        % How to combine points in a probe
                
        MinContrastRatio = 0; %1.25;  % bright/dark has to be at least this
        
        SquareWidthFraction = 0.1; % as a fraction of the fiducial width
        FiducialPaddingFraction = 0.1; % as a fraction of the fiducial width
        
        ProbeRegion = [VisionMarkerTrained.SquareWidthFraction+VisionMarkerTrained.FiducialPaddingFraction ...
            1-(VisionMarkerTrained.SquareWidthFraction+VisionMarkerTrained.FiducialPaddingFraction)];

        ProbePattern = VisionMarkerTrained.CreateProbePattern();
        
        % NOTE: this assumes large-files repo is alongside products-cozmo
        SavedTreePath = fullfile(fileparts(mfilename('fullpath')), '..', '..', '..', '..', '..', ...
          'products-cozmo-large-files', 'trainedTrees');
      
        ProbeTree = VisionMarkerTrained.LoadProbeTree();
                
        DarkProbes   = VisionMarkerTrained.CreateProbes('dark'); 
        BrightProbes = VisionMarkerTrained.CreateProbes('bright');
        
        % X-Z Plane: 
        Canonical3dCorners = [-1 0 1; -1 0 -1; 1 0 1; 1 0 -1];
        % X-Y Plane: Canonical3dCorners = [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
        
    end % Constant Properties
    
    methods(Static = true)
        
        [img, alpha] = AddFiducial(img, varargin);
        AddFiducialBatch(inputDir, outputDir, varargin);
        
        trainingState = ExtractProbeValues(varargin)
        VisualizeExtractedProbes(data, varargin)
        
        probeTree = TrainProbeTree(varargin);
        
        [squareWidth_pix, padding_pix] = GetFiducialPixelSize(imageSize, imageSizeType);
        %Corners = GetMarkerCorners(imageSize, isPadded);
        corners = GetFiducialCorners(imageSize, isPadded);
        [threshold, bright, dark] = ComputeThreshold(img, tform, method);
        outputString = GenerateHeaderFiles(varargin);
        [numMulticlassNodes, numVerificationNodes] = GetNumTreeNodes(tree);

        CreateTestImage(varargin);
        CreatePrintableCodeSheet(varargin);
        
        [probeValues, X, Y] = GetProbeValues(img, tform);
        
    end % Static Methods
    
    methods(Static = true, Access = 'protected')
        
        pattern = CreateProbePattern();
        
        tree = LoadProbeTree();
                
        probes = CreateProbes(probeType); 
        
    end % Protected Static Methods
    
    % TODO: change back to protected, with proper accessor methods
    properties(SetAccess = 'public')
     
        codeID;
        codeName;
        
        corners;
        H;
        
        name;
        pose;
        fiducialSize;
        
        isValid;
        
        matchDistance;
        
    end % Properties
                
    methods 

        function this = VisionMarkerTrained(img, varargin)
            
            Corners = [];
            Pose = [];
            Size = 1;
            UseSingleProbe = false;
            CornerRefinementIterations = 25;
            UseMexCornerRefinment = false;
            VerifyLabel = true;
            Initialize = true;
            ThresholdMethod = 'Otsu'; % 'Otsu' or 'FiducialProbes'
                        
            parseVarargin(varargin{:});
            
            if ~Initialize
                this.corners = Corners;
                return;
            end
            
            assert(~isempty(VisionMarkerTrained.ProbeTree), 'No probe tree loaded.');
            assert(exist('img', 'var') == true);
            
            if ischar(img)
                [img, ~, alpha] = imread(img);
                img = mean(im2double(img),3);
                img(alpha < .5) = 1;
            end
            
            if isempty(Corners)
                %Corners = [0 0; 0 1; 1 0; 1 1];
                Corners = VisionMarkerTrained.GetFiducialCorners(size(img,1), false);
            end
                        
            this.corners = Corners;
            
            try
                tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
            catch                    
                disp('Some points are co-linear');
                this.isValid = false;
                this.H = eye(3);
                return;
            end
            
            this.H = tform.tdata.T';
    
            [threshold, bright, dark] = VisionMarkerTrained.ComputeThreshold(img, tform, ThresholdMethod);

            if threshold < 0
                %warning('VisionMarkerTrained:TooLittleContrast', ...
                %    'Not enough contrast to create VisionMarkerTrained.');
                this.isValid = false;
            else
                if CornerRefinementIterations > 0
                    if UseMexCornerRefinment
                        [this.corners, this.H] = mexRefineQuadrilateral(im2uint8(img), int16(this.corners-1), single(this.H), ...
                            CornerRefinementIterations, VisionMarkerTrained.SquareWidthFraction, dark*255, bright*255, 100, 5);
                        this.corners = this.corners + 1;
                    else
                        [this.corners, this.H] = this.RefineCorners(img, 'NumSamples', 100, ... 'DebugDisplay', true,  ...
                            'MaxIterations', CornerRefinementIterations, ...
                            'DarkValue', dark, 'BrightValue', bright);
                    end
                end
                
                if iscell(VisionMarkerTrained.ProbeTree)
                  VerifyLabel = false;
                  numTrees = length(VisionMarkerTrained.ProbeTree);
                  codeNames = cell(1, numTrees);
                  codeIDs   = cell(1, numTrees);
                  for iTree = 1:numTrees
                    [codeNames{iTree}, codeIDs{iTree}] = TestTree( ...
                      VisionMarkerTrained.ProbeTree{iTree}, img, tform, threshold, ...
                      VisionMarkerTrained.ProbePattern);
                    if ~isscalar(codeIDs{iTree})
                      codeIDs{iTree} = mode(codeIDs{iTree});
                    end
                  end
                  
                  counts = hist([codeIDs{:}], 1:length(VisionMarkerTrained.ProbeTree{1}.labels));
                  
                  % Majority
                  [maxCount,this.codeID] = max(counts);
                  this.isValid = maxCount > 0.5*numTrees;
                  
%                   % Plurality (ID with most votes wins, so long as it is
%                   % more than second-most)
%                   [sortedCounts,sortedIDs] = sort(counts, 'descend');
%                   this.codeID = sortedIDs(1);
%                   this.isValid = sortedCounts(1) > sortedCounts(2);
                  
                  this.codeName = VisionMarkerTrained.ProbeTree{1}.labels{this.codeID};
                  
                else
                  [this.codeName, this.codeID] = TestTree( ...
                    VisionMarkerTrained.ProbeTree, img, tform, threshold, ...
                    VisionMarkerTrained.ProbePattern);
                end
                
                if length(this.codeID) > 1
                  warning('Multiclass tree returned multiple labels. Choosing most frequent.');
                  this.codeID = mode(this.codeID);
                  this.codeName = VisionMarkerTrained.ProbeTree.labels{this.codeID};
                end
                                
                if any(strcmp(this.codeName, {'UNKNOWN', 'INVALID'}))
                    this.isValid = false;
                else
                    if VerifyLabel
                        if UseSingleProbe
                            oneProbe.x = 0;
                            oneProbe.y = 0;
                            [verificationResult, verifiedID] = TestTree( ...
                                VisionMarkerTrained.ProbeTree.verifiers(this.codeID), ...
                                img, tform, threshold, oneProbe);

                            this.isValid = verifiedID ~= 1;
                        else
                            if all(isfield(VisionMarkerTrained.ProbeTree, ...
                                    {'verifyTreeRed', 'verifyTreeBlack'}))

                                verificationResult = this.codeName;
                                this.isValid = false;

                                [redResult, redLabelID] = TestTree( ...
                                    VisionMarkerTrained.ProbeTree.verifyTreeRed, ...
                                    img, tform, threshold, VisionMarkerTrained.ProbePattern);

                                if any(this.codeID == redLabelID)
                                    assert(any(strcmp(this.codeName, redResult)));

                                    [blackResult, blackLabelID] = TestTree(...
                                        VisionMarkerTrained.ProbeTree.verifyTreeBlack, ...
                                        img, tform, threshold, VisionMarkerTrained.ProbePattern);

                                    if any(this.codeID == blackLabelID)
                                        assert(any(strcmp(this.codeName, blackResult)));

                                        this.isValid = true;
                                    end
                                end

                            elseif isfield(VisionMarkerTrained.ProbeTree, 'verifiers')
                                [verificationResult, verifiedID] = TestTree( ...
                                    VisionMarkerTrained.ProbeTree.verifiers(this.codeID), ...
                                    img, tform, threshold, VisionMarkerTrained.ProbePattern);

                                this.isValid = verifiedID ~= 1;
                                
                            else 
                              this.isValid = true;
                              verificationResult = this.codeName;
                              verifiedID = this.codeID;
                            end
                        end
                    else % if VerifyLabel
                        verificationResult = this.codeName;
                        verifiedID = this.codeID;
                    end
                end

                if this.isValid
                    assert(strcmp(verificationResult, this.codeName));

                    this = this.ReorderCorners();
                end
            end % IF threshold < 0
            
            this.pose = Pose;
            this.fiducialSize = Size;
            this.name = this.codeName;
            
        end % Constructor VisionMarkerTrained()
       
        % TODO: why was this line here?
%         h = Draw(this, varargin);
        
        function this = ReorderCorners(this, varargin)
            if ~iscell(this.codeName)
                this.codeName = {this.codeName};
            end
            
            for iName = 1:length(this.codeName)
                underscoreIndex = find(this.codeName{iName} == '_');
                if strncmpi(this.codeName{iName}, 'inverted_', 9)
                    % Ignore the first underscore found if this is
                    % an inverted code name
                    underscoreIndex(1) = [];
                end

                if ~isempty(underscoreIndex)
                    assert(length(underscoreIndex) == 1, ...
                        'There should be no more than 1 underscore in the code name: "%s".', this.codeName{iName});

                    angleStr = this.codeName{iName}((underscoreIndex+1):end);
                    this.codeName{iName} = this.codeName{iName}(1:(underscoreIndex-1));

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

                end % if ~isempty(underscoreIndex)

                % Use code names that match what gets put into
                % the C++ enums by auto code generation after
                % training
                this.codeName{iName} = sprintf('MARKER_%s', upper(this.codeName{iName}));
            end
            
            if length(this.codeName) == 1
                this.codeName = this.codeName{1};
            end
            
            this.name = this.codeName;
        end
        
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
