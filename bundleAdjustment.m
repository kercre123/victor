function [Rvec,T,X_world, Rmat] = bundleAdjustment(Rvec, T, x_image, ...
    X_world,camera, varargin)

%compute_extrinsic
%
%[omckk,Tckk,Rckk] = compute_extrinsic_refine(omc_init,x_kk,X_kk,fc,cc,kc,alpha_c,MaxIter)
%
%Computes the extrinsic parameters attached to a 3D structure X_kk given its projection
%on the image plane x_kk and the intrinsic camera parameters fc, cc and kc.
%Works with planar and non-planar structures.
%
%INPUT: x_kk: Feature locations on the images
%       X_kk: Corresponding grid coordinates
%       fc: Camera focal length
%       cc: Principal point coordinates
%       kc: Distortion coefficients
%       alpha_c: Skew coefficient
%       MaxIter: Maximum number of iterations
%
%OUTPUT: omckk: 3D rotation vector attached to the grid positions in space
%        Tckk: 3D translation vector attached to the grid positions in space
%        Rckk: 3D rotation matrices corresponding to the omc vectors

%
%Method: Computes the normalized point coordinates, then computes the 3D pose
%
%Important functions called within that program:
%
%normalize_pixel: Computes the normalize image point coordinates.
%
%pose3D: Computes the 3D pose of the structure given the normalized image projection.
%
%project_points.m: Computes the 2D image projections of a set of 3D points

changeTolerance = 1e-10;
maxIterations   = 50;
conditionThreshold = inf;
reprojErrThreshold = .01;
%DEBUG_DISPLAY = false;

parseVarargin(varargin{:});

assert(iscell(Rvec) && iscell(T) && iscell(x_image) && iscell(camera), ...
    'Rvec, T, x_image, and should camera should be cell arrays.');

numPoses = sum(~cellfun(@isempty, Rvec(:)));

assert(size(X_world,2)==3, 'X_world should be Nx3.');

X_world = X_world';

visible = cell(1, numPoses);
for i_pose = 1:numPoses
    assert(size(x_image{i_pose},2)==2, 'x_images should be Nx2.');
    x_image{i_pose} = x_image{i_pose}';
    
    visible{i_pose} = all(x_image{i_pose} >= 0, 1);
    
    % Fold the camera's frame into the passed-in frames:
    tempFrame = Frame(Rvec{i_pose}, T{i_pose});
    tempFrame = inv(camera{i_pose}.frame)*tempFrame; %#ok<MINV>
    
    Rvec{i_pose} = tempFrame.Rvec;
    T{i_pose}    = tempFrame.T;
end

        
param = [vertcat(Rvec{:});vertcat(T{:});X_world(:)];

change = 1;

iter = 0;

lambda = 0.5; % LM parameter
lambdaAdjFactor = 1.5;

%fprintf(1,'Gradient descent iterations: ');

% Get initial reprojection error and Jacobians
[reprojErr, J] = reprojectionHelper(param, numPoses, x_image, camera, visible);
    
reprojErrNorm = norm(reprojErr);
    
while norm(reprojErrNorm) > reprojErrThreshold && ...
        change > changeTolerance && iter < maxIterations
    
    if false && cond(JJ) > conditionThreshold,
        change = 0;
    else
        
        JtJ = J'*J;
        
        paramNew = param;
        
        %param_innov = inv(JJ2)*(JJ')*ex(:);
        paramUpdate = (JtJ + lambda*diag(diag(JtJ))) \ (J'*reprojErr);
        %paramUpdate = robust_least_squares(JtJ + mu*eye(size(JtJ)), J'*reprojErr);
               
        newFrames = cell(1, numPoses);
        
        % Compose the pose frames with the updates:
        Rindex = 1:3;
        Tindex = 4:6;
        for i_pose = 1:numPoses
            frame = Frame(Rvec{i_pose}, T{i_pose});
            frameUpdate = Frame(paramUpdate(Rindex), paramUpdate(Tindex));
            newFrames{i_pose} = frameUpdate * frame;
            paramNew(Rindex) = newFrames{i_pose}.Rvec;
            paramNew(Tindex) = newFrames{i_pose}.T;
            
            Rindex = Rindex + 6;
            Tindex = Tindex + 6;
            %Rvec{i_pose} = frame.Rvec;
            %T{i_pose} = frame.T;
        end
        
        % Update the world point coordinates
        Xindex = Tindex(end)+1;
        paramNew(Xindex:end) = paramNew(Xindex:end) + paramUpdate(Xindex:end);
        
        [reprojErrNew, Jnew] = reprojectionHelper(paramNew, numPoses, ...
            x_image, camera, visible);
        
        
        % See if reprojection error reduced.  If so, use the new parameters
        % and reduce the LM.  Otherwise, leave the parameters where they
        % were and increase the LM parameter.
        reprojErrNormNew = norm(reprojErrNew);
        if reprojErrNormNew < reprojErrNorm
            lambda = lambda/lambdaAdjFactor;
            
            param = paramNew;
            reprojErr = reprojErrNew;
            reprojErrNorm = reprojErrNormNew;
            J = Jnew;
            
            fprintf('Successful update, lambda now %.2f\n', lambda);
        else
            lambda = lambda*lambdaAdjFactor;
            fprintf('Unsuccessful update, lambda now %.2f\n', lambda);
        end
        
        change = norm(paramUpdate)/norm(param);
        iter = iter + 1;
        
    end;
    
end;

Rindex = [1 2 3];
Tindex = [4 5 6];
X_world = reshape(param((numPoses*6 + 1):end), 3, [])';

for i_pose = 1:numPoses
    % Remove the camera's frame from the final frames:
    tempFrame = Frame(param(Rindex), param(Tindex));
    tempFrame = camera{i_pose}.frame*tempFrame;   
    Rvec{i_pose} = tempFrame.Rvec;
    T{i_pose}    = tempFrame.T;
end

if nargout > 3
    Rmat = cell(1, numPoses);
    for i_pose = 1:numPoses
        Rmat{i_pose} = rodrigues(Rvec{i_pose});
    end
end

end % FUNCTION bundleAdjustment()


function [reprojErr, J] = reprojectionHelper(params, numPoses, ...
    x_image, camera, visible)

Rindex = [1 2 3];
Tindex = [4 5 6];
X_world = reshape(params((numPoses*6 + 1):end), 3, []);

dxdX  = cell(1, numPoses);
dxdR  = cell(1, numPoses);
dxdT  = cell(1, numPoses);
reprojErr = cell(1, numPoses);

for i_pose = 1:numPoses
    vis = visible{i_pose};
    
    Rvec = params(Rindex);
    T    = params(Tindex);
    
    [x,dX,dxdR{i_pose},dxdT{i_pose}] = project_points3( ...
        X_world(:,vis), ...
        Rvec, T, ...
        camera{i_pose}.focalLength, camera{i_pose}.center, ...
        camera{i_pose}.distortionCoeffs, camera{i_pose}.alpha);
    
    dxdX{i_pose} = zeros(size(dX,1), 3*size(X_world,2));
    visible3D = [vis; vis; vis];
    dxdX{i_pose}(:,visible3D(:)) = dX;
    
    % Compute current reprojection error for this pose
    reprojErr{i_pose} = x_image{i_pose}(:,vis) - x;
    
    if true
        namedFigure('BA Reprojection')
        subplot(1,numPoses,i_pose), hold off
        plot(x_image{i_pose}(1,vis), x_image{i_pose}(2,vis), '.');
        hold on
        plot(x(1,:), x(2,:), 'rx');
    end
    
    Rindex = Rindex + 6;
    Tindex = Tindex + 6;
  
end

reprojErr = [reprojErr{:}];
reprojErr = reprojErr(:);

J = [blkdiag(dxdR{:}) blkdiag(dxdT{:}) vertcat(dxdX{:})];
    
end % FUNCTIOn reprojectionHelper()

