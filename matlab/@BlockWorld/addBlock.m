function addBlock(this, robot, markers2D)
% Add a block to a BlockWorld, given observed BlockMarker2Ds.

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

% Instantiate the block (at canonical location).  This in turn will
% instantiate its faces' 3D markers.
B = Block(blockType, this.numMarkers);
index = this.blockTypeToIndex.Count + 1;
this.blockTypeToIndex(B.blockType) = index;

assert(index <= this.MaxBlocks, 'Index for block too large.');

%this.blocks{index} = B; 
this.blocks = [this.blocks {B}];

if ~isempty(this.groundTruthPoseFcn)
    B_true = Block(blockType, this.numMarkers);
    B_true.pose = this.getGroundTruthBlockPose(B);
    
    this.groundTruthBlocks = [this.groundTruthBlocks {B_true}];
end

assert(index == this.numBlocks, 'Block indexing problem.');

numMarkersAdded = length(B.markers);
assert(B.markers{1}.ID == this.numMarkers + 1, 'Marker indexing problem.');
assert(B.markers{end}.ID == this.numMarkers + numMarkersAdded, 'Marker indexing problem.');

this.allMarkers3D = [this.allMarkers3D cell(1, numMarkersAdded)];
for i=1:numMarkersAdded
    M = B.markers{i};
    this.allMarkers3D{M.ID} = M;
end

markerPose = BlockWorld.blockPoseHelper(robot, B, markers2D);

% Now, since the markers are all already in world coordinates, we can use
% that same pose for the block.  And updating the block's pose will also
% update its' face markers:
B.pose = markerPose;

this.updateBlockObservation(B.blockType, B.pose);

end % FUNCTION addMarkerAndBlock()