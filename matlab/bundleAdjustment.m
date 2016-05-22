function [invRobotPoses, blocks] = bundleAdjustment(invRobotPoses, ...
    blocks, markerCorners, camera, varargin)
% BlockWorld Bundle Adjustment to updat all robot and block poses.
%
% [invRobotPoses, <blocks>] = bundleAdjustment(invRobotPoses, ...
%     blocks, markerCorners, camera, varargin)
%
%   If 'blocks' is not requested as an output, only the robot poses are
%   adjusted.
% 
% ------------
% Andrew Stein
%


changeTolerance = 1e-4; % this is the change of the reprojection error b/w iterations
maxIterations   = 50;
reprojErrThreshold = .5;
%DEBUG_DISPLAY = false;

reprojErrNormFcn = @(e)norm(e);
%reprojErrNormFcn = @(e)median(abs(e));

parseVarargin(varargin{:});

assert(iscell(invRobotPoses) && iscell(blocks) && ...
    iscell(markerCorners) && iscell(camera), ...
    'robotPoses, blockPoses, markerCorners, and camera should be cell arrays.');

numPoses = sum(~cellfun(@isempty, invRobotPoses(:)));

if numPoses == 0
    return;
end

updateBlocks = nargout > 1;

visible = cell(1, numPoses);
for i_pose = 1:numPoses
    assert(size(markerCorners{i_pose},2)==2, 'markerCorners should be Nx2.');
    markerCorners{i_pose} = markerCorners{i_pose}';
    
    visible{i_pose} = all(markerCorners{i_pose} >= 0, 1);
    
    % Fold the camera's pose into the passed-in poses:
    invRobotPoses{i_pose} = inv(camera{i_pose}.pose)*invRobotPoses{i_pose}; %#ok<MINV>
    
end % FOR each robot pose


% Parameters consist of all robots' poses within their observation windows, 
% plus the poses of all blocks.

change = 1;
iter = 0;

lambda = 10; % LM parameter
lambdaAdjFactor = 1.5;


% %%% DEBUG! %%%
% % Perturb the state and see if bundle adjustment can correct
% param = posesToParamsHelper(invRobotPoses, blocks);
% paramPerturb = param + (.02*param).*randn(size(param));
% [invRobotPoses, blocks] = paramToPosesHelper(paramPerturb, ...
%     invRobotPoses, blocks, 'replace');
% %%% DEBUG! %%%


% Get initial reprojection error and Jacobian:
[reprojErr, J] = reprojectionHelper(invRobotPoses, blocks, ...
    markerCorners, camera, visible, updateBlocks);
    
reprojErrNorm = reprojErrNormFcn(reprojErr);
    
while norm(reprojErrNorm) > reprojErrThreshold && ...
        change > changeTolerance && iter < maxIterations
    
    JtJ = J'*J;
    
    param = posesToParamsHelper(invRobotPoses, blocks);
    
    % Take an optimization step: compute the parameter update amount.
    A = JtJ + diag(sparse(lambda*max(lambda,diag(JtJ))));
    b = J'*reprojErr;
    paramUpdate = linsolve(A, b);
    %paramUpdate = robust_least_squares(A, b);
    
    % Roll the update into the current robot/block poses:
    % NOTE: I don't think "update" mode is really working.  Seems
    % better to update the params by actually adding the update rather
    % than computing a pose update from the updated parmas and
    % composing that with the existing poses.
    if updateBlocks
        [invRobotPoses, blocks] = paramToPosesHelper(param+paramUpdate, ...
            invRobotPoses, blocks, 'replace');
    else
        invRobotPoses = paramToPosesHelper(param+paramUpdate, ...
            invRobotPoses, {}, 'replace');
    end
    
    % Compute the new reprojection error, and the Jacobian matrix for
    % the next step (if we need it):
    [reprojErrNew, Jnew] = reprojectionHelper(invRobotPoses, blocks, ...
        markerCorners, camera, visible, updateBlocks);
    
    % Get the norm of the reprojection error to see if we've converged:
    reprojErrNormNew = reprojErrNormFcn(reprojErrNew);
    
    change = abs(reprojErrNorm - reprojErrNormNew);
    
    % See if reprojection error reduced.  If so, use the new parameters
    % and reduce the LM.  Otherwise, leave the parameters where they
    % were and increase the LM parameter.
    if reprojErrNormNew < reprojErrNorm
        lambda = lambda/lambdaAdjFactor;
        
        reprojErr = reprojErrNew;
        reprojErrNorm = reprojErrNormNew;
        J = Jnew;
        
        fprintf('Successful update (reprojErr = %.4f), lambda now %.2f\n', ...
            reprojErrNorm, lambda);
    else
        % Reprojection error did not reduce, so restore previous
        % parameters/poses:
        if updateBlocks
            [invRobotPoses, blocks] = paramToPosesHelper(param, ...
                invRobotPoses, blocks, 'replace');
        else
            invRobotPoses = paramToPosesHelper(param, ...
                invRobotPoses, {}, 'replace');
        end
        
        lambda = lambda*lambdaAdjFactor;
        fprintf('Unsuccessful update (reprojErr = %.4f), lambda now %.2f\n', ...
            reprojErrNorm, lambda);
    end
    
    %change = norm(paramUpdate)/norm(param);
    iter = iter + 1;
    
end % WHILE not converged/done

% Remove the camera's pose from the final poses:
for i_pose = 1:numPoses
    invRobotPoses{i_pose} = camera{i_pose}.pose * invRobotPoses{i_pose};
end

end % FUNCTION bundleAdjustment()


function param = posesToParamsHelper(robotPoses, blocks)
% Convert cell arrays of robot poses and block objects to a param vector

numPoses = length(robotPoses);
Rvec_robot = cell(1,numPoses);
T_robot    = cell(1,numPoses);
for i_pose = 1:numPoses
    Rvec_robot{i_pose} = robotPoses{i_pose}.Rvec;
    T_robot{i_pose}    = robotPoses{i_pose}.T;
end % FOR each robot pose

param = [vertcat(Rvec_robot{:}); vertcat(T_robot{:})];
    
if nargin > 1 && ~isempty(blocks)
    numBlocks = length(blocks);
    Rvec_block = cell(1,numBlocks);
    T_block    = cell(1,numBlocks);
    for i_block = 1:numBlocks
        pose = blocks{i_block}.pose;
        Rvec_block{i_block} = pose.Rvec;
        T_block{i_block}    = pose.T;
    end % FOR each block
    
    param = [param; vertcat(Rvec_block{:}); vertcat(T_block{:})];
end

end % FUNCTION posesToParamsHelper()


function [robotPoses, blocks] = paramToPosesHelper(param, robotPoses, blocks, mode)
% Convert param vector back into cell arrays of robot poses and blocks

numPoses  = length(robotPoses);

param = reshape(param, 3, []);

for i_pose = 1:numPoses
    poseUpdate = Pose(param(:,i_pose), param(:,i_pose+numPoses));
    switch(mode)
        case 'update'
            robotPoses{i_pose} = poseUpdate * robotPoses{i_pose};
        case 'replace'
            robotPoses{i_pose} = poseUpdate;
        otherwise
            error('Unrecognized mode "%s"', mode);
    end
end % FOR each robot pose

if ~isempty(blocks)
    numBlocks = length(blocks);
    
    R_offset = 2*numPoses;
    T_offset = R_offset + numBlocks;
    for i_block = 1:numBlocks
        poseUpdate = Pose(param(:,i_block+R_offset), param(:,i_block+T_offset));
        switch(mode)
            case 'update'
                blocks{i_block}.pose = poseUpdate * blocks{i_block}.pose;
            case 'replace'
                blocks{i_block}.pose = poseUpdate;
            otherwise
                error('Unrecognized mode "%s"', mode);
        end
    end% FOR each block
end

end % FUNCTION paramToPosesHelper()

function [X_world, dXdR, dXdT] = getMarker3Dcoords(blocks)

computeDerivatives = nargout > 1;

numBlocks = length(blocks);

X_world = cell(1, numBlocks);

if computeDerivatives
    dXdR    = cell(1, numBlocks);
    dXdT    = cell(1, numBlocks);
end

for i_block = 1:numBlocks
    block = blocks{i_block};
    numMarkers = block.numMarkers;
    X_world_crnt = cell(1, numMarkers);
    
    if computeDerivatives
        dXdR_crnt    = cell(1, numMarkers);
        dXdT_crnt    = cell(1, numMarkers);
        for i_marker = 1:numMarkers
            marker = block.markers{i_marker};
            [X_world_crnt{i_marker}, dXdR_crnt{i_marker}, ...
                dXdT_crnt{i_marker}] = getPosition(marker);
        end
        dXdR{i_block}    = vertcat(dXdR_crnt{:});
        dXdT{i_block}    = vertcat(dXdT_crnt{:});
        
    else
        for i_marker = 1:numMarkers
            marker = block.markers{i_marker};
            X_world_crnt{i_marker} = getPosition(marker);
        end
    end
    
    X_world{i_block} = vertcat(X_world_crnt{:})'; % Note the transpose!
    
end % FOR each block

X_world = [X_world{:}];

if computeDerivatives
    dXdR    = blkdiag(dXdR{:});
    dXdT    = blkdiag(dXdT{:});
end

end % FUNCTION blockPosesToWorldCoords()


function [reprojErr, J] = reprojectionHelper(robotPoses, blocks, ...
    x_image, camera, visible, updateBlockPoses)

if updateBlockPoses
    [X_world, dXdR_block, dXdT_block] = getMarker3Dcoords(blocks);
else
    % Don't need block pose derivatives
    X_world = getMarker3Dcoords(blocks);
end

numPoses = length(robotPoses);

dxdR_robot = cell(1, numPoses);
dxdT_robot = cell(1, numPoses);
dxdR_block = cell(1, numPoses);
dxdT_block = cell(1, numPoses);

reprojErr = cell(1, numPoses);

for i_pose = 1:numPoses
    vis = visible{i_pose};
    
    if updateBlockPoses
        [x, dX, dxdR_robot{i_pose}, dxdT_robot{i_pose}] = project_points3( ...
            X_world(:,vis), ...
            robotPoses{i_pose}.Rvec, robotPoses{i_pose}.T, ...
            camera{i_pose}.focalLength, camera{i_pose}.center, ...
            camera{i_pose}.distortionCoeffs, camera{i_pose}.alpha);
        
        % Use chain rule to get derivatives of image coords (x) w.r.t. the
        % block poses:
        dxdX = zeros(size(dX,1), 3*size(X_world,2));
        visible3D = [vis; vis; vis];
        dxdX(:,visible3D(:)) = dX;
        dxdR_block{i_pose} = sparse(dxdX * dXdR_block);
        dxdT_block{i_pose} = sparse(dxdX * dXdT_block);
    else
        [x, dxdR_robot{i_pose}, dxdT_robot{i_pose}] = project_points2( ...
            X_world(:,vis), ...
            robotPoses{i_pose}.Rvec, robotPoses{i_pose}.T, ...
            camera{i_pose}.focalLength, camera{i_pose}.center, ...
            camera{i_pose}.distortionCoeffs, camera{i_pose}.alpha);
    end
    
    dxdR_robot{i_pose} = sparse(dxdR_robot{i_pose});
    dxdT_robot{i_pose} = sparse(dxdT_robot{i_pose});
    
    % Compute current reprojection error for this pose
    reprojErr{i_pose} = x_image{i_pose}(:,vis) - x;
    
    if false % DEBUG_DISPLAY of reprojection error
        namedFigure('BA Reprojection')
        subplot(1,numPoses,i_pose), hold off
        plot(x_image{i_pose}(1,vis), x_image{i_pose}(2,vis), '.');
        hold on
        plot(x(1,:), x(2,:), 'rx');
        
        title(norm(reprojErr{i_pose}))
        if i_pose==numPoses
            legend('Observed Points', 'Reprojected Points');
            pause
        end
    end
   
end % FOR each robot pose

reprojErr = [reprojErr{:}];
reprojErr = reprojErr(:);

J = [blkdiag(dxdR_robot{:}) blkdiag(dxdT_robot{:})];

if updateBlockPoses
    J = [J vertcat(dxdR_block{:}) vertcat(dxdT_block{:})];
    assert(size(J,2) == (length(blocks)*6 + numPoses*6), ...
        'Jacobian has an unexpected number of columns.');
else
    assert(size(J,2) == numPoses*6, ...
        'Jacobian has an unexpected number of columns.');
end
    
assert(issparse(J), 'Expecting sparse Jacobian.');

end % FUNCTIOn reprojectionHelper()



