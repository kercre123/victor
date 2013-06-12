classdef Robot < handle
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        modelX;
        modelY;
        modelZ;
        
        X;
        Y;
        Z;
                
        camera;
        
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
            'EyeColor', [.5 0 0]);
        
    end % PROPERTIES (get-public, set-protected)
    
    properties(GetAccess = 'public', SetAccess = 'public', ...
            Dependent = true)
        
        frame;
    end
    
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        
        handles;        
        
    end % PROPERTIES (get-public, set-protected)
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        frame_ = Frame();
    end
    
    methods(Access = 'public')
        
        function this = Robot(cameraCalibration, cameraDevice, varargin)
            
            this.appearance = parseVarargin(this.appearance, varargin{:});
            
            this.appearance.EyeLength = this.appearance.EyeLengthFraction * ...
                this.appearance.BodyLength;
            this.appearance.EyeRadius = this.appearance.EyeRadiusFraction * ...
                this.appearance.BodyWidth/2;
            
            % From robot to camera frame:
            % Rotation 90 degrees around x axis:
            Rrc = [1 0 0; 0 0 -1; 0 1 0];
            Trc = [0; ...
                this.appearance.BodyLength/2; ...
                this.appearance.BodyHeight + this.appearance.EyeRadius];
        
            this.camera = Camera('device', cameraDevice, ...
                'calibration', cameraCalibration, ...
                'frame', inv(Frame(Rrc, Trc)));
                        
           
        end
        
    end % METHODS (public)
    
    methods
        
        function F = get.frame(this)
            F = this.frame_;
        end
        
        function set.frame(this, F)
            assert(isa(F, 'Frame'), 'Must provide a Frame object.');
            
            % Total hack of a filter on the robot's position: if the change
            % is "too big" average the update with the previous position.
            % TODO: Full blown Kalman Filter or Bundle Adjustment.
            prevAngle = norm(this.frame.Rvec);
            newAngle = norm(F.Rvec);
            
            angleChange = abs(newAngle - prevAngle);
            if angleChange > pi
                angleChange = 2*pi - angleChange;
            end
            
            Tchange = max(abs(this.frame.T - F.T));
            
            angleSigma = 30*pi/180;
            translationSigma = 30;
            
            w_angle = exp(-.5 * angleChange^2 / (2*angleSigma^2));
            w_T     = exp(-.5 * Tchange^2 / (2*translationSigma^2));
    
            w = min(w_angle, w_T);
            
            Rvec = w*F.Rvec + (1-w)*this.frame_.Rvec;
            T = w*F.T + (1-w)*this.frame_.T;
            
            this.frame_ = Frame(Rvec, T);
            
        end
        
    end
    
end % CLASSDEF Robot