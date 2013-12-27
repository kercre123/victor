function [w_R, w_L] = computeWheelAngularVelocity(x, y, theta, t, varargin)
% Given a 3DOF path x(t), y(t), theta(t), compute the corresponding wheel
% angular velocities w_R(t) and w_L(t).

% Assume constant velocity between each time step

% distance between front wheels
WheelSpacing = 50; 

% wheel radius
WheelRadius = 10; 

parseVarargin(varargin{:});

x = x(:);
y = y(:);
theta = theta(:);
t = t(:);
assert(length(x) == length(y) && length(y) == length(t) && length(t) == length(theta));

dt = [1; t(2:end)-t(1:end-1)];
assert(all(dt > 0), 'Time should be strictly increasing.')

% TODO: should first entry really be zero here?
dx_dt = [0; x(2:end)-x(1:end-1)] ./ dt;
dy_dt = [0; y(2:end)-y(1:end-1)] ./ dt;
dtheta_dt = [0; angleDiff(theta(2:end),theta(1:end-1))] ./ dt;  

% TODO: exploit small angle approximation since dtheta should generally be
% very small?

s = sqrt(dx_dt.^2 + dy_dt.^2);

v_R =  0.5*WheelSpacing*dtheta_dt + s;
v_L = -0.5*WheelSpacing*dtheta_dt + s;

% convert to angular velocities on the wheels
w_R = v_R/WheelRadius;
w_L = v_L/WheelRadius;



