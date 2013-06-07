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
    
        % Figure out where the marker is in 3D space:
        seenMarkers{i_marker}.frame = camera.computeExtrinsics( ...
            seenMarkers{i_marker}.imgCorners, ...
            seenMarkers{i_marker}.Pmodel);
        
        this.markers{seenMarkers{i_marker}.blockType, ...
            seenMarkers{i_marker}.faceType} = seenMarkers{i_marker};
        
        % Rotate the block to match the marker and set it's origin to the
        % marker's origin:
        blockFrame = Frame(seenMarkers{i_marker}.frame.Rmat, ...
            seenMarkers{i_marker}.origin);
        this.blocks{seenMarkers{i_marker}.blockType} = ...
            Block(seenMarkers{i_marker}, blockFrame);
        
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
    
    matchedBlocks = false(1,numSeenMarkers);
    p_marker = cell(1,numSeenMarkers);
    P_marker = cell(1,numSeenMarkers);
    for i_marker = 1:numSeenMarkers
        bType = seenMarkers{i_marker}.blockType;
        fType = seenMarkers{i_marker}.faceType;
        if ~isempty(this.markers{bType, fType})
            matchedBlocks(i_marker) = true;
            p_marker{i_marker} = seenMarkers{i_marker}.imgCorners;
            P_marker{i_marker} = this.markers{bType, fType}.P;
        end
        
    end
    
    if ~any(matchedBlocks)
        warning(['No blocks found matched one seen before. Not sure yet ' ...
            'what to do in this case.']);
    else
        p_marker = vertcat(p_marker{:});
        P_marker = vertcat(P_marker{:});
        invRobotFrame = camera.computeExtrinsics(p_marker, P_marker);
        this.robots{1}.frame = inv(invRobotFrame);
    end
        
end

end % FUNCTION BlockWorld/update()