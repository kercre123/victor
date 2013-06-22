function update(this, img)

doBundleAdjustment = false;

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
    this.robots{i_robot}.update(img{i_robot});
end

uv = cell(this.numRobots, Robot.ObservationWindowLength);
R  = cell(this.numRobots, Robot.ObservationWindowLength);
T  = cell(this.numRobots, Robot.ObservationWindowLength);

numMarkers3D = this.numMarkers;

seen = false(numMarkers3D*4,1);
for i_robot = 1:this.numRobots
    for i_obs = 1:Robot.ObservationWindowLength
        obs = this.robots{i_robot}.observationWindow{i_obs};
        if ~isempty(obs) && obs.numMarkers > 0
            uv{i_robot, i_obs} = obs.getAllPoints2D(numMarkers3D);
            frame = obs.frame;
            R{i_robot, i_obs} = frame.Rmat;
            T{i_robot, i_obs} = frame.T;
            
            assert(isequal(size(uv{i_robot,i_obs}), [4*numMarkers3D 2]), ...
                'Returned marker locations is the wrong size.');
            
            seen(all(uv{i_robot,i_obs}>=0,2)) = true;
        end
    end
end

X = this.getAllPoints3D;

% Remove markers that were instantiated but never seen
for i_robot = 1:this.numRobots
    for i_obs = 1:Robot.ObservationWindowLength
        if ~isempty(uv{i_robot,i_obs})
            uv{i_robot, i_obs} = uv{i_robot, i_obs}(seen,:);
        end
    end
end


%% "My" Bundle Adjustment
if doBundleAdjustment
    cameras = cell(size(R));
    [cameras{:}] = deal(this.robots{1}.camera);
    Rvec = cell(size(R));
    for i=1:length(R)
        if ~isempty(R{i})
            Rvec{i} = rodrigues(R{i});
        end
    end
    
    [Rvec, T, Xnew] = bundleAdjustment(Rvec, T, uv, X(seen,:), cameras);
    
    index = [1 2 3 4];
    for i_marker = 1:this.numMarkers
        i_X = (i_marker-1)*4 + 1;
        if seen(i_X)
            M = this.allMarkers3D{i_marker};
            M.P = Xnew(index,:);
            index = index + 4;
        end
    end
    
    for i_robot = 1:this.numRobots
        for i_obs = 1:Robot.ObservationWindowLength
            obs = this.robots{i_robot}.observationWindow{i_obs};
            if ~isempty(obs) && obs.numMarkers > 0
                obs.frame = Frame(Rvec{i_robot, i_obs}, T{i_robot, i_obs});
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
    

    
    
 