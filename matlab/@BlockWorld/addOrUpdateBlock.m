function addOrUpdateBlock(this, robot, markers2D, mergeMethod)
% Adds or updates block(s) in a BlockWorld, given observed BlockMarker2Ds.

if nargin < 4
    mergeMethod = 'replace';
end

if ~iscell(markers2D)
    markers2D = {markers2D};
end

assert(all(cellfun(@(m)isa(m, 'BlockMarker2D'), markers2D)), ...
    'All markers must be BlockMarker2D objects.');

blockType = markers2D{1}.blockType;

numMarkers2D = length(markers2D);
if numMarkers2D > 1
    assert(all(blockType == cellfun(@(marker)marker.blockType, markers2D(2:end))), ...
        'All 2D markers should have the same blockType.');
end

assert(blockType < this.MaxBlockTypes, ...
    'Out-of-range block type (%d) seen.', blockType);


%% Determine Pose
% First figure out where each of the 2D markers implies the 3D block must 
% be.  We will instantiate a Block to do this, since we need its 3D 
% markers, but that block may not be added to the world list if it gets 
% merged into an existing block below, based on its estimated pose.

% Instantiate the block (at canonical location).  This in turn will
% instantiate its faces' 3D markers.
B_init = Block(blockType, this.numMarkers);

% Estimate a pose for each 3D marker individually (remember, we don't yet
% know if we are seeing faces of multiple blocks of the same type)
blockPose = cell(1, numMarkers2D);
for i_marker = 1:numMarkers2D
    blockPose{i_marker} = BlockWorld.computeBlockPose(robot, B_init, ...
        markers2D(i_marker));
end

if numMarkers2D > 1
    % Group markers which could be part of the same block (i.e. those whose
    % poses are colocated, noting that the pose is relative to the center 
    % of the block)
    whichBlock = zeros(1, numMarkers2D);
    numBlocks = 0;
    for i_marker = 1:numMarkers2D
        if whichBlock(i_marker) == 0 % this marker hasn't been assigned to a block yet
            % start new block with this marker
            numBlocks = numBlocks + 1;
            whichBlock(i_marker) = numBlocks; 
            
            % see how far unassigned markers' poses are from this one
            unused = find(whichBlock==0);
            d = compare(blockPose{i_marker}, blockPose(unused));
            
            % all markers whose corresponding pose is close enough should
            % be clustered with this one
            whichBlock(unused(d < B_init.mindim/2)) = numBlocks;            
        end
    end
    
    assert(all(whichBlock > 0), 'All markers should have been assigned a block.');
    
    %fprintf('Grouping %d observed 2D markers of type %d into %d blocks.\n', ...
    %    numMarkers2D, B_init.blockType, numBlocks);
    
    % For each new block, re-estimate the pose based on all markers that 
    % were assigned to it
    B_new = cell(1, numBlocks);
    for i_blockExist = 1:numBlocks
        whichMarkers = find(whichBlock == i_blockExist);
        assert(~isempty(whichMarkers), ...
            'whichMarkers should never be empty.');
        
        B_new{i_blockExist} = Block(blockType, this.numMarkers);
        if length(whichMarkers)==1
            % No need to re-estimate, just use the one marker we saw.
            B_new{i_blockExist}.pose = blockPose{whichMarkers};
        else
            B_new{i_blockExist}.pose = BlockWorld.computeBlockPose(robot, ...
                B_init, markers2D(whichMarkers));            
        end
    end % FOR each of the new blocks
    
else
    numBlocks = 1;
    B_new = {B_init};
    B_new{1}.pose = blockPose{1};
end % IF numMarkers > 1

%% Add / Merge Block(s)
% Based on the computed pose, decide whether to update an existing block's
% pose with any of the new observations or to add new blocks

for i_blockObs = 1:numBlocks
    
    % Loop over the existing blocks of this type and see if we overlap with
    % any enough to do a merge
    i_match = [];
    for i_blockExist = 1:length(this.blocks{blockType})
        B_existing = this.blocks{blockType}{i_blockExist};
        
        % TODO: Better overlap check that utilizes uncertainty in the poses
        % For now, just check if we're within half a block width of each other
        minDist = min(B_new{i_blockObs}.mindim, B_existing.mindim)/2;
        if compare(B_existing.pose, B_new{i_blockObs}.pose) < minDist
            i_match = [i_match i_blockExist]; %#ok<AGROW>
        end
        
    end % FOR each block of this type
    
    %% Add or Merge Block
    if isempty(i_match)
        % No overlapping blocks of this type, so just add a new one        
        fprintf('Adding a new block with type %d.\n', blockType);
        this.blocks{blockType}{end+1} = B_new{i_blockObs}; 
        
        robot.visibleBlocks{end+1} = this.blocks{blockType}{end};
        robot.visibleFaces{end+1}  = markers2D{1}.faceType;
        
        %this.updateObsBlockPose(blockType, B_new{i_blockObs}.pose);
    else
        if length(i_match) > 1
            warning(['More than one overlapping block found! ' ...
                'Not sure what to do yet, so just using first match.']);
            i_match = i_match(1);
        end
        
        % Merge this new observation into pose of existing block of this type
        B_match = this.blocks{blockType}{i_match};
                
        robot.visibleBlocks{end+1} = this.blocks{blockType}{i_match};
        robot.visibleFaces{end+1}  = markers2D{1}.faceType;
        
        switch(mergeMethod)
            case 'mean'
                B_match.pose = mean(B_new{i_blockObs}.pose, B_match.pose);
            case 'interpolate'
                B_match.pose = interpolate(B_new{i_blockObs}.pose, B_match.pose, 0.5);
            case 'replace'
                B_match.pose = B_new{i_blockObs}.pose;
            otherwise
                error('Unrecognized block merging method "%s".', mergeMethod);
        end % SWITCH(mergeMethod)
        
        %this.updateObsBlockPose(blockType, B_match.pose);
    end
    
end % FOR each observed Block



%% TODO: bookkeeping for indexing (since bundleAdjustment will now be broken)

%{
% Bookkeeping for new block
index = this.blockTypeToIndex.Count + 1;
this.blockTypeToIndex(B_new.blockType) = index;

assert(index <= this.MaxBlocks, 'Index for block too large.');

%this.blocks{index} = B; 
this.blocks = [this.blocks {B_new}];

if ~isempty(this.groundTruthPoseFcn)
    B_true = Block(blockType, this.numMarkers);
    B_true.pose = this.getGroundTruthBlockPose(B_new);
    
    this.groundTruthBlocks = [this.groundTruthBlocks {B_true}];
end

assert(index == this.numBlocks, 'Block indexing problem.');

numMarkersAdded = length(B_new.markers);
assert(B_new.markers{1}.ID == this.numMarkers + 1, 'Marker indexing problem.');
assert(B_new.markers{end}.ID == this.numMarkers + numMarkersAdded, 'Marker indexing problem.');

this.allMarkers3D = [this.allMarkers3D cell(1, numMarkersAdded)];
for i=1:numMarkersAdded
    M = B_new.markers{i};
    this.allMarkers3D{M.ID} = M;
end
%}

end % FUNCTION addMarkerAndBlock()