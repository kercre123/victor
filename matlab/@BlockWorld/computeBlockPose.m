function markerPose = computeBlockPose(robot, B, markers2D)

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

% Figure out where the marker we saw is in 3D space, in camera's world 
% coordinates, which are relative to the robot's current pose!
markerPose = robot.camera.computeExtrinsics(corners, Pmodel);

% Put the marker into the robot's world pose
markerPose = robot.pose * markerPose;

end