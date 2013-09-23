function computeVirtualBlockPose(this)
% Compute where we want to see a Block to dock with it.

% Figure out where we _want_ to see the block in order to dock with it,
% according to our head pose and the block's docking anchor.
% TODO: update this to use the docking anchor of the correct face, not the marker

assert(~isempty(this.virtualBlock), ...
    'Virtual (docking) block should be set by now.');

if this.isBlockLocked 
    % If we are carrying a block, our desired final location for the lifter
    % is one block height above the docking block's face.
    % TODO: figure out the height adjustment based on block type and dimensions!
    
    % Pose for where we want the block we're carrying to end up when we
    % place it: one block height above the block we are docking with.
    placedBlockFacePose = Pose([0 0 0], [0 0 this.dockingBlock.mindim]);
    placedBlockFacePose.parent = this.dockingFace.pose;
    
    % Figure out the lift angle needed to put the lift end effector in
    % position to leave the block at that pose:
    face_wrt_liftBase = placedBlockFacePose.getWithRespectTo(this.liftBasePose);
    desiredLiftAngle = asin(face_wrt_liftBase.T(3)/this.T_lift(2));
    temp = rodrigues(desiredLiftAngle * [1 0 0]) * this.T_lift;
    
    % Create the corresponding lifter Pose, w.r.t. the robot's lift base:
    desiredLiftPose = Pose([0 0 0], temp);
    desiredLiftPose.parent = this.liftBasePose;
    
    % Figure out the pose for the virtual block position we want to see the
    % docking block in once we are in position to place the carried block
    % on top of it: i.e., one block height below the desired lift pose.
    desiredDockingFacePose = Pose([0 0 0], [0 0 -this.dockingBlock.mindim]);
    desiredDockingFacePose.parent = desiredLiftPose;
    
    % Now get that that pose w.r.t. the robot's position:
    desiredDockingFacePose = desiredDockingFacePose.getWithRespectTo(this.pose);
    
else
    % If not, we figure out the lifter angle we need to simply dock directly 
    % with this block's face.
    face_wrt_liftBase = this.dockingFace.pose.getWithRespectTo(this.liftBasePose);
    desiredLiftAngle = asin(face_wrt_liftBase.T(3)/this.T_lift(2));
    temp = rodrigues(desiredLiftAngle * [1 0 0]) * this.T_lift;
    
    desiredLiftPose = Pose([0 0 0], temp);
    desiredLiftPose.parent = this.liftBasePose;
    desiredDockingFacePose = desiredLiftPose.getWithRespectTo(this.pose);
   
end

this.virtualBlock.pose = this.virtualFace.pose.inv;
this.virtualBlock.pose.parent = desiredDockingFacePose;

end % FUNCTION computeVirtualBlockPose()

    
    
    