function C = mtimes(A,B)
% Quaternion multiplication.

assert(isa(B, 'Quaternion'), 'B must be a Quaternion.');

classA = class(A);
if any(strcmp(classA, {'single', 'double'}))
    C = Quaternion(double(A)*B.q);
        
elseif isa(A, 'Quaternion')
    %C = Quaternion([A.r*B.r - A.v'*B.v;
    %    A.r*B.v + B.r*A.v + cross(A.v, B.v)]);
    a1 = A.q(1); b1 = A.q(2); c1 = A.q(3); d1 = A.q(4);
    a2 = B.q(1); b2 = B.q(2); c2 = B.q(3); d2 = B.q(4);
    
    C = Quaternion( ...
        [a1*b2 + b1*a2 + c1*d2 - d1*c2;
         a1*c2 - b1*d2 + c1*a2 + d1*b2;
         a1*d2 + b1*c2 - c1*b2 + d1*a2;
         a1*a2 - b1*b2 - c1*c2 - d1*d2 ] );
    
else
    error('Multiplication of a "%s" by a Quaternion is undefined.', ...
        classA);
end
    
end