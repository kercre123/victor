function C = mtimes(A, B)

assert(isa(B, 'DualQuaternion'), 'B must be a DualQuaternion.');

switch(class(A))
    case {'single', 'double'}
        assert(isscalar(A), 'A must be a scalar.');
        C = DualQuaternion(A*B.real, A*B.dual);
        
    case 'DualQuaternion'
        C = DualQuaternion(A.real*B.real, A.real*B.dual + A.dual*B.real);
        
    otherwise
        error('Multiplying a "%s" by a DualQuaternion is undefined.');
end

end