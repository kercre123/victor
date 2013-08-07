classdef BlockWorld < handle
    
    properties(GetAccess = 'public', Constant = true)
        %HasMat    = true; % use mat for robot localization
        MaxBlocks = 256;  % 8 bits
        MaxFaces  = 16;   % 4 bits
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        blocks = {};
        blockTypeToIndex;
        
        allMarkers3D = {};
        robots = {}; 
        
        hasMat = true;
        
        embeddedConversions;
        
    end % PROPERTIES (get-public, set-protected)
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        numMarkers;
        numBlocks;
        numRobots;
        
    end % DEPENDENT PROPERTIES (get-public, set-protected)
    
    
    methods(Access = 'public')
        
        function this = BlockWorld(varargin)
            
            CameraType = 'usb'; % 'usb' or 'webot'
            CameraDevice = {};
            CameraCalibration = {};
            MatCameraDevice = {};
            MatCameraCalibration = {};
            HasMat = true;
            EmbeddedConversions = EmbeddedConversionsManager( ...
                'homographyEstimationType', 'matlab_cp2tform'); 
            
            parseVarargin(varargin{:});
            
            this.hasMat = HasMat;
            this.blocks = {};
            this.allMarkers3D = {};
            
            this.embeddedConversions = EmbeddedConversions;
            
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
                this.robots{i} = Robot('World', this, ...
                    'CameraType', CameraType, ...
                    'CameraCalibration', CameraCalibration{i}, ...
                    'CameraDevice', CameraDevice{i}, ...
                    'MatCameraCalibration', MatCameraCalibration{i}, ...
                    'MatCameraDevice', MatCameraDevice{i});
            end
            
        end % FUNCTION BlockWorld()
        
        function index = getBlockTypeIndex(this, blockType)
            index = this.blockTypeToIndex(blockType);
        end
        
        function B = getBlock(this, blockType)
            if isKey(this.blockTypeToIndex, blockType)
                B = this.blocks{this.blockTypeToIndex(blockType)};
            else
                B = [];
            end
        end
        
    end % METHODS (public)
    
    methods(Static = true, Access = 'protected')
        
        markerPose = blockPoseHelper(robot, B, markers2D);
        
    end
    
    methods % Dependent SET/GET
        
        function N = get.numMarkers(this)
            N = length(this.allMarkers3D);
        end
        
        function N = get.numBlocks(this)
            N = length(this.blocks);
        end
        
        function N = get.numRobots(this)
            N = length(this.robots);
        end
        
    end % METHODS (dependent)
    
end % CLASSDEF BlockWorld