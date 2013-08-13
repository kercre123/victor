% This object encapsulates the controller code from the original 
% cozmo_basicdrive01.c Webot controller Hanns put together.
classdef BlockWorldWebotController < handle
    
    properties(Constant = true)
    
        TIME_STEP = 64;

        ROBOT_NAME = 'Cozmo';
        
        %Names of the wheels used for steering
        WHEEL_FL = 'wheel_fl';
        WHEEL_FR = 'wheel_fr';
        PITCH    = 'motor_head_pitch';
        LIFT     = 'lift_motor';
        LIFT2    = 'lift_motor2';
        GOAL     = 'goal_motor';
        
        CONNECTOR   = 'connector';
        CAMERA_HEAD = 'cam_head';
        CAMERA_LIFT = 'cam_lift';
        CAMERA_DOWN = 'cam_down';
        
        DRIVE_VELOCITY_SLOW = 5.0;
        TURN_VELOCITY_SLOW  = 1.0;
        HIST                = 50;
        LIFT_CENTER         = -0.275;
        LIFT_UP             = 0.635;
        LIFT_UPUP           = 0.7;

        %Why do some of those not match ASCII codes?
        %Numbers, spacebar etc. work, letters are different, why?
        %a, z, s, x, Space
        CKEY_LIFT_UP    = 65;
        CKEY_LIFT_DOWN  = 90;
        CKEY_HEAD_UP    = 83;
        CKEY_HEAD_DOWN  = 88;
        CKEY_UNLOCK     = 32;
        
    end % Constant Properties
    
    properties
        wheels;      % Steering wheels
        head_pitch;  % Head pitch motor
        lift, lift2; % Lift motors
        con_lift;    % Lift connector
        
        % Cameras:
        cam_head;
        cam_lift;
        cam_down; % for mat
        
        % Count down after unlock until the next lock
        unlockhysteresis = 0;
        
        % Formerly-static properties inside ControlCozmo()
        pitch_angle = 0;
        lift_angle  = BlockWorldWebotController.LIFT_CENTER;
        locked      = false;
    end
    
    methods
        
        function this = BlockWorldWebotController(varargin)
            wb_robot_init();
            
            % Get "handles" to the robots' controllable parts:
            this.wheels = [wb_robot_get_device(this.WHEEL_FL) ...
                wb_robot_get_device(this.WHEEL_FR)];
            this.head_pitch = wb_robot_get_device(this.PITCH);
            this.lift = wb_robot_get_device(this.LIFT);
            this.lift2 = wb_robot_get_device(this.LIFT2);
            
            % Set the motors to velocity mode
            wb_motor_set_position(this.wheels(1), inf);
            wb_motor_set_position(this.wheels(2), inf);
            
            this.SetAngularWheelVelocity(0, 0);
            
            % Set the head pitch to 0
            wb_motor_set_position(this.head_pitch, 0);
            wb_motor_set_position(this.lift, this.LIFT_CENTER);
            wb_motor_set_position(this.lift2, -this.LIFT_CENTER);
            
            % NOTE: Cameras will be enabled by the instantiation of the
            % BlockWorld Camera objects.
            this.cam_head = wb_robot_get_device(this.CAMERA_HEAD);
            this.cam_lift = wb_robot_get_device(this.CAMERA_LIFT);
            this.cam_down = wb_robot_get_device(this.CAMERA_DOWN);
                        
            % get a handler to the connector and the motor.
            this.con_lift = wb_robot_get_device(this.CONNECTOR);
            
            % activate them.
            wb_connector_enable_presence(this.con_lift, this.TIME_STEP);
            
            fprintf('Drive: arrows\n');
            fprintf('Lift up/down: a/z\n');
            fprintf('Lift presets: 1/2/3\n');
            fprintf('Head up/down: s/x\n');
            fprintf('Undock: space\n');
            fprintf('Stop and Save Timing: q\n');
            
            %Update Keyboard every 0.1 seconds
            wb_robot_keyboard_enable(this.TIME_STEP);
            
        end % CONSTRUCTOR BlockWorldWebotController()
        
        
        function SetLiftAngle(this, angle)
            wb_motor_set_position(this.lift,   angle);
            wb_motor_set_position(this.lift2, -angle);
        end

        function SetAngularWheelVelocity(this, left, right)
            % Set an angular wheel velocity in rad/sec
            wb_motor_set_velocity(this.wheels(1), -left);
            wb_motor_set_velocity(this.wheels(2), -right);
        end
              
        function P = GetGroundTruthPose(~, name)
            assert(~isempty(name), ...
                'Cannot request Pose for an empty name string.');
            
            node = wb_supervisor_node_get_from_def(name);
            if isempty(node)
                error('No such node "%s"', name);
            end
           
            rotField = wb_supervisor_node_get_field(node, 'rotation');
            assert(~isempty(rotField), ...
                'Could not find "rotation" field for node named "%s".', name);
            rotation = wb_supervisor_field_get_sf_rotation(rotField);
            assert(isvector(rotation) && length(rotation)==4, ...
                'Expecting 4-vector for rotation of node named "%s".', name);
            
            transField = wb_supervisor_node_get_field(node, 'translation');
            assert(~isempty(transField), ...
                'Could not find "translation" field for node named "%s".', name);
            translation = wb_supervisor_field_get_sf_vec3f(transField);
            assert(isvector(translation) && length(translation)==3, ...
                'Expecting 3-vector for translation of node named "%s".', name);            
            
            % Note the coordinate change here: Webot has y pointing up,
            % while Matlab has z pointing up.  That swap induces a
            % right-hand to left-hand coordinate change (I think).  Also,
            % things in Matlab are defined in millimeters, while Webot is
            % in meters.
            %rotAngle = rotation(4);
            %rotVec   = [-rotation(1) rotation(3) rotation(2)]
            rotation = (rotation(4))*[-rotation(1) rotation(3) rotation(2)];
            translation = 1000*[-translation(1) translation(3) translation(2)];
            P = Pose(rotation, translation);            
        end
        
        function calib = GetCalibrationStruct(this, whichCam)
            if nargin < 2
                whichCam = 'cam_head';
            elseif ~isprop(this, whichCam)
                error('Unknown camera "%s".', whichCam);
            end
            
            camera = this.(whichCam);
            
            width  = wb_camera_get_width(camera);
            height = wb_camera_get_height(camera);
            aspect = width/height;
            
            fov_hor = wb_camera_get_fov(camera);
            fov_ver = fov_hor / aspect;
            
            fx = width / (2*tan(fov_hor/2));
            fy = height / (2*tan(fov_ver/2));
            %fy = fx / aspect;
            
            calib = struct('fc', [fy fy], ...
                'cc', [width height]/2, 'kc', zeros(5,1), 'alpha_c', 0);
        end
        
        function UpdatePose(~, name, pose, transparency)
            
            if nargin < 4
                transparency = 0.0; % fully visible
            end
            
            node = wb_supervisor_node_get_from_def(name);
            if isempty(node)
                error('No node named "%s"', name);
            end
                        
            rotField = wb_supervisor_node_get_field(node, 'rotation');
            assert(~isempty(rotField), ...
                'Could not find "rotation" field for node "%s".', name);
            
            translationField = wb_supervisor_node_get_field(node, 'translation');
            assert(~isempty(translationField), ...
                'Could not find "translation" field for node "%s".', name);
            
            transparencyField = wb_supervisor_node_get_field(node, 'transparency');
            assert(~isempty(transparencyField), ...
                'Could not find "transparency" field for node "%s".', name);
            
            wb_supervisor_field_set_sf_rotation(rotField, ...
                [-pose.axis(1) pose.axis(3) pose.axis(2) pose.angle]);
            
            wb_supervisor_field_set_sf_vec3f(translationField, ...
                [-pose.T(1) pose.T(3) pose.T(2)]/1000);
            
            wb_supervisor_field_set_sf_float(transparencyField, transparency);
        end
    end        
    
end % CLASSDEF BlockWorldWebotController


