function C = plus(A, B)

assert(isa(A, 'DualQuaternion') && isa(B, 'DualQuaternion'), ...
    'A and B must be DualQuaternions.');

C = DualQuaternion(A.Qr + B.Qr, A.Qd + B.Qd);

end