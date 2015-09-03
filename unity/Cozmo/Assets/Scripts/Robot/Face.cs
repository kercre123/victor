using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

public class Face
{
	public long ID { get; private set; }

	public uint RobotID { get; private set; }

	public Vector3 WorldPosition { get; private set; }

	public Quaternion Rotation { get; private set; }

	public Vector3 Forward { get { return Rotation * Vector3.right; } }

	public Vector3 Right { get { return Rotation * -Vector3.up; } }

	public Vector3 Up { get { return Rotation * Vector3.forward; } }

	public Face (long faceId)
	{
		ID = faceId;
	}

	public static implicit operator long (Face faceObject)
	{
		if (faceObject == null)
			return -1;

		return faceObject.ID;
	}

	public void UpdateInfo (G2U.RobotObservedFace message)
	{
		RobotID = message.robotID;

		Vector3 newPos = new Vector3 (message.world_x, message.world_y, message.world_z);

		//dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
		WorldPosition = newPos;
		Rotation = new Quaternion (message.quaternion1, message.quaternion2, message.quaternion3, message.quaternion0);
		// Size = Vector3.one * CozmoUtil.BLOCK_LENGTH_MM;

		// TopFaceNorthAngle = message.topFaceOrientation_rad + Mathf.PI * 0.5f;

		//if (message.markersVisible > 0)
		//  TimeLastSeen = Time.time;
	}

}
