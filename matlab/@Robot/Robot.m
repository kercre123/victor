classdef Robot < handle
    
    properties(GetAccess = 'public', Constant = true)
        ObservationWindowLength = 3;
        LiftAngle_Low   = -.3;
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
        
        % The liftBase is a fixed link on which the lift pivotes.
        liftBasePose;
        liftPose;
        
        % Keep track of untilted camera pose, since we will adjust from the
        % untilted position to the current head pitch at each time step
        % (and not incrementally from the current position)
        % (And same for lift)
        R_headCam;
        T_headCam;
        R_lift;
        T_lift;
        
        % for auto-docking
        dockingBlock = [];
        dockingFace  = [];
        virtualBlock = []; 
        virtualFace  = [];
        
        xPreDock = [];
        yPreDock = [];
        orientPreDock = [];
        
        camPoseCov;
                        
        % For drawing:
        appearance = struct( ...
            'BodyColor', [.7 .7 0], ...
            'BodyHeight', 35, ...
            'BodyWidth', 40, ...
            'BodyLength', 90, ...
            'LiftWidth', 30, ...
            'LiftLength', 5, ...
            'LiftHeight', 10, ...
            'WheelWidth', 20, ...
            'WheelRadius', 20, ...
            'AxlePosition', 25, ... % forward/backward from body center
            'WheelColor', 0.25*ones(1,3), ...
            'EyeLength', 40, ...
            'EyeRadius', 15, ...
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
                        
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected', ...
            Dependent = true)
        
        currentObservation;
        stateVector;
        headingAngle;
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
        nextOperationMode = '';
        liftAngle;
        
        LiftAngle_Place;
        LiftAngle_Dock;
        LiftAngle_High;
        
        getHeadPitchFcn; % query current head pitch angle
        setHeadPitchFcn;
        
        getLiftAngleFcn;
        setLiftAngleFcn;
        
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
            GetLiftAngleFcn = [];
            SetLiftAngleFcn = [];
            SetLiftPositionFcn = [];
            DriveFcn = [];
            IsBlockLockedFcn = [];
            ReleaseBlockFcn = [];
            
            appearanceArgs = parseVarargin(varargin);
            
            this.name = Name;
            this.world = World;
            
            this.observationWindow = cell(1, this.ObservationWindowLength);
            
            this.appearance = parseVarargin(this.appearance, appearanceArgs{:});
            
            if isempty(this.appearance.EyeLength)
                this.appearance.EyeLength = this.appearance.EyeLengthFraction * ...
                    this.appearance.BodyLength;
            end
            
            if isempty(this.appearance.EyeRadius)
                this.appearance.EyeRadius = this.appearance.EyeRadiusFraction * ...
                    this.appearance.BodyWidth/2;
            end
            
            this.pose_ = Pose();
            this.pose_.name = 'fromRobot_toWorld';
            
            % From camera to robot pose:
            % Rotation -90 degrees around x axis:
            this.R_headCam = [1 0 0; 0 0 1; 0 -1 0];
            this.T_headCam = [0; 25; 15]; % relative to neck 
            
            % Add uncertainty for Camera pose(s) w.r.t. robot?
            %   1 degree uncertainty in rotation
            %   .1mm uncertainty in translation
            Rcov = pi/180*eye(3);
            Tcov = (.1)^2*eye(3);
            this.camPoseCov = blkdiag(Rcov,Tcov);
            
            % TODO: get the liftBasePose from Webots programmatically
            this.liftBasePose = Pose([0 0 0], [0 -35 27]); % to match webots
            this.liftBasePose.parent = this.pose;
            this.liftBasePose.name = 'fromBase_toRobot';
            
            % TODO: get the liftPose from Webots programmatically
            this.R_lift = eye(3);
            this.T_lift = [0; 97.5; 0]; % 97.5 = 45 + 50 + 2.5
            
            this.liftPose = Pose(this.R_lift, this.T_lift); % TODO add uncertainty
            this.liftPose.parent = this.liftBasePose;
            this.liftPose.name = 'fromLift_toLiftBase';
            
            % TODO: these should probably be computed dynamically based on
            % the Block size(s) we are docking with and/or placing on!
            liftBaseZ = this.liftBasePose.T(3) + this.appearance.WheelRadius;
            this.LiftAngle_Place = asin((90.5-liftBaseZ)/this.T_lift(2));
            this.LiftAngle_Dock  = asin((30-liftBaseZ)/this.T_lift(2));
            this.LiftAngle_High  = asin((100.5-liftBaseZ)/this.T_lift(2));
             
            % TODO: get the neckPose from Webots programmatically
            this.neckPose = Pose([0 0 0], [0; 9.5; 25]);
            this.neckPose.parent = this.pose;
            this.neckPose.name = 'fromNeck_toRobot';
            
            headPose = Pose(this.R_headCam, this.T_headCam, this.camPoseCov);
            headPose.parent = this.neckPose;
            headPose.name = 'fromHeadCam_toNeck';
            
            function setFcnHelper(Fcn, name, default)
                if isempty(Fcn)
                    Fcn = default;
                end
                assert(isa(Fcn, 'function_handle'), ...
                    [name ' should be function handle.']);
                this.(name) = Fcn;
            end
            
            setFcnHelper(GetHeadPitchFcn,    'getHeadPitchFcn',    @()0 );
            setFcnHelper(SetHeadPitchFcn,    'setHeadPitchFcn',    @(angle)0 );
            setFcnHelper(GetLiftAngleFcn,    'getLiftAngleFcn',    @()0 );
            setFcnHelper(SetLiftAngleFcn,    'setLiftAngleFcn',    @(angle)0 );
            setFcnHelper(SetLiftPositionFcn, 'setLiftPositionFcn', @(pos)0 );
            setFcnHelper(DriveFcn,           'driveFcn',           @(left,right)0);
            setFcnHelper(IsBlockLockedFcn,   'isBlockLockedFcn',   @()false);
            setFcnHelper(ReleaseBlockFcn,    'releaseBlockFcn',    @()0);
            
            this.camera = Camera('device', CameraDevice, ...
                'deviceType', CameraType, ...
                'calibration', CameraCalibration, ...
                'pose', headPose);
            
            if (isempty(this.world) && ~isempty(MatCameraDevice)) ...
                    || (~isempty(this.world) && this.world.hasMat)
                
                % From robot to camera frame:
                Rrc = [1 0 0; 0 -1 0; 0 0 -1]; % This seems wrong, shouldn't it be negating y and z?
                Trc = [0; 0; -3]; % Based on Webot down-camera position
                matCamPose = inv(Pose(Rrc, Trc, this.camPoseCov));
                matCamPose.parent = this.pose;
                this.matCamera = Camera('device', MatCameraDevice, ...
                    'deviceType', CameraType, ...
                    'calibration', MatCameraCalibration, ...
                    'pose', matCamPose);
            end
            
            this.liftAngle = this.getLiftAngleFcn();
            
        end % CONSTRUCTOR Robot()
        
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
        
        function adjustLift(this, angleInc)
            this.setLiftAngleFcn(this.getLiftAngleFcn() + angleInc);
            this.updateLiftPose();
        end
        
        function updateLiftPose(this)
            angle = this.getLiftAngleFcn();
            
            Rangle = rodrigues(angle*[1 0 0]);
            
            % Because of the two-bar assembly, the end effector of the lift
            % always faces the same way, even as the whole thing lifts, so
            % the Rotation need not be adjusted here:
            this.liftPose.update(this.R_lift, Rangle*this.T_lift);
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
                
        function angle = get.headingAngle(this)
            assert(Pose.isRootPose(this.pose.parent), ...
                'Expecting the Robot''s pose to be w.r.t. the World.');
            
            angle = this.pose.angle;
            
            if abs(angle) > 0
                if ~all(abs(this.pose.axis(1:2))<eps)
                    desktop
                    keyboard
                    error(['Expecting Robot pose to be in X/Y plane, so all ' ...
                        'rotation must be around the Z axis.']);
                end
            
                % Make sure rotation is about the positive Z axis so that we
                % can appropriately compare angles as they wrap from -pi to
                % +pi.
                angle = angle*dot(this.pose.axis, [0 0 1]);
            end
            
            % this.pose's angle is the angle of the Robot's x axis, which
            % sticks out the side.  We are interested in the y axis, which
            % points ahead:
            angle = angle + pi/2;
            %             if angle > pi
            %                 angle = angle - 2*pi;
            %             end
%             
%             assert(abs(angle) <= pi, ...
%                 'Expecting headingANgle to be on the interval [-pi,+pi].');
        end
        
        function set.operationMode(this, mode)
            this.operationMode_ = mode;
        end
        
        function mode = get.operationMode(this)
            mode = this.operationMode_;
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