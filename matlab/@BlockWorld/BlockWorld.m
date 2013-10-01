classdef BlockWorld < handle
    
    properties(GetAccess = 'public', Constant = true)
        MaxBlockTypes = 256;  % 8 bits
        MaxFaces      = 16;   % 4 bits
        
        SelectedBlockColor = [0 1 1];
        UnselectedBlockColor = [1 1 0];
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        blocks = {};
        blockTypeToIndex;
        groundTruthBlocks = {};
                        
        allMarkers3D = {};
        robots = {}; 
        groundTruthRobots = {};
        
        hasMat = true;
        matSize;
        zDirection;
        
        embeddedConversions;
        
        groundTruthPoseFcn;
        updateObsBlockPoseFcn;
        updateObsRobotPoseFcn;
        getHeadPitchFcn;
        setHeadPitchFcn;
        getLiftAngleFcn;
        setLiftAngleFcn;
        setLiftPositionFcn;
        driveRobotFcn;
        isBlockLockedFcn;
        releaseBlockFcn;
        displayMessageFcn;
                
    end % PROPERTIES (get-public, set-protected)
        
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        numMarkers;
        numBlocks;
        numRobots;
        
    end % DEPENDENT PROPERTIES (get-public, set-protected)
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        selectedBlock_ = [];
        
    end % PROPERTIES (get/set protected)
    
    properties(GetAccess = 'public', SetAccess = 'public', ...
            Dependent = true)
        
        selectedBlock;
        
    end % DEPENDENT PROPERTIES (get/set public)
    
    methods(Access = 'public')
        
        function this = BlockWorld(varargin)
            
            CameraType = 'usb'; % 'usb' or 'webot'
            CameraDevice = {};
            CameraCalibration = {};
            MatCameraDevice = {};
            MatCameraCalibration = {};
            HasMat = true;
            MatSize = [1000 1000]; % in mm
            ZDirection = 'up';
            EmbeddedConversions = EmbeddedConversionsManager(); 
            GroundTruthPoseFcn = [];
            UpdateObservedRobotPoseFcn = [];
            UpdateObservedBlockPoseFcn = [];
            GetHeadPitchFcn = [];
            SetHeadPitchFcn = [];
            GetLiftAngleFcn = [];
            SetLiftAngleFcn = [];
            SetLiftPositionFcn = [];
            DriveFcn = [];
            IsBlockLockedFcn = [];
            ReleaseBlockFcn = [];
            DisplayMessageFcn = @(type,message)fprintf('%s: %s\n', type, message);
            
            parseVarargin(varargin{:});
            
            assert(islogical(HasMat), 'HasMat should be logical.');
            assert(isvector(MatSize) && length(MatSize)==2, ...
                'MatSize should be 2-element vector [xSize ySize] in mm.');
            
            this.hasMat  = HasMat;
            this.matSize = MatSize;
            this.zDirection = ZDirection;
            
            this.blocks = cell(1, BlockWorld.MaxBlockTypes);
            this.allMarkers3D = {};
            
            this.embeddedConversions = EmbeddedConversions;
            
            this.groundTruthPoseFcn = GroundTruthPoseFcn;
            
            this.updateObsBlockPoseFcn = UpdateObservedBlockPoseFcn;
            this.updateObsRobotPoseFcn = UpdateObservedRobotPoseFcn;
            this.getHeadPitchFcn       = GetHeadPitchFcn;
            this.setHeadPitchFcn       = SetHeadPitchFcn;
            this.getLiftAngleFcn       = GetLiftAngleFcn;
            this.setLiftAngleFcn       = SetLiftAngleFcn;
            this.setLiftPositionFcn    = SetLiftPositionFcn;
            this.driveRobotFcn         = DriveFcn;
            this.isBlockLockedFcn      = IsBlockLockedFcn;
            this.releaseBlockFcn       = ReleaseBlockFcn;
            this.displayMessageFcn     = DisplayMessageFcn;
            
            this.blockTypeToIndex = containers.Map('KeyType', 'double', ...
                'ValueType', 'double');
            
            if isempty(CameraCalibration)
                error(['You must provide camera calibration ' ...
                    'information for each robot in the world.']);
            elseif ~iscell(CameraCalibration)
                CameraCalibration = {CameraCalibration};
            end
            
            if isempty(CameraDevice)
                CameraDevice = cell(size(CameraCalibration));
            else 
                if ~iscell(CameraDevice)
                    CameraDevice = {CameraDevice};
                end
                if length(CameraDevice) ~= length(CameraCalibration)
                    error(['You must provide a CameraDevice for each ' ...
                        'CameraCalibration (or none).']);
                end
            end
            
            if this.hasMat
                if isempty(MatCameraDevice)
                    MatCameraDevice = cell(size(MatCameraCalibration));
                else
                    if ~iscell(MatCameraDevice)
                        MatCameraDevice = {MatCameraDevice};
                    end
                    if length(MatCameraDevice) ~= length(MatCameraCalibration)
                        error(['You must provide a MatCameraDevice for each ' ...
                            'MatCameraCalibration (or none).']);
                    end
                end
                
                if isempty(MatCameraCalibration)
                    error(['You must provide mat camera calibration ' ...
                        'information for each robot in the world.']);
                elseif ~iscell(MatCameraCalibration)
                    MatCameraCalibration = {MatCameraCalibration};
                end
                
            else
                MatCameraDevice = cell(size(CameraDevice));
                MatCameraCalibration = cell(size(CameraDevice));
            end
            
            assert(isequal(size(CameraDevice), size(MatCameraDevice)), ...
                'There should be a Camera and MatCamera for each robot.');
            
            this.robots = cell(1, length(CameraCalibration));
            for i=1:this.numRobots
                name = 'Cozmo';
                if i > 1
                    name = sprintf('%s%d', name, i);
                end
                
                this.robots{i} = Robot('World', this, 'Name', name, ...
                    'CameraType', CameraType, ...
                    'CameraCalibration', CameraCalibration{i}, ...
                    'CameraDevice', CameraDevice{i}, ...
                    'GetHeadPitchFcn', this.getHeadPitchFcn, ... % TODO: make this per robot?
                    'SetHeadPitchFcn', this.setHeadPitchFcn, ... % TODO: make this per robot?
                    'GetLiftAngleFcn', this.getLiftAngleFcn, ... % TODO: make this per robot?
                    'SetLiftAngleFcn', this.setLiftAngleFcn, ... % TODO: make this per robot?
                    'DriveFcn', this.driveRobotFcn, ... % TODO: make this per robot?
                    'SetLiftPositionFcn', this.setLiftPositionFcn, ... % TODO: make this per robot?
                    'IsBlockLockedFcn', this.isBlockLockedFcn, ... % TODO: make this per robot?
                    'ReleaseBlockFcn', this.releaseBlockFcn, ... % TODO: make this per robot?
                    'MatCameraCalibration', MatCameraCalibration{i}, ...
                    'MatCameraDevice', MatCameraDevice{i});
            end
        
        if ~isempty(this.groundTruthPoseFcn)
            this.groundTruthRobots = cell(1, this.numRobots);
        
            for i=1:this.numRobots
                this.groundTruthRobots{i} = Robot('World', this, ...
                    'CameraCalibration', CameraCalibration{i}, ...
                    'MatCameraCalibration', MatCameraCalibration{i}, ...
                    'PathHistoryColor', 'r');
            end
        end
            
        end % FUNCTION BlockWorld()
        
        function index = getBlockTypeIndex(this, blockType)
            index = this.blockTypeToIndex(blockType);
        end
        
        function P = getGroundTruthRobotPose(this, robotName)
            if isempty(this.groundTruthPoseFcn)
                P = [];
            else
                P = this.groundTruthPoseFcn(robotName);
                
                if strcmp(this.robots{1}.camera.deviceType, 'webot')
                    % The Webot robot is defined rotated 180 degrees (?)
                    P = Pose([-1 0 0; 0 -1 0; 0 0 1]*P.Rmat, P.T);
                end
            end
        end
        
        function P = getGroundTruthBlockPose(this, blockID)
            if isempty(this.groundTruthPoseFcn)
                P = [];
            else
                if isa(blockID, 'Block')
                    blockID = blockID.blockType;
                end
                
                P = this.groundTruthPoseFcn(sprintf('Block%d', blockID));
            end
        end
        
        function updateObsBlockPose(this, blockID, pose, color)
           if ~isempty(this.updateObsBlockPoseFcn)
               if ~Pose.isRootPose(pose)
                   pose = pose.getWithRespectTo('World');
               end
               this.updateObsBlockPoseFcn(blockID, pose, color, 0.0);
           end
        end
        
        function updateObsRobotPose(this, robotName, pose)
            if ~isempty(this.updateObsRobotPoseFcn)
                if ~Pose.isRootPose(pose)
                    pose = pose.getWithRespectTo('World');
                end
                this.updateObsRobotPoseFcn(robotName, pose, 0.6);
            end
        end
        
        function isDocking = getDockingState(this, whichRobot)
            assert(whichRobot > 0 || whichRobot <= length(this.robots), ...
                'Which robot out of range for getDockingState.');
            
            isDocking = strcmp(this.robots{whichRobot}.operationMode, 'DOCK');
        end
        
        function mode = getOperationMode(this, whichRobot)
            assert(whichRobot > 0 || whichRobot <= length(this.robots), ...
                'Which robot out of range for getDockingMode.');
            
            mode = this.robots{whichRobot}.operationMode;
        end
        
        function setOperationMode(this, whichRobot, mode)
            assert(whichRobot > 0 || whichRobot <= length(this.robots), ...
                'Which robot out of range for setDockingMode.');
            
            this.robots{whichRobot}.operationMode = mode;
            
            fprintf('Set operation mode of robot %d, "%s", to %s.\n', ...
                whichRobot, this.robots{whichRobot}.name, ...
                this.robots{whichRobot}.operationMode);
        end
        
        function displayMessage(this, type, message)
            if ~isempty(this.displayMessageFcn)
                this.displayMessageFcn(type, message);
            end
        end
        
    end % METHODS (public)
    
    methods(Static = true, Access = 'public')
        
        markerPose = computeBlockPose(robot, B, markers2D);
        
    end
    
    methods % Dependent SET/GET
        
        function N = get.numMarkers(this)
            N = length(this.allMarkers3D);
        end
        
        function N = get.numBlocks(this)
            N = sum(cellfun(@length, this.blocks));
        end
        
        function N = get.numRobots(this)
            N = length(this.robots);
        end
        
        function block = get.selectedBlock(this)
            block = this.selectedBlock_;
        end
        
        function set.selectedBlock(this, block)
            assert(isempty(block) || isa(block, 'Block'), ...
                'Expecting a Block object for setting selectedBlock.');
            this.selectedBlock_ = block;
        end
        
    end % METHODS (dependent)
    
end % CLASSDEF BlockWorld