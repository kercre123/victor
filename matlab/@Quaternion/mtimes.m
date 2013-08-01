function C = mtimes(A,B)
% Quaternion multiplication.

assert(isa(B, 'Quaternion'), 'B must be a Quaternion.');

classA = class(A);
if any(strcmp(classA, {'single', 'double'}))
    C = Quaternion(double(A)*B.q);
        
elseif isa(A, 'Quaternion')
    C = Quaternion([A.r*B.r - A.v'*B.v;
        A.r*B.v + B.r*A.v + cross(A.v, B.v)]);
else
    error('Multiplication of a "%s" by a Quaternion is undefined.', ...
        classA);
end
    
end