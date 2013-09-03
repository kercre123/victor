function blockPose = computeBlockPose(robot, B, markers2D)

assert(iscell(markers2D), 'markers2D should be a cell array.');

numMarkers2D = length(markers2D);
corners = cell(1, numMarkers2D);
Pmodel  = cell(1, numMarkers2D);
for i = 1:numMarkers2D
    corners{i} = markers2D{i}.corners;
    marker3D = B.getFaceMarker(markers2D{i}.faceType);
    Pmodel{i} = marker3D.model;
end
corners = vertcat(corners{:});
Pmodel  = vertcat(Pmodel{:});

% Figure out where the marker we saw is in 3D space, in camera's  
% coordinate frame:
markerPose = robot.camera.computeExtrinsics(corners, Pmodel);

% Get the marker pose in World coordinates using the pose tree:
markerPose = markerPose.getWithRespectTo('World');

% We measured the _marker_ pose.  We are returning the _block_ pose:
blockPose = markerPose * marker3D.pose.inv;

end % FUNCTION computeBlockPose()
