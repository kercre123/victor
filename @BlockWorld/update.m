function update(this, img)

assert(this.numRobots == 1, 'Currently only supporting one robot.');
camera = this.robots{1}.camera;

if nargin < 2 || isempty(img)
    camera.grabFrame();
else
    camera.image = img;
end

seenMarkers = simpleDetector(camera.image);

numSeenMarkers = length(seenMarkers);
if numSeenMarkers == 0
    return;
end

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