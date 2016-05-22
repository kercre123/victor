function Pmean = mean(P, varargin)
% Compute the average of a set of Poses, weighted by their covariances.

assert(all(cellfun(@(x)isa(x, 'Pose'), varargin)), ...
    'All inputs should be Pose objects.');

% If ALL poses have covariance information, use it in averaging.
useCovariance = ~isempty(P.sigma) && ...
    all(@(p)~isempty(p.sigma), varargin);

% Use Quaternions to get a mean rotation.  Use the upper left 3x3 block of
% the covariance matrix to weight poses' contributions.
%
% Reference:
%  "Quaternion Averaging", by Markley, Cheng, Crassidis, and Oshman.
%  http://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/20070017872_2007014421.pdf
%
if useCovariance
    % NOTE: This ignores any dependence/correlation between rotation and 
    % translation, as revealed by the covariance matrix.  But we will 
    % assume this to be negligible...

    Q = Quaternion(P.angle, P.axis);
    QRmap = createMap(Q);
    Rsigma = P.sigma(1:3,1:3); % Rotation part of full covariance matrix
    M = QRmap * (Rsigma\QRmap');
    for i = 1:length(varargin)
        Q = Quaternion(varargin{i}.angle, varargin{i}.axis);
        QRmap = createMap(Q);
        M = M + QRmap * (Rsigma\QRmap');
    end
    [largestEvector, temp] = eigs(M, 1, 'SM');  %#ok<NASGU> NOTE: I *DO* need to return "temp", otherwise Matlab seems to flip the sign of the evector??
    Qavg = Quaternion(largestEvector);
    Rmean = Qavg.angle * Qavg.axis; % Rotation vector
    QRmap = createMap(Qavg);
    Rsigma = inv(QRmap'*M*QRmap);
    
    % Combine the translation estimates as though they are Gaussians.  Just
    % take the product of the Gaussians:
    Tmean = P.sigma(4:6,4:6) \ P.T;
    Tsigma_inv = inv(P.sigma(4:6,4:6)); % Translation part of full covariance matrix
    for i = 1:length(varargin)
        sigma_i = varargin{i}.sigma(4:6,4:6);
        Tmean = Tmean + sigma_i \ varargin{i}.T;
        Tsigma_inv = Tsigma_inv + inv(sigma_i);
    end
    Tmean = Tsigma_inv \ Tmean;

    % Create final average pose, with new covariance info:
    Pmean = Pose(Rmean, Tmean, blkdiag(Rsigma, inv(Tsigma_inv)));

else
    if ~isempty(P.sigma) || any(@(p)~isempty(p.sigma), varargin)
        warning(['Taking the mean of Poses with and without ' ...
            'covariance information: will ignore covariance entirely.']);
    end
    
    % Make a cell array of Quaternions, one for each Pose's rotation:
    Q = cellfun(@(p)Quaternion(p.angle, p.axis), [{P} varargin], ...
        'UniformOutput', false);
    
    % Compute average quaternion and convert back to rotation vector:
    Qmean = unitMean(Q{:});
    Rmean = Qmean.angle * Qmean.axis;
    
    % Simple unweighted average of translation vectors:
    Tmean = mean([P.T cellfun(@(p)p.T, varargin)], 2);
    
    % Create final average Pose:
    Pmean = Pose(Rmean, Tmean);
    
end % IF useCovariance

end % FUNCION mean()

function map = createMap(Q)
map = [-Q.q(2:4)'; Q.q(1)*eye(3)+skew3(Q.q(2:4))];
end




function Pavg = meanOld(P1, P2, method)
% Compute the average of two Pose objects.

% Method should be one of DualQuaternionGaussians, PoseGuassians,
% Interpolation, WeightedInterpolation, KeepMostConfident

switch(method)
    
    case 'DualQuaternionGaussians'
        % Convert the two poses to Dual Quaternions:
        DQ1 = DualQuaternion(P1);
        DQ2 = DualQuaternion(P2);
        
        % Use the Dual Quaternions as means of two Gaussians:
        q1 = [DQ1.real.q(:); DQ1.dual.q(:)];
        q2 = [DQ2.real.q(:); DQ2.dual.q(:)];
        
        % Compute the Jacobian matrix dQ/dP, the derivative of the 8 Dual
        % Quaternion parameters w.r.t. the 6 Pose parameters:
        dDQ1_dP1 = dDualQuaternion_wrt_RandT(P1.Rvec, P1.T);
        dDQ2_dP2 = dDualQuaternion_wrt_RandT(P2.Rvec, P2.T);
        
        % Compute the (inverse) covariance matrix in terms of Dual Quaternion
        % parameters (as opposed to the Pose parameters):
        sigma1_inv = dDQ1_dP1'*(P1.sigma\dDQ1_dP1);
        sigma2_inv = dDQ2_dP2'*(P2.sigma\dDQ2_dP2);
        
        % Compute a third Gaussian which is the product of the two Gaussians
        % above:
        sigma3_inv = sigma1_inv + sigma2_inv;
        q3 = sigma3_inv \ (sigma1_inv*q1 + sigma2_inv*q2);
        %q3 = least_squares_norm(sigma3_inv, sigma1_inv*q1 + sigma2_inv*q2);
        
        % Create a Dual Quaternion from that Gaussian's mean:
        DQ3 = DualQuaternion(Quaternion(q3(1:4)/norm(q3(1:4))), Quaternion(q3(5:8)));
        
        % Figure out the Jacobian of the new Dual Quaternion's parameter's w.r.t.
        % the Pose it represents:
        Rmat = DQ3.rotationMatrix;
        Rvec = rodrigues(Rmat);
        T    = DQ3.translation;
        dDQ3_dP3 = dDualQuaternion_wrt_RandT(Rvec, T);
        
        % Create a new average pose from the Dual Quaternion, using the Jacobian to
        % convert the Dual Quaternion covariance computed above to Pose covariance:
        Pavg = Pose(Rvec, T, inv(dDQ3_dP3*sigma3_inv*dDQ3_dP3'));
        
    case 'Interpolation'
        Pavg = interpolate(P1, P2, .5);
        
    case 'WeightedInterpolation'
        
        % Incorporate the uncertainty, kinda.
        w1 = 1/sqrt(trace(P1.sigma));
        w2 = 1/sqrt(trace(P2.sigma));

        w = w1/(w1+w2);
            
        %fprintf('New weight = %.3f, Old weight = %.3f\n', w1, w2);
        
        Pavg = interpolate(P1, P2, w);
        
    case 'KeepMostConfident'
        
        % Incorporate the uncertainty, kinda.
        w1 = 1/sqrt(trace(P1.sigma));
        w2 = 1/sqrt(trace(P2.sigma));
        
        if w1 > w2
            Pavg = P1;
        else
            Pavg = P2;
        end
        % TODO: update/increase Pavg's covariance?
        
    case 'PoseGaussians'
        %     axis1 = P1.axis;
        %     axis2 = P2.axis;
        %
        %     if dot(axis1, axis2) < 0
        %         axis2 = -axis2;
        %     end
        
        mu1 = [P1.Rvec; P1.T];
        mu2 = [P2.Rvec; P2.T];
        
        sigma3_inv = inv(P1.sigma) + inv(P2.sigma);
        mu3 = sigma3_inv \ (P1.sigma\mu1 + P2.sigma\mu2);
        
        Pavg = Pose(mu3(1:3), mu3(4:6), inv(sigma3_inv));
        
    otherwise
        error('Unrecognized method "%s".', method);
        
end % SWITCH(method)

end % FUNCTION mean()

