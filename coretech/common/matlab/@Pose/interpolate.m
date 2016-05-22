function Pavg = interpolate(P1, P2, w, method, shortestPath)
% Interpolate between two Pose objects.

if nargin < 5
    shortestPath = true;
end

if nargin < 4
    method = 'LinearBlend';
end

assert(w>=0 && w<=1, 'Expecting w on the interval [0,1].');

% Currently converting to DualQuaternions to do this for now, since they
% can be linearly interpolated and still return a valid Pose, unlike
% rotation matrix representations...
Q1 = DualQuaternion(P1);
Q2 = DualQuaternion(P2);

switch(method)
    case 'LinearBlend'
        if dot(Q1.real, Q2.real) < 0
            Q2 = DualQuaternion(-Q2.real, -Q2.dual);
        end
        
        Qi = (1-w)*Q1 + w*Q2;
        
        if ~isempty(P1.sigma) && ~isempty(P2.sigma)
            % Need to compute the new covariance too... is this remotely ok?
            sigma_i = (1-w)*P1.sigma + w*P2.sigma;
        else
            sigma_i = [];
        end
        
    case {'SphericalLinearInterp', 'SLERP'}
        error('Not working yet.');
        Qi = DualQuaternion(SLERP(Q1.real, Q2.real, w, shortestPath), ...
            SLERP(Q1.dual, Q2.dual, w, shortestPath) );
        Qi = normalize(Qi);
        
    otherwise
        error('Unrecognized DualQuaternion averaging method "%s".', method);

end % SWITCH(method)

Pavg = Pose(Qi.rotationMatrix, Qi.translation, sigma_i);

end

