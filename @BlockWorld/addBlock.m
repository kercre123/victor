function addBlock(this, robot, markers2D)

if ~iscell(markers2D)
    markers2D = {markers2D};
end

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

assert(index == this.numBlocks, 'Block indexing problem.');

numMarkersAdded = length(B.markers);
assert(B.markers{1}.ID == this.numMarkers + 1, 'Marker indexing problem.');
assert(B.markers{end}.ID == this.numMarkers + numMarkersAdded, 'Marker indexing problem.');

this.allMarkers3D = [this.allMarkers3D cell(1, numMarkersAdded)];
for i=1:numMarkersAdded
    M = B.markers{i};
    this.allMarkers3D{M.ID} = M;
end

corners = cell(1, numMarkers2D);
Pmodel  = cell(1, numMarkers2D);
for i = 1:numMarkers2D
    corners{i} = markers2D{i}.corners;
    marker3D = B.getFaceMarker(markers2D{i}.faceType);
    Pmodel{i} = marker3D.model;
end
corners = vertcat(corners{:});
Pmodel  = vertcat(Pmodel{:});

% Figure out where the marker we saw is in 3D space, in camera's world 
% coordinates, which are relative to the robot's pose!
markerPose = robot.camera.computeExtrinsics(corners, Pmodel);

% Put the marker into the robot's world pose
markerPose = robot.pose * markerPose;

% Now, since the markers are all already in world coordinates, we can use
% that same frame for the block.  And updating the block's frame will also
% update its' face markers:
B.pose = markerPose;

end % FUNCTION addMarkerAndBlock()