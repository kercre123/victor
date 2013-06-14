function update(this, img)

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


return
 
%% TODO:

% Do bundle adjustment on all the observations of all the robots
uv = cell(this.numRobots, 1);
XYZ = cell(this.MaxBlocks, 1);

% Get all 3D locations of all markers on all blocks:
for i_block = 1:this.MaxBlocks
    B = this.blocks{i_block};
    if ~isempty(B)
        N = B.numMarkers;
        XYZ{i_block} = cell(N,1);
        faces = B.faces;
        for i_face = 1:N
            M = B.markers(faces{i_face});
            XYZ{i_block}{i_face} = M.P;
        end
    end
    
    XYZ{i_block} = vertcat(XYZ{i_block}{:});
end
XYZ{i_block} = vertcat(XYZ{:});

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
    

    
    
    

return;

if this.numBlocks == 0
    % If we haven't instantiated any blocks in our world, let the world
    % origin start at the Robot's current position and instantiate a block
    % for each marker we detected.
    for i_marker = 1:numSeenMarkers
    
        addMarkerAndBlock(this, this.robots{1}, seenMarkers{i_marker});
        
    end
    
else
    % We've already got world coordinates set up
    
    % Two cases:
    % 1. We see at least one block again that we've already seen.  Update
    %    the robot's position relative to that block's position (or the
    %    average of where the set of re-detected blocks say the robot is)
    % 2. We don't see any blocks we've seen before. We could instatiate the
    %    new blocks based on the last known position of the robot, but this
    %    seems pretty error-prone, since we presumably moved if we're
    %    seeing a new block...
    
    % Find markers we've seen before
    matchedMarkers = false(1,numSeenMarkers);
    p_marker = cell(1,numSeenMarkers);
    P_marker = cell(1,numSeenMarkers);
    for i_marker = 1:numSeenMarkers
        bType = seenMarkers{i_marker}.blockType;
        fType = seenMarkers{i_marker}.faceType;
        if bType > this.MaxBlocks
            warning('Out-of-range block detected! (%d > %d)', bType, this.MaxBlocks);
            keyboard
        elseif fType > this.MaxFaces
            warning('Out-of-range face detected! (%d > %d)', fType, this.MaxFaces);
            keyboard
        end
        
        if ~isempty(this.markers{bType, fType})
            matchedMarkers(i_marker) = true;
            p_marker{i_marker} = seenMarkers{i_marker}.imgCorners;
            P_marker{i_marker} = this.markers{bType, fType}.P;
        end
        
    end
    
    % Use previously-seen markers to update the robot/camera's position
    if ~any(matchedMarkers)
        warning(['No markers found matched one seen before. Not sure yet ' ...
            'what to do in this case.']);
    else
        p_marker = vertcat(p_marker{:});
        P_marker = vertcat(P_marker{:});
        invRobotFrame = camera.computeExtrinsics(p_marker, P_marker);
        this.robots{1}.frame = inv(invRobotFrame);
    end
    
    % Add new markers/blocks to world, relative to robot's updated position
    for i_marker = 1:numSeenMarkers
        if ~matchedMarkers(i_marker)
            addMarkerAndBlock(this, this.robots{1}, seenMarkers{i_marker});        
        end
    end % FOR each marker
    
    % Do bundle adjustment on all markers' 3D positions and last several
    % poses
    [P2,Rvec2,T2,Rmat2] = bundleAdjustment(rodrigues(Rmat), T, p', P', ...
        this.focalLength, this.center, this.distortionCoeffs, ...
        this.alpha, maxRefineIterations, threshCond);
end

end % FUNCTION BlockWorld/update()


function addMarkerAndBlock(this, robot, marker)

% Figure out where the marker is in 3D space, in camera's world
% coordinates, which is the robot's frame!
marker.frame = robot.camera.computeExtrinsics(marker.imgCorners, marker.Pmodel);

% Put the marker into the robot's world frame
marker.frame = robot.frame * marker.frame;

% Add the marker to the world's list
this.markers{marker.blockType, marker.faceType} = marker;

% Rotate the block to match the marker and set it's origin to the
% marker's origin:
blockFrame = Frame(marker.frame.Rmat, marker.origin);
this.blocks{marker.blockType} = Block(marker, blockFrame);

end % FUNCTION addMarkerAndBlock()

