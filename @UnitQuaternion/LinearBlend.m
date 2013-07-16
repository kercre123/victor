function Qi = LinearBlend(P, Q, t)
% QLB, Quaternion Linear Blending (approximation of SLERP).
assert(isa(P, 'UnitQuaternion') && isa(Q, 'UnitQuaternion') && ...
    isa(t, 'double') && isscalar(t) && t>=0 && t<=1, ...
    'Expecting two UnitQuaternions and a scalar double, [0,1].');

Qi = (1-t)*P + t*Q;

end