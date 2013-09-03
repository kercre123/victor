classdef Robot < handle
    
    properties(GetAccess = 'public', Constant = true)
        ObservationWindowLength = 3;
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        name;
        world; % handle to the world this robot lives in
        
        modelX;
        modelY;
        modelZ;
                
        camera;
        matCamera;
        
        embeddedConversions;
        
        % The neck is a fixed link on which the head (with camera) pivots.
        neckPose; 
        
        % Keep track of untilted camera pose, since we will adjust from the
        % untilted position to the current head pitch at each time step
        % (and not incrementally from the current position)
        R_headCam;
        T_headCam;
        
        camPoseCov;
        getHeadPitchFcn; % query current head pitch angle
        
        % For drawing:
        appearance = struct( ...
            'BodyColor', [.7 .7 0], ...
            'BodyHeight', 30, ...
            'BodyWidth', 60, ...
            'BodyLength', 75, ...
            'WheelWidth', 15, ...
            'WheelRadius', 15, ...
            'WheelColor', 0.25*ones(1,3), ...
            'EyeLength', [], ...
            'EyeRadius', [], ...
            'EyeLengthFraction', .65, ... % fraction of BodyLength
            'EyeRadiusFraction', .47, ... % fraction of BodyWidth/2
            'EyeColor', [.5 0 0], ...
            'PathHistoryColor', 'k', ...
            'PathHistoryLength', 500);
        
        handles; 
                
    end % PROPERTIES (get-public, set-protected)
    
    properties(GetAccess = 'public', SetAccess = 'public', ...
            Dependent = true)
        
        pose;
        origin;
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        currentObservation;
    end
    
    properties(GetAccess = 'public', SetAccess = 'public')
        
        observationWindow = {};
        
    end % PROPERTIES (get-public, set-protected)
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        pose_;
    end
    
    methods(Access = 'public')
        
        function this = Robot(varargin)
            Name = 'Robot';
            World = [];
            CameraType = 'usb'; % 'usb' or 'webot'
            CameraDevice = [];
            CameraCalibration = [];
            MatCameraDevice = [];
            MatCameraCalibration = [];
            GetHeadPitchFcn = [];
            
            appearanceArgs = parseVarargin(varargin);
            
            this.name = Name;
            this.world = World;
            
            this.observationWindow = cell(1, this.ObservationWindowLength);
            
            this.appearance = parseVarargin(this.appearance, appearanceArgs{:});
            
            this.appearance.EyeLength = this.appearance.EyeLengthFraction * ...
                this.appearance.BodyLength;
            this.appearance.EyeRadius = this.appearance.EyeRadiusFraction * ...
                this.appearance.BodyWidth/2;
            
            this.pose_ = Pose();
            this.pose_.name = 'fromRobot_toWorld';
            
            % From camera to robot pose:
            % Rotation -90 degrees around x axis:
            this.R_headCam = [1 0 0; 0 0 1; 0 -1 0];
            this.T_headCam = [0; 30; 23];
            
            % Add uncertainty for Camera pose(s) w.r.t. robot?
            %   1 degree uncertainty in rotation
            %   .1mm uncertainty in translation
            Rcov = pi/180*eye(3);
            Tcov = (.1)^2*eye(3);
            this.camPoseCov = blkdiag(Rcov,Tcov);
            
            this.neckPose = Pose([0 0 0], [0; 0; 20]);
            this.neckPose.parent = this.pose;
            this.neckPose.name = 'fromNeck_toRobot';
            
            headPose = Pose(this.R_headCam, this.T_headCam, this.camPoseCov);
            headPose.parent = this.neckPose;
            headPose.name = 'fromHeadCam_toNeck';
            
            if isempty(GetHeadPitchFcn)    
                this.getHeadPitchFcn = @()0;
            else
                assert(isa(GetHeadPitchFcn, 'function_handle'), ...
                    'GetHeadCameraPoseFcn should be a function handle.');
                this.getHeadPitchFcn = GetHeadPitchFcn;
            end
            
            this.camera = Camera('device', CameraDevice, ...
                'deviceType', CameraType, ...
                'calibration', CameraCalibration, ...
                'pose', headPose);
            
            if (isempty(this.world) && ~isempty(MatCameraDevice)) ...
                    || (~isempty(this.world) && this.world.hasMat)
                
                % From robot to camera frame:
                Rrc = [1 0 0; 0 -1 0; 0 0 -1]; % This seems wrong, shouldn't it be negating y and z?
                Trc = [0; 0; 3]; % Based on Webot down-camera position
                this.matCamera = Camera('device', MatCameraDevice, ...
                    'deviceType', CameraType, ...
                    'calibration', MatCameraCalibration, ...
                    'pose', inv(Pose(Rrc, Trc, this.camPoseCov)));
            end
        end
        
        function updateHeadPose(this)
            % TODO: return error too and adjust covariance
           
            angle = this.getHeadPitchFcn();
            
            % Get rotation matrix for the current head pitch:
            Rpitch = rodrigues(angle*[1 0 0]); 
            
            % Rotate around the head's anchor point:
            this.camera.pose.update(Rpitch*this.R_headCam, ...
                Rpitch*this.T_headCam);
        end


    end % METHODS (public)
    
    methods
        
        function obs = get.currentObservation(this)
            obs = this.observationWindow{1};
        end
        
        function P = get.pose(this)
            % Asking for a robot's pose gives you the pose of the 
            % current observation:
            P = this.pose_;
        end
                
        function o = get.origin(this)
            o = this.pose.T;
        end
        
        function set.pose(this, P)
            assert(isa(P, 'Pose'), 'Must provide a Pose object.');
            this.pose_.update(P.Rmat, P.T, P.sigma);
        end
        
    end
    
end % CLASSDEF Robot