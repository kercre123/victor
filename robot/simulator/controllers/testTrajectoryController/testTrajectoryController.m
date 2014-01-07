function testTrajectoryController()

run(fullfile('..', '..', '..', '..', 'matlab', 'initCozmoPath')); 

%%
% Specify start and end pose and speed, as well as duration
x1 = 0;
y1 = 0;
speed1 = .1; % m/s
theta1 = 90 * pi/180;

x2 = 0.1;
y2 = .3500;
speed2 = .1; % m/s
theta2 = 110 * pi/180;

T = 5; % seconds

TIME_STEP = 10; % ms
measurementUpdate = 10; % in number of TIME_STEPs

% Robot geometry 
WheelRadius = 0.02; % in meters
WheelSpacing = 0.06; 

maxAccel = 10; % m/s^2

% Initial plan for motorspeeds
[w_L, w_R] = getMotorSpeeds(x1, y1, speed1, theta1, ...
    x2, y2, speed2, theta2, T, maxAccel, TIME_STEP, ...
    WheelRadius, WheelSpacing);
   
%%

% set up webots robot
wheels = [wb_robot_get_device('wheel_fl') ...
    wb_robot_get_device('wheel_fr')];
            
wheelGyros = [wb_robot_get_device('wheel_gyro_fl') ...
    wb_robot_get_device('wheel_gyro_fr')];

% Set the motors to velocity mode
wb_motor_set_position(wheels(1), inf);
wb_motor_set_position(wheels(2), inf);
          
% enable gyros
wb_gyro_enable(wheelGyros(1), 2);
wb_gyro_enable(wheelGyros(2), 2);

robotNode = wb_supervisor_node_get_from_def('Cozmo');
rotField = wb_supervisor_node_get_field(robotNode, 'rotation');
assert(~rotField.isNull, ...
    'Could not find "rotation" field for node named "%s".', robotNode);

transField = wb_supervisor_node_get_field(robotNode, 'translation');
assert(~transField.isNull, ...
    'Could not find "translation" field for node named "%s".', robotNode);

% execute 
wb_robot_init();

i = 1;

pathFinished = false;
timePassed = 0;
while wb_robot_step(TIME_STEP) ~= -1 || pathFinished
    timePassed = timePassed + TIME_STEP/1000;

    rotation = wb_supervisor_field_get_sf_rotation(rotField);
    assert(isvector(rotation) && length(rotation)==4, ...
        'Expecting 4-vector for rotation of node named "%s".', robotNode);
    
    translation = wb_supervisor_field_get_sf_vec3f(transField);
    assert(isvector(translation) && length(translation)==3, ...
        'Expecting 3-vector for translation of node named "%s".', robotNode);
    
    if mod(i, measurementUpdate*TIME_STEP)==0
        % update path from simulated noisy measurement of current position
        % and and target position
        
        % TODO: simulate noise being less as we get close to target
                
        %targetDist = sqrt((-translation(1)-x2)^2 + (translation(3)-y2)^2);
        
        % Measure position
        x1 = -translation(1);% + .002*randn(1);
        y1 = translation(3);% + .002*randn(1);
        
        % Measure speed
        wL = wb_gyro_get_values(wheelGyros(1)); 
        wR = wb_gyro_get_values(wheelGyros(2));
        speed1 = abs((wL(2) + wR(2))*WheelRadius/2);
        
        % Measure orientation
        theta1 = rotation(4) + pi; % + 0.25*pi/180*randn(1);
        
        % Noise on target measurement decreases as time goes on
        x2_current = x2 + .01*randn(1)/timePassed;
        y2_current = y2 + .01*randn(1)/timePassed;
        theta2_current = theta2 + 5*pi/180*randn(1)/timePassed;
        
        % Use the time remaining from the previous path to create a new
        % path:
        T = 1.05*(length(w_L)-i+1)*TIME_STEP/1000;
        
        [w_L, w_R] = getMotorSpeeds(x1, y1, speed1, theta1, ...
            x2_current, y2_current, speed2, theta2_current, T, maxAccel, TIME_STEP, ...
            WheelRadius, WheelSpacing);
        
        i = 1;
    end
    
    if i > length(w_L)
        wb_motor_set_velocity(wheels(1), 0);
        wb_motor_set_velocity(wheels(2), 0);
        pathFinished = true;
    else
        % Set an angular wheel velocity in rad/sec
        wb_motor_set_velocity(wheels(1), -w_L(i));
        wb_motor_set_velocity(wheels(2), -w_R(i));
        i = i+1;
    end

    if mod(i,5)==0
        subplot 121
        plot(-translation(1), translation(3), 'rx');
        %quiver(-translation(1)*[1 1], translation(3)*[1 1], cos(rotation(4)+pi)*[1 1], sin(rotation(4)+pi)*[1 1], .05, 'r');
        drawnow
    end
    

end

end


 function [w_L, w_R] = getMotorSpeeds(x1, y1, speed1, theta1, ...
            x2, y2, speed2, theta2, T, maxAccel, TIME_STEP, ...
            WheelRadius, WheelSpacing)
        useDubinsPath = true;
        
        if useDubinsPath
            stepSize = .002;
            
            p = dubins([x1 y1 theta1], [x2 y2 theta2], .025, stepSize);
            x = p(1,:);
            y = p(2,:);
            theta = p(3,:);
            
            % total length is the number of steps times the size of each
            % step
            pathLength = (size(p,2)-1) * stepSize;
            
            % speed needed to traverse the entire path in the specified
            % time
            speed = pathLength/T;
            
            % corresponding time at each step
            t_init = cumsum([0 stepSize / speed * ones(1, length(x)-1)]);
            
            % length of each step needed to achieve that speed in the
            % specified TIME_STEP
            stepSize_resample = stepSize / speed;
            
            % resample every TIME_STEP steps
            t = 0:TIME_STEP/1000:T;
            x = interp1(t_init, x, t);
            y = interp1(t_init, y, t);
            theta = interp1(t_init, theta, t);
            
        else
            
            if speed1==0
                T_runway1 = speed1/maxAccel; % Time to get from zero to speed1
                runway1_length = 0.5*maxAccel*T_runway1^2;
                x1_runway = x1 + runway1_length*cos(theta1);
                y1_runway = y1 + runway1_length*sin(theta1);
            else
                x1_runway = x1;
                y1_runway = y1;
                T_runway1 = 0;
            end
            
            T_runway2 = speed2/maxAccel; % Time to get from speed2 to zero
            
            runway2_length = 0.5*maxAccel*T_runway2^2;
            x2_runway = x2 - runway2_length*cos(theta2);
            y2_runway = y2 - runway2_length*sin(theta2);
            
            % Compute the path starting and ending with those constraints
            T1 = T_runway1;
            T2 = T - T_runway2;
            assert(T2 > T1)
            LHS = [T1^3 T1^2 T1 1;
                T2^3 T2^2 T2 1;
                3*T1^2 2*T1 1 0;
                3*T2^2 2*T2 1 0];
            RHS_x = [x1_runway; x2_runway; speed1*cos(theta1); speed2*cos(theta2)];
            RHS_y = [y1_runway; y2_runway; speed1*sin(theta1); speed2*sin(theta2)];
            
            % LHS = [0 0 0 1;
            %     T^3 T^2 T 1;
            %     dT^3 dT^2 dT 1;
            %     (T-dT)^3 (T-dT)^2 (T-dT) 1];
            % RHS_x = [x1; x2; x1 + speed1*dT*cos(theta1); x2 - speed2*dT*cos(theta2)];
            % RHS_y = [y1; y2; y1 + speed1*dT*sin(theta1); y2 - speed2*dT*sin(theta2)];
            
            px = LHS \ RHS_x;
            py = LHS \ RHS_y;
            
            inc = TIME_STEP/1000;
            t_runway1 = 0:inc:T_runway1-inc; % linspace(0,  T1, T_runway1*1000/TIME_STEP);
            t_path    = T1:inc:T2-inc; %linspace(T1, T2, (T2-T1)*1000/TIME_STEP);
            t_runway2 = 0:inc:T_runway2; %linspace(T2, T,  T_runway2*1000/TIME_STEP);
            
            t = [t_runway1 t_path T2+t_runway2];
            
            %t = linspace(0,T, T*1000 / TIME_STEP);
            if isempty(t_runway1)
                x_runway1 = [];
                y_runway1 = [];
            else
                x_runway1 = 0.5*maxAccel*t_runway1.^2 * cos(theta1);
                y_runway1 = 0.5*maxAccel*t_runway1.^2 * sin(theta1);
            end
            x_runway2 = x2 - 0.5*maxAccel*fliplr(t_runway2).^2 * cos(theta2);
            y_runway2 = y2 - 0.5*maxAccel*fliplr(t_runway2).^2 * sin(theta2);
            
            x_path = px(1)*t_path.^3 + px(2)*t_path.^2 + px(3)*t_path + px(4);
            y_path = py(1)*t_path.^3 + py(2)*t_path.^2 + py(3)*t_path + py(4);
            
            x = [x_runway1 x_path x_runway2];
            y = [y_runway1 y_path y_runway2];
            
            %dx = 3*px(1)*t.^2 + 2*px(2)*t + px(3);
            %dy = 3*py(1)*t.^2 + 2*py(2)*t + py(3);
            dx = x([2 2:end])-x([1 1:end-1]);
            dy = y([2 2:end])-y([1 1:end-1]);
            
            theta = atan2(dy, dx);
            
        end % IF useDubinsPath
        
        h_pathAxes = findobj(gcf, 'Type', 'axes', 'Tag', 'PathAxes');
        if isempty(h_pathAxes)
            h_pathAxes = subplot(1, 2, 1, 'Tag', 'PathAxes');
            hold(h_pathAxes, 'on');
            axis(h_pathAxes, 'equal');
            grid(h_pathAxes, 'on');
        else
            set(findobj(h_pathAxes, 'Tag', 'LastPath'), 'LineWidth', 1, 'Color', 'g', 'Tag', '');
        end
               
        plot(x2,y2,'gx', 'Parent', h_pathAxes);
        plot(x,y,'-', 'Parent', h_pathAxes, 'Tag', 'LastPath', 'LineWidth', 2);
        
        %quiver(x,y,cos(theta),sin(theta))
        
        % compute the angular velocities for that path
        [w_R, w_L] = computeWheelAngularVelocity(x, y, theta, t, ...
            'WheelRadius', WheelRadius, 'WheelSpacing', WheelSpacing);
        
        subplot(122)
        hold off
        plot(t, w_R)
        hold on
        plot(t, w_L, 'r')
        
        drawnow
 end

    