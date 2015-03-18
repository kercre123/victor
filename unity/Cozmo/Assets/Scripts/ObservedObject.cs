using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class ObservedObject
{
	public uint RobotID { get; private set; }
	public uint Family { get; private set; }
	public uint ObjectType { get; private set; }
	public int ID { get; private set; }
	public Rect VizRect { get; private set; }
	public Vector3 WorldPosition { get; private set; }
	public Quaternion Rotation { get; private set; }

	public void UpdateInfo( G2U_RobotObservedObject message )
	{
		RobotID = message.robotID;
		Family = message.objectFamily;
		ObjectType = message.objectType;
		ID = message.objectID;
		VizRect = new Rect(message.img_topLeft_x, message.img_topLeft_y, message.img_width, message.img_height);

		//dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
		WorldPosition = new Vector3(message.world_x, message.world_y, message.world_z);

		Rotation = new Quaternion(message.quaternion0, message.quaternion1, message.quaternion2, message.quaternion3);
	}
}
