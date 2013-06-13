function addBlock(this, robot, marker2D)

% Instantiate the block (at canonical location).  This in turn will
% instantiate its faces' 3D markers.
B = Block(marker2D.blockType);
this.blocks{marker2D.blockType} = B; 

marker3D = B.getFaceMarker(marker2D.faceType);

% Figure out where the marker is in 3D space, in camera's world 
% coordinates, which are relative to the robot's frame!
markerFrame = robot.camera.computeExtrinsics(marker2D.corners, marker3D.Pmodel);

% Put the marker into the robot's world frame
markerFrame = robot.frame * markerFrame;

% Now, since the markers are all already in world coordinates, we can use
% that same frame for the block.  And updating the block's frame will also
% update its' face markers:
B.frame = markerFrame;

end % FUNCTION addMarkerAndBlock()