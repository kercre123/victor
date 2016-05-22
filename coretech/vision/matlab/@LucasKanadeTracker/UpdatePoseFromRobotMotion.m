function UpdatePoseFromRobotMotion(this, robotTx, robotTy, robotTheta, headAngle)
% Update the tracker's

assert(strcmp(this.tformType, 'planar6dof'), ...
    'Tracker should be planar6dof to use function UpdateFromRobotMotion().');

Rpitch = rodrigues(-headAngle*[0 1 0]);
                                  
P_blockRelHead = Pose(this.rotationMatrix, [this.tx; this.ty; this.tz]);

P_headRelNeck  = Pose(Rpitch * this.headRotation, Rpitch * this.headPosition); 
P_neckRelRobot = Pose([0 0 0], this.neckPosition);
P_headRelRobot = P_neckRelRobot * P_headRelNeck;

P_robot2RelRobot1 = Pose(robotTheta*[0 0 1], [robotTx robotTy 0]');

P_blockRelHead_new = P_headRelRobot.inv * P_robot2RelRobot1.inv * P_headRelRobot * P_blockRelHead;

this.UpdatePose(P_blockRelHead_new);

end % UpdatePoseFromRobotMotion()