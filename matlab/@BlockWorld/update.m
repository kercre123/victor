function update(this, img, matImg)

doBundleAdjustment = false;

if nargin < 3 
    matImg = cell(1, this.numRobots);
elseif ~iscell(matImg)
    matImg = {matImg};
end
    
if nargin < 2
    img = cell(1, this.numRobots);
elseif ~iscell(img)
    img = {img};
end

if length(img) ~= this.numRobots
    error('You must provide an image for each of the %d robots.', ...
        this.numRobots);
end

% Let each robot take a new observation
for i_robot = 1:this.numRobots
    this.robots{i_robot}.update(img{i_robot}, matImg{i_robot});
end

if ~isempty(this.groundTruthPoseFcn)
    for i_robot = 1:this.numRobots
        this.groundTruthRobots{i_robot}.pose = ...
            this.getGroundTruthRobotPose(this.robots{i_robot}.name);
    end
end

%% "My" Bundle Adjustment
if doBundleAdjustment && all(~cellfun(@isempty, invRobotPoses(:)))

    
uv = cell(this.numRobots, Robot.ObservationWindowLength);
%R  = cell(this.numRobots, Robot.ObservationWindowLength);
%T  = cell(this.numRobots, Robot.ObservationWindowLength);
invRobotPoses = cell(this.numRobots, Robot.ObservationWindowLength);

numMarkers3D = this.numMarkers;

%seen = false(numMarkers3D*4,1);
for i_robot = 1:this.numRobots
    for i_obs = 1:Robot.ObservationWindowLength
        obs = this.robots{i_robot}.observationWindow{i_obs};
        if ~isempty(obs) && obs.numMarkers > 0
            uv{i_robot, i_obs} = obs.getAllPoints2D(numMarkers3D);
            invRobotPoses{i_robot, i_obs} = inv(obs.pose);
            
            assert(isequal(size(uv{i_robot,i_obs}), [4*numMarkers3D 2]), ...
                'Returned marker locations is the wrong size.');
            
            %seen(all(uv{i_robot,i_obs}>=0,2)) = true;
        end
    end
end

%{
% Remove markers that were instantiated but never seen
for i_robot = 1:this.numRobots
    for i_obs = 1:Robot.ObservationWindowLength
        if ~isempty(uv{i_robot,i_obs})
            uv{i_robot, i_obs} = uv{i_robot, i_obs}(seen,:);
        end
    end
end
%}

% validPoses    = ~cellfun(@isempty, invRobotPoses(:));
% invRobotPoses = invRobotPoses(validPoses);
% uv            = uv(validPoses);

    
    % Just assuming all cameras are same here!
    % TODO: deal with possibly different cameras on each robot?
    cameras = cell(size(invRobotPoses));
    [cameras{:}] = deal(this.robots{1}.camera);
    
    %[Rvec, T, Xnew] = bundleAdjustment(Rvec, T, uv, X(seen,:), cameras);
    numPosesBefore = length(invRobotPoses);
    [invRobotPoses, this.blocks] = bundleAdjustment(invRobotPoses, ...
        this.blocks, uv, cameras);
    assert(length(invRobotPoses) == numPosesBefore, ...
        'Number of poses changed before and after bundle adjustment!');
    
    %tempPoses = cell(this.numRobots, Robot.ObservationWindowLength);
    %tempPoses(validPoses) = invRobotPoses;
   
    for i_robot = 1:this.numRobots
        for i_obs = 1:Robot.ObservationWindowLength
            if ~isempty(this.robots{i_robot}.observationWindow{i_obs}) && ...
                    this.robots{i_robot}.observationWindow{i_obs}.numMarkers > 0
                
                assert(~isempty(invRobotPoses{i_robot, i_obs}), ...
                    'Pose about to be assigned to observation should not be empty.');
                
                this.robots{i_robot}.observationWindow{i_obs}.pose = ...
                    inv(invRobotPoses{i_robot, i_obs});
            end

        end
    end
    
end % IF do bundle adjustment


return

%% OpenCV Bundle Adjustment
            
% Camera calibartion matrix
% TODO: make these per-robot
K = this.robots{1}.camera.calibrationMatrix;
radDistortionCoeffs = this.robots{1}.camera.distortionCoeffs(:);
if length(radDistortionCoeffs)>4
    if any(radDistortionCoeffs(5:end) ~= 0)
        warning('Ignoring any radial distortion coefficients past 4th for bundle adjustment.');
    end
    radDistortionCoeffs = column(radDistortionCoeffs(1:4));
end
            


[X_final, R_final, T_final] = mexBundleAdjust(uv, X, R, T, ...
    K, radDistortionCoeffs);

%{
x_obs = cell(this.numRobots, Robot.ObservationWindowLength);
x_hat = cell(this.numRobots, Robot.ObservationWindowLength);
X     = cell(this.numRobots, Robot.ObservationWindowLength);
dx_dX = cell(this.numRobots, Robot.ObservationWindowLength);
dx_dR = cell(this.numRobots, Robot.ObservationWindowLength);
dx_dT = cell(this.numRobots, Robot.ObservationWindowLength);
bookkeeping = cell(this.numRobots, Robot.ObservationWindowLength);

for i_robot = 1:this.numRobots
    for i_obs = 1:Robot.ObservationWindowLength
        obs = this.robots{i_robot}.observationWindow{i_obs};
        if ~isempty(obs)
            [x_obs{i_robot,i_obs}, x_hat{i_robot,i_obs}, X{i_robot,i_obs}, ...
                dx_dX{i_robot,i_obs}, dx_dR{i_robot,i_obs}, ...
                dx_dT{i_robot,i_obs}, book] = ...
                reproject(obs);
            rep = ones(size(book,1),1);
            bookkeeping{i_robot, i_obs} = [i_robot*rep i_obs*rep book];
        end
    end
end

% Assemble all the observations, predictions, 3D world coordinates, and
% poses into one giant matrix for 
keyboard
%}

return
 
%% TODO:



% Get all observed 2D markers and poses from all robots in all frames
for i_robot = 1:this.numRobots
    markers2D = cell(Robot.ObservationWindowLength,1);
    for i_obs = 1:Robot.ObservationWindowLength
        obs = this.robots{i_robot}.observationWindow{i_obs};
        if ~isempty(obs)
            markers2D{i_obs} = cell(obs.numMarkers,1);
            for i_marker = 1:obs.numMarkers
                markers2D{i_obs}{i_marker} = obs.markers{i_marker}.corners;
            end
        end
    end
    uv{i_robot} = vertcat(markers2D{:});
end
uv = vertcat(uv{:});
    

    
    
 