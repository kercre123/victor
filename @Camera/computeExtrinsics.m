function varargout = computeExtrinsics(this, p, P, varargin)

threshCond = 1e6;
maxRefineIterations = 20;
initializeWithCurrentPose = false;

parseVarargin(varargin{:});


if initializeWithCurrentPose
    invPose = inv(this.pose);
    Rvec = invPose.Rvec;
    T    = invPose.T;
else
    % TODO: write my own version instead of using Bouguet's (or use openCV's)
    [Rvec,T] = compute_extrinsic_init(p', P', ...
       this.focalLength, this.center, this.distortionCoeffs, this.alpha);
end

if maxRefineIterations > 0
    %Y_kk = (rodrigues(Rvec)*X_kk' + Tckk(:,ones(1,size(X_kk,1))))';
    %plot3(Y_kk([1 3 4 2 1],1), Y_kk([1 3 4 2 1], 3), -Y_kk([1 3 4 2 1], 2), 'r--');
    
    % TODO: write my own version instead of using Bouguet's (or use openCV's)
    [~,T,Rmat] = compute_extrinsic_refine(Rvec, T, p', P', ...
        this.focalLength, this.center, this.distortionCoeffs, ...
        this.alpha, maxRefineIterations, threshCond);
else
    Rmat = rodrigues(Rvec);
end

% Put camera coordinates into "world" coordinates (or the coordinates of
% whatever is holding this camera, e.g. a Robot)
P = this.pose * Pose(Rmat, T);
%F = Frame(Rmat, T);

if nargout == 2
    varargout = {P.Rmat, P.T};
else
    varargout = {P};
end
    
end % FUNCTION computeExtrinsics()