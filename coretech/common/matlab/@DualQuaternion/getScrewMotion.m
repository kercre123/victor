function [thetaReal, thetaDual, axisReal, axisDual] = getScrewMotion(this)
% Return angle and axis of screw motion corresponding to a DualQuaternion.


% Angle of rotation, thetaReal
thetaReal = 2*acos(this.Qr.q(1)); 
thetaReal = mod(thetaReal,2*pi);

% Translation along the screw axis, thetaDual
if thetaReal == 0
    thetaDual = 2*sqrt(sum(this.Qd.q(2:3).^2));
else
    thetaDual = -2*this.Qd.q(1)/sin(thetaReal/2);
end

% Screw axis, axisReal
if thetaReal == 0
    axisReal = [1; 0; 0];
else 
    axisReal = this.Qr.q(2:4) / sin(thetaReal/2);
end

% Moment axis, axisDual
if thetaReal == 0
    axispoint = zeros(3,1);
else
    m_axis = this.Qd.q(2:4) - d/2 * cos(thetaReal/2)*axis/sin(thetaReal);
    axispoint = cross(axis, m_axis);
end
axisDual = cross(axispoint, axis);
