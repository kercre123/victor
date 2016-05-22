function C = mtimes(A,B)
% UnitQuaternion multiplication.

assert(isa(B, 'UnitQuaternion'), 'B must be a UnitQuaternion.');

C = mtimes@Quaternion(A, B);

% if isa(A, 'UnitQuaternion')
%     % If A and B were both Unit, C will be too, so make it a UnitQuaternion
%     % type as well (this will unnecessiarly re-normalize it...)
%     C = UnitQuaternion(C.q);
% end

end