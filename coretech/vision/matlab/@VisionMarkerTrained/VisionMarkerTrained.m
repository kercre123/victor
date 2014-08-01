classdef VisionMarkerTrained
        
    properties(Constant = true)
        
        %TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/lettersWithFiducials/rotated';
        %TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/symbolsWithFiducials/unpadded/rotated';
        
        TrainingImageDir = { ...
            '~/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/rotated', ... '~/Box Sync/Cozmo SE/VisionMarkers/matWithFiducials/unpadded/rotated', ...
            '~/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/rotated', ...
            '~/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/rotated', ...}; %, ...
            '~/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/rotated'};
                
        ProbeParameters = struct( ...
            'GridSize', 32, ...            %'Radius', 0.02, ...  % As a fraction of a canonical unit square 
            'NumAngles', 8, ...       % How many samples around ring to sample
            'Method', 'mean');        % How to combine points in a probe
                
        MinContrastRatio = 1.25;  % bright/dark has to be at least this
        
        SquareWidthFraction = 0.1; % as a fraction of the fiducial width
        FiducialPaddingFraction = 0.1; % as a fraction of the fiducial width
        
        ProbeRegion = [VisionMarkerTrained.SquareWidthFraction+VisionMarkerTrained.FiducialPaddingFraction ...
            1-(VisionMarkerTrained.SquareWidthFraction+VisionMarkerTrained.FiducialPaddingFraction)];

        ProbePattern = VisionMarkerTrained.CreateProbePattern();
        
        ProbeTree = VisionMarkerTrained.LoadProbeTree();
        
        AllMarkerImages = VisionMarkerTrained.LoadAllMarkerImages(VisionMarkerTrained.TrainingImageDir);
        
        DarkProbes   = VisionMarkerTrained.CreateProbes('dark'); 
        BrightProbes = VisionMarkerTrained.CreateProbes('bright');
        
        % X-Z Plane: 
        Canonical3dCorners = [-1 0 1; -1 0 -1; 1 0 1; 1 0 -1];
        % X-Y Plane: Canonical3dCorners = [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
        
    end % Constant Properties
    
    methods(Static = true)
        
        [img, alpha] = AddFiducial(img, varargin);
        AddFiducialBatch(inputDir, outputDir, varargin);
        probeTree = TrainProbeTree(varargin);
        [squareWidth_pix, padding_pix] = GetFiducialPixelSize(imageSize, imageSizeType);
        %Corners = GetMarkerCorners(imageSize, isPadded);
        corners = GetFiducialCorners(imageSize, isPadded);
        [threshold, bright, dark] = ComputeThreshold(img, tform);
        outputString = GenerateHeaderFiles(varargin);
        [numMulticlassNodes, numVerificationNodes] = GetNumTreeNodes();

        CreateTestImage(varargin);
        CreatePrintableCodeSheet(varargin);
        
    end % Static Methods
    
    methods(Static = true, Access = 'protected')
        
        pattern = CreateProbePattern();
        
        tree = LoadProbeTree();
        
        markerImages = LoadAllMarkerImages(TrainingImageDir);
        
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
        
        matchDistance;
        
    end % Properties
                
    methods 

        function this = VisionMarkerTrained(img, varargin)
            
            Corners = [];
            Pose = [];
            Size = 1;
            UseSortedCorners = false;
            UseSingleProbe = false;
            CornerRefinementIterations = 25;
            UseMexCornerRefinment = false;
            exhaustiveSearchMethod = {0,''};
            exhaustiveSearchThreshold = 0.1;
                        
            parseVarargin(varargin{:});
            
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
            
            if UseSortedCorners
                centerX = mean(Corners(:,1));
                centerY = mean(Corners(:,2));

                [thetas,~] = cart2pol(Corners(:,1)-centerX, Corners(:,2)-centerY);
                [~,sortedIndexes] = sort(thetas);
                
                sortedCorners = Corners(sortedIndexes,:);
                
                try
                    tform = cp2tform([0 0 1 1; 0 1 1 0]', sortedCorners, 'projective');
                catch
                    disp('Some points are co-linear');
                    this.isValid = false;
                    this.H = eye(3);
                    return;
                end
            else
                try
                    tform = cp2tform([0 0 1 1; 0 1 0 1]', Corners, 'projective');
                catch                    
                    disp('Some points are co-linear');
                    this.isValid = false;
                    this.H = eye(3);
                    return;
                end
            end            
            
            this.H = tform.tdata.T';
    
            [threshold, bright, dark] = VisionMarkerTrained.ComputeThreshold(img, tform);
            
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
                
                if exhaustiveSearchMethod{1} ~= 0                    
                    if exhaustiveSearchMethod{1} == 1
                        [this.codeName, this.codeID, this.matchDistance] = TestExhaustiveSearch( ...
                            VisionMarkerTrained.AllMarkerImages, img, this.corners, exhaustiveSearchMethod{2});
                    else
                        persistent numDatabaseImages databaseImageHeight databaseImageWidth databaseImages databaseLabelIndexes; %#ok<TLEV>
                        
                        if isempty(numDatabaseImages)
                            % TODO: get paths from the right place
                            patterns = {'Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/*.png', 'Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/*.png', 'Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/*.png', 'Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/*.png'};
                            
                            if ~ispc()
                                for i = 1:length(patterns)
                                    patterns{i} = strrep('Z:/', '~/');
                                end
                            end
                            
                            imageFilenames = {};
                            for iPattern = 1:length(patterns)
                                files = dir(patterns{iPattern});
                                for iFile = 1:length(files)
                                    imageFilenames{end+1} = [strrep(patterns{iPattern}, '*.png', ''), files(iFile).name]; %#ok<AGROW>
                                end
                            end
                            [numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes] = mexLoadExhaustiveMatchDatabase(imageFilenames);
                        end
                        
                        [markerName, orientation, this.matchDistance] = mexExhaustiveMatchFiducialMarker(uint8(img*255), this.corners, numDatabaseImages, databaseImageHeight, databaseImageWidth, databaseImages, databaseLabelIndexes);
                        
                        if orientation == 0
                            this.codeName = [markerName(8:end), '_000'];
                        elseif orientation == 90
                            this.codeName = [markerName(8:end), '_090'];
                        elseif orientation == 180
                            this.codeName = [markerName(8:end), '_180'];
                        elseif orientation == 270
                            this.codeName = [markerName(8:end), '_270'];
                        end
                            
                        this.codeID = []; % TODO: must this be set?
                    end
                    
                    verificationResult = this.codeName;
                    
                    if this.matchDistance < exhaustiveSearchThreshold
                        this.isValid = true;
                    else 
                        this.isValid = false;
                    end
                else
                    [this.codeName, this.codeID] = TestTree( ...
                        VisionMarkerTrained.ProbeTree, img, tform, threshold, ...
                        VisionMarkerTrained.ProbePattern);
                end
                
                if any(strcmp(this.codeName, {'UNKNOWN', 'INVALID'}))
                    this.isValid = false;
                else
                    if exhaustiveSearchMethod{1} == 0
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

                            else
                                assert(isfield(VisionMarkerTrained.ProbeTree, 'verifiers'));

                                [verificationResult, verifiedID] = TestTree( ...
                                    VisionMarkerTrained.ProbeTree.verifiers(this.codeID), ...
                                    img, tform, threshold, VisionMarkerTrained.ProbePattern);

                                this.isValid = verifiedID ~= 1;
                            end
                        end
                    end % if ~UseExhaustiveSearch ... else                    
                    
                    if this.isValid
                        assert(strcmp(verificationResult, this.codeName));
                        
                        underscoreIndex = find(this.codeName == '_');
                        if strncmpi(this.codeName, 'inverted_', 9)
                            % Ignore the first underscore found if this is
                            % an inverted code name
                            underscoreIndex(1) = [];
                        end
                        
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
                            
                        end % if ~isempty(underscoreIndex)
                        
                        % Use code names that match what gets put into
                        % the C++ enums by auto code generation after
                        % training
                        this.codeName = sprintf('MARKER_%s', upper(this.codeName));
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

        [probeValues, X, Y] = GetProbeValues(this, img);
        
    end % Public Methods
    
end % CLASSDEF TrainedVisionMarker
