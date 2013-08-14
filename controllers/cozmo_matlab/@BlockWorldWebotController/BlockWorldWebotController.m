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
        
        % World Nodes
        rootNode;
        sceneNodes;
        numSceneNodes;
        
        blockNodes;
        robotNodes;
        
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
            
            this.rootNode = wb_supervisor_node_get_root();
            this.sceneNodes = wb_supervisor_node_get_field(this.rootNode, 'children');
            this.numSceneNodes = wb_supervisor_field_get_count(this.sceneNodes);
                
            % Populate lists of block/robot nodes for later use:
            this.blockNodes = containers.Map('keyType', 'uint32', 'ValueType', 'any');
              
            for i_node = 1:this.numSceneNodes
                
                sceneObject = wb_supervisor_field_get_mf_node(this.sceneNodes, i_node-1);
                nameField = wb_supervisor_node_get_field(sceneObject, 'name');
                if ~nameField.isNull
                    objName = wb_supervisor_field_get_sf_string(nameField);
                    
                    if ~isempty(objName) 
                        if strncmp(objName, 'Block', 5)
                            blockID = str2double(objName(6:8));
                            assert(blockID > 0 && ...
                                blockID <= BlockWorld.MaxBlockTypes, ...
                                'Block node has ID out of range.');
                            %{                            
                            % Find the block's child that is an observation
                            blockChildren = wb_supervisor_node_get_field(sceneObject, 'children');
                            numChildren = wb_supervisor_field_get_count(blockChildren);
                            observationNode = [];
                            for i_child = 1:numChildren
                                blockChild = wb_supervisor_field_get_mf_node(blockChildren, i_child-1);
                                childNameField = wb_supervisor_node_get_field(blockChild, 'name');
                                if ~childNameField.isNull
                                    childName = wb_supervisor_field_get_sf_string(childNameField);
                                    
                                    if strcmp(childName, 'Observation')
                                        observationNode = blockChild;
                                        break;
                                    end
                                end
                                        
                            end % FOR each block child
                            
                            assert(~isempty(observationNode), ...
                                'Could not find Observation Node child of Block %d', ...
                                blockID);
                            
                            blockStruct = struct( ...
                                    'node', sceneObject, ...
                                    'pose', GetNodePose(this, sceneObject), ...
                                    'obsNode', observationNode, ...
                                    'obsPose', GetNodePose(this, observationNode));
                                
                            if isKey(this.blockNodes, blockID)
                                temp = this.blockNodes(blockID);
                                temp(end+1) = blockStruct; %#ok<AGROW>
                                this.blockNodes(blockID) = temp;
                            else
                                this.blockNodes(blockID) = blockStruct;
                            end
                            %}
                                if isKey(this.blockNodes, blockID)
                                    this.blockNodes(blockID) = ...
                                        [this.blockNodes(blockID) {sceneObject}];
                                else
                                    this.blockNodes(blockID) = {sceneObject};
                                end
                                    
                        elseif strncmp(objName, 'Cozmo', 5)
                            this.robotNodes{end+1} = sceneObject;
                        end % IF name is Block or Cozmo
                        
                    end % IF object name isn't empty
                       
                end % IF nameField isn't null
                
            end % FOR each scene node
            
            fprintf('Found %d block node types and %d robot nodes in the world.\n', ...
                this.blockNodes.Count, length(this.robotNodes));
            
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
            
            %fx = width / (2*tan(fov_hor/2));
            fy = height / (2*tan(fov_ver/2));
            %fy = fx / aspect;
            
            calib = struct('fc', [fy fy], ...
                'cc', [width height]/2, 'kc', zeros(5,1), 'alpha_c', 0);
        end
        
        
    end        
    
end % CLASSDEF BlockWorldWebotController


