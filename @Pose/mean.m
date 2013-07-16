function Pavg = mean(P1, P2, method)
% Compute the average of two Pose objects.

if nargin < 3
    method = 'LinearBlend';
end

% Currently converting to DualQuaternions to do this for now.
Q1 = DualQuaternion(P1.Rvec, P1.T);
Q2 = DualQuaternion(P2.Rvec, P2.T);

switch(method)
    case 'LinearBlend'
        Qavg = DualQuaternion.LinearBlend(Q1, Q2, 0.5);
        
    case 'SphericalLinearInterp'
        Qavg = DualQuaternion.SphericalLinearInterp(Q1, Q2, 0.5);
        
    otherwise
        error('Unrecognized DualQuaternion averaging method "%s".', method);

end % SWITCH(method)

Pavg = Pose(Qavg.rotationMatrix, Qavg.translation);

end

