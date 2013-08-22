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
    
%     properties(GetAccess = 'protected', SetAccess = 'protected')
%         poseProtected = Pose();
%     end
    
    methods(Access = 'public')
        
        function this = Robot(varargin)
            Name = 'Robot';
            World = [];
            CameraType = 'usb'; % 'usb' or 'webot'
            CameraDevice = [];
            CameraCalibration = [];
            MatCameraDevice = [];
            MatCameraCalibration = [];
            
            appearanceArgs = parseVarargin(varargin);
            
            this.name = Name;
            this.world = World;
            
            this.observationWindow = cell(1, this.ObservationWindowLength);
            
            this.appearance = parseVarargin(this.appearance, appearanceArgs{:});
            
            this.appearance.EyeLength = this.appearance.EyeLengthFraction * ...
                this.appearance.BodyLength;
            this.appearance.EyeRadius = this.appearance.EyeRadiusFraction * ...
                this.appearance.BodyWidth/2;
            
            % From robot to camera frame:
            % Rotation 90 degrees around x axis:
            Rrc = [1 0 0; 0 0 1; 0 -1 0];
            Trc = [0; 30; 43];
                %this.appearance.BodyLength/2; ...
                %this.appearance.BodyHeight + this.appearance.EyeRadius];
        
            % Add uncertainty for Camera pose(s) w.r.t. robot?
            %   1 degree uncertainty in rotation
            %   .1mm uncertainty in translation
            Rcov = pi/180*eye(3); 
            Tcov = (.1)^2*eye(3);
            camPoseCov = blkdiag(Rcov,Tcov);
            this.camera = Camera('device', CameraDevice, ...
                'deviceType', CameraType, ...
                'calibration', CameraCalibration, ...
                'pose', Pose(Rrc, Trc, camPoseCov));
            
            if (isempty(this.world) && ~isempty(MatCameraDevice)) ...
                    || (~isempty(this.world) && this.world.hasMat)
                
                % From robot to camera frame:
                Rrc = [1 0 0; 0 -1 0; 0 0 -1]; % This seems wrong, shouldn't it be negating y and z?
                Trc = [0; 0; 3]; % Based on Webot down-camera position
                this.matCamera = Camera('device', MatCameraDevice, ...
                    'deviceType', CameraType, ...
                    'calibration', MatCameraCalibration, ...
                    'pose', inv(Pose(Rrc, Trc, camPoseCov)));
            end
        end
        
    end % METHODS (public)
    
    methods
        
        function obs = get.currentObservation(this)
            obs = this.observationWindow{1};
        end
        
        function P = get.pose(this)
            % Asking for a robot's pose gives you the pose of the 
            % current observation:
            %P = this.poseProtected;
            if isempty(this.currentObservation)
                P = Pose();
            else
                P = this.currentObservation.pose;
            end
        end
                
        function o = get.origin(this)
            o = this.pose.T;
        end
        
        function set.pose(this, P)
            assert(isa(P, 'Pose'), 'Must provide a Pose object.');
            
            if false % Pose smoothing hack
                
                % Total hack of a filter on the robot's position: if the change
                % is "too big" average the update with the previous position.
                % TODO: Full-blown Kalman Filter and/or Bundle Adjustment.
                prevAngle = norm(this.pose.Rvec);
                newAngle = norm(P.Rvec);
                
                angleChange = abs(newAngle - prevAngle);
                if angleChange > pi
                    angleChange = 2*pi - angleChange;
                end
                
                Tchange = max(abs(this.pose.T - P.T));
                
                angleSigma = 60*pi/180;
                translationSigma = 50;
                
                w_angle = exp(-.5 * angleChange^2 / (2*angleSigma^2));
                w_T     = exp(-.5 * Tchange^2 / (2*translationSigma^2));
                
                w = min(w_angle, w_T);
                
                Rvec = w*P.Rvec + (1-w)*this.pose.Rvec;
                T = w*P.T + (1-w)*this.pose.T;
                
                this.observationWindow{1}.pose = Pose(Rvec, T);
            else
                this.observationWindow{1}.pose = P;
            end
        end
        %}
    end
    
end % CLASSDEF Robot