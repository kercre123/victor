function blockPose = computeBlockPose(robot, B, markers2D)

assert(iscell(markers2D), 'markers2D should be a cell array.');
assert(all(cellfun(@(M)M.blockType==B.blockType, markers2D)), ...
    sprintf('All markers2D should have the same block type (%d).', ...
    B.blockType));

numMarkers2D = length(markers2D);
corners = cell(1, numMarkers2D);
Pmodel  = cell(1, numMarkers2D);
for i = 1:numMarkers2D
    corners{i} = markers2D{i}.corners;
    marker3D = B.getFaceMarker(markers2D{i}.faceType);
    
    % Get the marker coordinates w.r.t. the Block, whose pose is what we
    % are trying to estimate below.
    Pmodel{i} = marker3D.getPosition(B.pose, 'marker');
end
corners = vertcat(corners{:});
Pmodel  = vertcat(Pmodel{:});

% Figure out where the marker we saw is in 3D space, in camera's  
% coordinate frame:
% (Since the marker's coordinates relative to the block are used here, the
% computed pose will be the blockPose we want)
blockPose = robot.camera.computeExtrinsics(corners, Pmodel);

% Get the block pose in World coordinates using the pose tree:
blockPose = blockPose.getWithRespectTo('World');

% This is not longer needed/true because we are using the the marker's
% position w.r.t. the block above already. 
% % We measured the _marker_ pose.  We are returning the _block_ pose:
% blockPose = markerPose * marker3D.pose.inv;

end % FUNCTION computeBlockPose()
