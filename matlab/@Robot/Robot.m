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
        
        % for auto-docking
        dockingBlock = [];
        dockingFace  = [];
        virtualBlock = []; 
        virtualFace  = [];
        
        camPoseCov;
                        
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
        operationMode;
        liftPosition;
        
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        currentObservation;
        stateVector;
    end
    
    properties(GetAccess = 'public', SetAccess = 'public')
        
        observationWindow = {};
                
        visibleBlocks;
        visibleFaces;
        
    end % PROPERTIES (get-public, set-protected)
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        pose_;
        posePrev_;
        
        operationMode_ = '';
        liftPosition_ = '';
        
        getHeadPitchFcn; % query current head pitch angle
        setHeadPitchFcn;
        
        driveFcn; 
        setLiftPositionFcn;
        isBlockLockedFcn;
        releaseBlockFcn;
        
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
            SetHeadPitchFcn = [];
            SetLiftPositionFcn = [];
            DriveFcn = [];
            IsBlockLockedFcn = [];
            ReleaseBlockFcn = [];
            
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
                    'GetHeadPitchFcn should be a function handle.');
                this.getHeadPitchFcn = GetHeadPitchFcn;
            end
            
            if isempty(SetHeadPitchFcn)
                this.setHeadPitchFcn = @(angle)0;
            else
                assert(isa(SetHeadPitchFcn, 'function_handle'), ...
                    'SetHeadPitchFcn should be a function handle.');
                this.setHeadPitchFcn = SetHeadPitchFcn;
            end
            
            if isempty(SetLiftPositionFcn)
                this.setLiftPositionFcn = @(pos)0;
            else
                assert(isa(SetLiftPositionFcn, 'function_handle'), ...
                    'SetLiftPositionFcn should be a function handle.');
                this.setLiftPositionFcn = SetLiftPositionFcn;
            end
            
            if isempty(DriveFcn)
                this.driveFcn = @(left,right) 0;
            else
                assert(isa(DriveFcn, 'function_handle'), ...
                    'DriveFcn should be a function handle.');
                this.driveFcn = DriveFcn;
            end
            
            if isempty(IsBlockLockedFcn)
                this.isBlockLockedFcn = @()false;
            else
                assert(isa(IsBlockLockedFcn, 'function_handle'), ...
                    'IsBlockLockedFcn should be a function handle.');
                this.isBlockLockedFcn = IsBlockLockedFcn;
            end
                
            if isempty(ReleaseBlockFcn)
                this.releaseBlockFcn = @()0;
            else
                assert(isa(ReleaseBlockFcn, 'function_handle'), ...
                    'ReleaseBlockFcn should be a function handle.');
                this.releaseBlockFcn = ReleaseBlockFcn;
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
        
        function drive(this, leftWheelVelocity, rightWheelVelocity)
            this.driveFcn(leftWheelVelocity, rightWheelVelocity);
        end
        
        function tiltHead(this, angleInc)
            this.setHeadPitchFcn(this.getHeadPitchFcn() + angleInc);
            this.updateHeadPose();
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

        function locked = isBlockLocked(this)
            locked = this.isBlockLockedFcn();
        end
        
        function releaseBlock(this)
            this.releaseBlockFcn();
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
            
            % Store previous pose for use in computing angular and
            % positional velocity for controllers:
            % TODO: do I need to copy in the parent/name info?
            this.posePrev_ = Pose(this.pose_.Rvec, this.pose_.T, this.pose_.sigma);
            this.pose_.update(P.Rmat, P.T, P.sigma);
        end
        
        function set.operationMode(this, mode)
            % If we are turning off docking mode, reset stuff
            if strcmp(this.operationMode_, 'DOCK') && ...
                    ~strcmp(mode, 'DOCK')
                
                this.dockingBlock = [];
                this.dockingFace  = [];
                this.virtualBlock = [];
                this.virtualFace  = [];
            end
            this.operationMode_ = mode;
        end
        
        function mode = get.operationMode(this)
            mode = this.operationMode_;
        end
        
        function set.liftPosition(this, position)
            this.liftPosition_ = position;
            this.setLiftPositionFcn(position);
        end
               
        function position = get.liftPosition(this)
            position = this.liftPosition_;
        end
        
        function vec = get.stateVector(this)
            assert(Pose.isRootPose(this.pose_.parent), ...
                'Robot''s pose should be w.r.t. World coordinates to compute state vector.');
            %
            % State is currently defined as:
            %  v = [xPosition yPosition headingAngle xVelocity yVelocity angularVelocity]'
            %
            % Note timestep (i.e. for xVelocity = dx/dt) is exactly 1.  
            % TODO: incorporate timestamps for poses so we can compute dt?
            %
            dt = 1;
            
            % Assuming we are on a simple XY plane and all rotation is around
            % the Z axis
            assert(abs(dot(this.pose_.axis, [0 0 1])) > 0.99, ...
                'Expecting Robot to be on a plane with all rotation around the Z axis.');
            
            if isempty(this.posePrev_)
                % We haven't taken a step yet to measure our velocity
                velocityVec = [0 0 0]';
            else
                angularVelocity = this.pose_.angle - this.posePrev_.angle;
                if this.pose_.angle > this.posePrev_.angle
                    if angularVelocity > pi
                        angularVelocity = 2*pi - angularVelocity;
                    end
                elseif angularVelocity < pi
                    angularVelocity = 2*pi + angularVelocity;
                end
                
                velocityVec = [this.pose_.T(1:2)-this.posePrev_.T(1:2); ...
                    angularVelocity];
            end
            
            vec = [this.pose_.T(1:2); this.pose_.angle; velocityVec/dt];
        end
    end
    
end % CLASSDEF Robot