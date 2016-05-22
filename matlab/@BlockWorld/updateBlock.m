function updateBlock(this, robot, markers2D)
% Update a block in a BlockWorld, given observed BlockMarker2Ds.

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

B = this.getBlock(blockType);

markerPose = BlockWorld.blockPoseHelper(robot, B, markers2D);

% Now, since the markers are all already in world coordinates, we can use
% that same pose for the block.  And updating the block's pose will also
% update its' face markers:
% TODO: Incorporate uncertainty/weighting into this combination/averaging:
B.pose = mean(markerPose, B.pose);

this.updateBlockObservation(B.blockType, B.pose);

end % FUNCTION addMarkerAndBlock()