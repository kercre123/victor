function varargout = computeExtrinsics(this, p, P, varargin)

maxRefineIterations = 20;
initializeWithCurrentPose = false;
changeThreshold = 1e-10;
ridgeWeight = 1e-2;

parseVarargin(varargin{:});


if initializeWithCurrentPose
    invPose = inv(this.pose); %#ok<UNRCH>
    Rvec = invPose.Rvec;
    T    = invPose.T;
else
    % TODO: write my own version instead of using Bouguet's (or use openCV's)
    [Rvec,T] = compute_extrinsic_init(p', P', ...
       this.focalLength, this.center, this.distortionCoeffs, this.alpha);
end

if maxRefineIterations > 0
    % Refine by minimizing reprojection error
    
    param = [Rvec; T];
    
    change = 1;
    iter = 0;
    
    while change > changeThreshold && iter < maxRefineIterations
        
        % TODO: update my Camera/projectPoints function to return the
        % derivatives w.r.t. R and T
        [p_obs, dp_dRvec, dp_dT] = project_points2(P', Rvec, T, ...
            this.focalLength, this.center, this.distortionCoeffs, this.alpha);
        
        %     namedFigure('CameraExtrinsicRefine');
        %     hold off, plot(x_kk(1,:), x_kk(2,:), 'b.');
        %     hold on, plot(x(1,:), x(2,:), 'rx');
        %     pause
        
        reprojErr = p' - p_obs;
        
        J = [dp_dRvec dp_dT];
        
        JtJ = J'*J;
        A = JtJ + ridgeWeight*eye(size(J,2));
        b = J'*reprojErr(:);
        
        paramUpdate = A \ b;
        % paramUpdate = robust_least_squares(A, b);
        
        param = param + paramUpdate;
        change = norm(paramUpdate)/norm(param);
        
        iter = iter + 1;
        
        Rvec = param(1:3);
        T    = param(4:6);
        
    end % WHILE still refining
    
    Rmat = rodrigues(Rvec);
    
else
    Rmat = rodrigues(Rvec);
end % IF do refinement


% Put camera coordinates into "world" coordinates (or the coordinates of
% whatever is holding this camera, e.g. a Robot)
poseCov = inv(JtJ);
P = this.pose * Pose(Rmat, T, poseCov);
%F = Frame(Rmat, T);

switch(nargout)
    case 3
        varargout = {P.Rmat, P.T, J};
    case 2
        varargout = {P.Rmat, P.T};
    case 1
        varargout = {P};
    otherwise
        error('Unrecognized number of output arguments.');
end
    
end % FUNCTION computeExtrinsics()