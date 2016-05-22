function Qi = SphericalLinearInterp(P, Q, t)
% SLERP, Spherical Linear Interpolation of Quaternions P and Q at t = [0,1].

assert(isa(P, 'UnitQuaternion') && isa(Q, 'UnitQuaternion') && isscalar(t) ...
    && isa(t, 'double'), 'Expecting two UnitQuaternions and a scalar double.');

Qi = P * (conj(P) * Q)^t;

end