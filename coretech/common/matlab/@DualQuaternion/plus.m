function C = plus(A, B)

assert(isa(A, 'DualQuaternion') && isa(B, 'DualQuaternion'), ...
    'A and B must be DualQuaternions.');

C = DualQuaternion(A.real + B.real, A.dual + B.dual);

end