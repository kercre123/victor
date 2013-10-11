function out = mtimes(P1, P2)

T1 = [P1.Rmat P1.T; zeros(1,3) 1];

switch(class(P2))
    case 'Pose'
        T2 = [P2.Rmat P2.T; zeros(1,3) 1];
        
        T3 = T1*T2;
        
        newSigma = [];
        if ~isempty(P1.sigma) && ~isempty(P2.sigma)
            % Is this the right way to combine covariance when two Poses
            % are composed??
            newSigma = P1.sigma + P2.sigma;
        end
        out = Pose(T3(1:3,1:3), T3(1:3,4), newSigma);
        
    case 'double'
        assert(size(P2,1)==3, ...
            'Expecting a 3xN matrix of 3D points (DOUBLE precision).');
        
        out = P1.applyTo(P2);
        
    otherwise
        error('Multiplication of a Pose by a "%s" is not defined.', ...
            class(P2));
        
end % SWITCH(class(P2))

end