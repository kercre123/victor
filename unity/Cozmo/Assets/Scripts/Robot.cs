using UnityEngine;
using System.Collections;

public class Robot
{
	public int ID { get; private set; }
	public float headAngle_rad { get; private set; }
	public float poseAngle_rad { get; private set; }
	public float leftWheelSpeed_mmps { get; private set; }
	public float rightWheelSpeed_mmps { get; private set; }
	public float liftHeight_mm { get; private set; }
	public float pose_x { get; private set; }
	public float pose_y { get; private set; }
	public float pose_z { get; private set; }

	public void UpdateInfo( G2U_RobotState message )
	{
		ID = message.ID;
		headAngle_rad = message.headAngle_rad;
		poseAngle_rad = message.poseAngle_rad;
		leftWheelSpeed_mmps = message.leftWheelSpeed_mmps;
		rightWheelSpeed_mmps = message.rightWheelSpeed_mmps;
		liftHeight_mm = message.liftHeight_mm;
		pose_x = message.pose_x;
		pose_y = message.pose_y;
		pose_z = message.pose_z;
	}
}
