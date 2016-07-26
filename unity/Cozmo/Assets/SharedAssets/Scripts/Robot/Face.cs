using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

public class Face { // TODO Implement IHaveCameraPosition
  public long ID { get; private set; }

  public uint RobotID { get; private set; }

  public Vector3 WorldPosition { get; private set; }

  public Quaternion Rotation { get; private set; }

  public Vector3 Forward { get { return Rotation * Vector3.right; } }

  public Vector3 Right { get { return Rotation * -Vector3.up; } }

  public Vector3 Up { get { return Rotation * Vector3.forward; } }

  public Face(long faceId, float world_x, float world_y, float world_z) {
    ID = faceId;
    WorldPosition = new Vector3(world_x, world_y, world_z);
  }

  public Face(G2U.RobotObservedFace message) {
    UpdateInfo(message);
  }

  public static implicit operator long(Face faceObject) {
    if (faceObject == null)
      return -1;

    return faceObject.ID;
  }

  public void UpdateInfo(G2U.RobotObservedFace message) {
    RobotID = message.robotID;
    ID = message.faceID;

    Vector3 newPos = new Vector3(message.pose.x, message.pose.y, message.pose.z);

    //dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
    WorldPosition = newPos;
    Rotation = new Quaternion(message.pose.q1, message.pose.q2, message.pose.q3, message.pose.q0);
    // Size = Vector3.one * CozmoUtil.kBlockLengthMM;

    // TopFaceNorthAngle = message.topFaceOrientation_rad + Mathf.PI * 0.5f;

    //if (message.markersVisible > 0)
    //  TimeLastSeen = Time.time;
  }


  // this should only be called in response to RobotChangedObservedFaceID
  public void UpdateFaceID(int id) {
    ID = id;
  }

}
