classdef VisionMarkerTrained
        
    properties(Constant = true)
        
        %TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/lettersWithFiducials/rotated';
        %TrainingImageDir = '~/Box Sync/Cozmo SE/VisionMarkers/symbolsWithFiducials/unpadded/rotated';
        
        RootImageDir = '~/Dropbox (Anki, Inc)/VisionMarkers - Final';
        CozmoImageDir = '/Cozmo/MassProduction/trainingImages';
        VictorImageDir = '/Victor/DVT2/trainingImages_white';
        
        TrainingImageDir = { ...
            %fullfile(VisionMarkerTrained.RootImageDir, 'letters/withFiducials/rotated'), ... '~/Box Sync/Cozmo SE/VisionMarkers/matWithFiducials/unpadded/rotated', ...
            %fullfile(VisionMarkerTrained.RootImageDir, 'lightCubeH/withFiducials/rotated'), ... fullfile(VisionMarkerTrained.RootImageDir, 'matGears/withFiducials/rotated'), ... 
            %fullfile(VisionMarkerTrained.RootImageDir, 'dice/withFiducials/rotated'), ...}; % '~/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/rotated', ...
            %fullfile(VisionMarkerTrained.RootImageDir, 'creepTest/withFiducials/rotated'), ...
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.CozmoImageDir, 'symbols_basic/withFiducials/rotated'), ...
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.CozmoImageDir, 'customSDK/withFiducials/rotated'), ...
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.CozmoImageDir, 'lightCubeI/withFiducials/rotated'), ...
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.CozmoImageDir, 'lightCubeJ/withFiducials/rotated'), ...
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.CozmoImageDir, 'lightCubeK/withFiducials/rotated'), ...
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.CozmoImageDir, 'charger/stretched/withFiducials/rotated'), ...
            '~/Dropbox (Anki, Inc)/VisionMarkers - Final/Victor/Preproduction/trainingImages/charger/withFiducials/rotated', ... % TODO: remove with VIC-945
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.VictorImageDir, 'charger/rotated'), ...
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.VictorImageDir, 'lightCubeCircle/rotated'), ...
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.VictorImageDir, 'lightCubeK_lightOnDark/rotated'), ...
            fullfile(VisionMarkerTrained.RootImageDir, VisionMarkerTrained.VictorImageDir, 'lightCubeSquare/rotated')};
                
        ProbeParameters = struct( ...
            'GridSize', 32, ...       %'Radius', 0.02, ...  % As a fraction of a canonical unit square 
            'NumAngles', 4, ...       % How many samples around ring to sample
            'Method', 'mean');        % How to combine points in a probe
                
        MinContrastRatio = 1.05; %1.25;  % bright/dark has to be at least this
        
        SquareWidthFraction = 0.1;     % as a fraction of the fiducial width
        FiducialPaddingFraction = 0.1; % as a fraction of the fiducial width
        CornerRadiusFraction = 0;    % as a fraction of the fiducial width
        
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
        
        [squareWidth_pix, padding_pix, cornerRadius_pix] = GetFiducialPixelSize(imageSize, imageSizeType);
        %Corners = GetMarkerCorners(imageSize, isPadded);
        corners = GetFiducialCorners(imageSize, isPadded);
        [threshold, bright, dark] = ComputeThreshold(img, tform, method);
        outputString = GenerateHeaderFiles(varargin);
        [nearestNeighborString, markerDefString] = GenerateNearestNeighborHeaderFiles(varargin);
        [probeDefString] = GenerateProbeDefinitionFiles(varargin);
        
        [numMulticlassNodes, numVerificationNodes] = GetNumTreeNodes(tree);

        CreateTestImage(varargin);
        CreatePrintableCodeSheet(varargin);

        [xgrid,ygrid] = GetProbeGrid(workingResolution);
        
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
        
        nearestNeighborLibrary;
        
    end % Properties
                
    methods 

        function this = VisionMarkerTrained(img, varargin)
            
            Corners = [];
            Pose = [];
            Size = 1;
            UseSingleProbe = false;
            CornerRefinementIterations = 100;
            UseMexCornerRefinment = true;
            VerifyLabel = true;
            Initialize = true;
            ThresholdMethod = 'FiducialProbes'; % 'Otsu' or 'FiducialProbes'
            NearestNeighborLibrary = [];
            CNN = [];
            
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
                      try
                        [this.corners, this.H] = mexRefineQuadrilateral(im2uint8(img), int16(this.corners-1), single(this.H), ...
                            CornerRefinementIterations, VisionMarkerTrained.SquareWidthFraction, dark*255, bright*255, 100, 5, .005);
                        this.corners = this.corners + 1;
                      catch E
                        warning('mexRefineQuadrilateral failed: %s', E.message);
                        this.isValid = false;
                      end
                    else
                        [this.corners, this.H] = this.RefineCorners(img, 'NumSamples', 100, ... 'DebugDisplay', true,  ...
                            'MaxIterations', CornerRefinementIterations, ...
                            'DarkValue', dark, 'BrightValue', bright);
                    end
                end
                
                if ~isempty(CNN)
                  assert(all(isfield(CNN, {'labels', 'labelNames'})), ...
                    'Expecting CNN to have "labels" and "labelNames" fields.');
                  
                  probeValues = VisionMarkerTrained.GetProbeValues(img, tform);
                  res = vl_simplenn(CNN, single(probeValues));
                  %[maxResponse,index] = max(squeeze(sum(sum(res(end).x,1),2)));
                  [sortedResponses, index] = sort(squeeze(sum(sum(res(end).x,1),2)), 'descend');
                  
                  % Max response has to be significantly better than
                  % second-best response for this to be a valid marker
                  responseRatio = sortedResponses(1)/sortedResponses(2);
                  %fprintf('responseRatio = %f\n', responseRatio);
                  
                  if responseRatio > 1.2
                    this.codeID = CNN.labels(index(1));
                    this.codeName = CNN.labelNames(this.codeID);
                    this.isValid  = true;
                  else
                    this.codeID = 0;
                    this.codeName = 'UNKNOWN';
                    this.isValid = false;
                  end
                  
                elseif ~isempty(NearestNeighborLibrary)
                  VerifyLabel = false;
                  assert(size(NearestNeighborLibrary.probeValues,1) == VisionMarkerTrained.ProbeParameters.GridSize^2, ...
                    'NearestNeighborLibrary should be from a %dx%d probe pattern.', ...
                    VisionMarkerTrained.ProbeParameters.GridSize, ...
                    VisionMarkerTrained.ProbeParameters.GridSize);
                  
                  % Compute distance of the observed probes to all known
                  % probes.
                  
                 probeValues = VisionMarkerTrained.GetProbeValues(img, tform);
                 %probeValues = (probeValues - dark)/(bright-dark);
                 
                 %probeValues = 255*imfilter(probeValues, [0 1 0; 1 -4 1; 0 1 0], 'replicate');
                 
                 %probeValues = probeValues - imfilter(probeValues, ones(16)/256, 'replicate');
                 kernel = -ones(16);
                 kernel(8,8) = 255;
                 probeValues = imfilter(probeValues, kernel, 'replicate');
                 minVal = min(probeValues(:));
                 maxVal = max(probeValues(:));
                 probeValues = (probeValues - minVal)/(maxVal-minVal);
                 
                 probeValues = im2uint8(probeValues(:));
                 
%                  if isfield(NearestNeighborLibrary, 'gradMagWeights')
%                    d = imabsdiff(probeValues(:,ones(1,size(NearestNeighborLibrary.probeValues,2))), NearestNeighborLibrary.probeValues);
%                    d = sum(NearestNeighborLibrary.gradMagWeights.*single(d),1) ./ sum(NearestNeighborLibrary.gradMagWeights,1);
%                  elseif isfield(NearestNeighborLibrary, 'gradMagValues')
%                    %Ix = abs(image_right(probeValues) - image_left(probeValues));
%                    %Iy = abs(image_down(probeValues) - image_up(probeValues));
%                    %probeWeights = max(0,min(1,single(column(1 - max(Ix,Iy)))));
%                    
%                    libraryWeights = im2single(NearestNeighborLibrary.gradMagValues);
%                    minVal = min(libraryWeights(:));
%                    maxVal = max(libraryWeights(:));
%                    w = 1 - (libraryWeights-minVal)/(maxVal-minVal);
%                    
%                    d = imabsdiff(probeValues(:,ones(1,size(NearestNeighborLibrary.probeValues,2))), NearestNeighborLibrary.probeValues);
%                    %w = min(probeWeights(:,ones(1,size(NearestNeighborLibrary.probeValues,2))), libraryWeights);
%                    d = sum(w.*single(d),1) ./ sum(w,1);
%                  else
                   d = mean(imabsdiff(probeValues(:,ones(1,size(NearestNeighborLibrary.probeValues,2))), NearestNeighborLibrary.probeValues),1);
%                  end
                 [minDist, index] = min(d);
                 %fprintf('Min NN dist = %f\n', minDist);
                 
                 this.codeID = NearestNeighborLibrary.labels(index);
                 this.codeName = NearestNeighborLibrary.labelNames{this.codeID};
                 this.isValid = minDist < 30;
                 
                elseif iscell(VisionMarkerTrained.ProbeTree)
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
                    if length(underscoreIndex) > 1
                      % If more than one underscore, use last
                      underscoreIndex = underscoreIndex(end);
                    end
                    
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
