function C = mtimes(A, B)

assert(isa(B, 'DualQuaternion'), 'B must be a DualQuaternion.');

switch(class(A))
    case {'single', 'double'}
        assert(isscalar(A), 'A must be a scalar.');
        C = DualQuaternion(A*B.Qr, A*B.Qd);
        
    case 'DualQuaternion'
        C = DualQuaternion(A.Qr*B.Qr, A.Qr*B.Qd + A.Qd*B.Qr);
        
    otherwise
        error('Multiplying a "%s" by a DualQuaternion is undefined.');
end

end