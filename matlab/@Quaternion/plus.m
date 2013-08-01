function C = plus(A, B)
% Quaternion addition
assert(isa(A, 'Quaternion') && isa(B, 'Quaternion'), ...
    'A and B must both be Quaternion objects.');

% Note that adding two UnitQuaternions or a Quaternion and a UnitQuaternion
% does not necessarily result in a UnitQuaternion
C = Quaternion(A.q + B.q);

end