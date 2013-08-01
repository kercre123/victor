function c = dot(A,B)

classA = class(A);
assert(isa(A, 'Quaternion') && strcmp(classA, class(B)), ...
    'Expecting two quaternions.');

c = A.q'*B.q;

end