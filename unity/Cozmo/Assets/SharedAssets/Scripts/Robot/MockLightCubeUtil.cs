using System;
using Anki.Cozmo.ExternalInterface;
using UnityEngine;

public static class MockLightCubeUtil {

  public static RobotObservedObject CurrentState(this ObservedObject cube) {
    return Singleton<RobotObservedObject>.Instance.Initialize(cube.RobotID,
      0,
      cube.Family,
      cube.ObjectType,
      cube.ID,
      cube.VizRect.x,
      cube.VizRect.y,
      cube.VizRect.width,
      cube.VizRect.height,
      cube.WorldPosition.x,
      cube.WorldPosition.y,
      cube.WorldPosition.z,
      cube.Rotation.w,
      cube.Rotation.x,
      cube.Rotation.y,
      cube.Rotation.z,
      cube.TopFaceNorthAngle,
      (byte)(cube.MarkersVisible ? 1 : 0),
      (byte)(cube.IsActive ? 1 : 0));
  }

  public static RobotObservedObject SetRotation(this RobotObservedObject obj, Quaternion rotation) {
    obj.quaternion_x = rotation.x;
    obj.quaternion_y = rotation.y;
    obj.quaternion_z = rotation.z;
    obj.quaternion_w = rotation.w;
    return obj;
  }

  public static Quaternion GetRotation(this RobotObservedObject obj) {
    return new Quaternion(obj.quaternion_x, obj.quaternion_y, obj.quaternion_z, obj.quaternion_w);
  }

  public static RobotObservedObject SetPosition(this RobotObservedObject obj, Vector3 worldPosition) {
    obj.world_x = worldPosition.x;
    obj.world_y = worldPosition.y;
    obj.world_z = worldPosition.z;
    return obj;
  }
  public static Vector3 GetPosition(this RobotObservedObject obj) {
    return new Vector3(obj.world_x, obj.world_y, obj.world_z);
  }

  public static RobotObservedObject Rotate(this RobotObservedObject obj, Quaternion rotation) {
    var q = obj.GetRotation();
    q *= rotation;
    obj.SetRotation(q);
    return obj;
  }

  public static RobotObservedObject Move(this RobotObservedObject obj, Vector3 position) {
    var p = obj.GetPosition();
    p += position;
    obj.SetPosition(p);
    return obj;
  }

  public static void Apply(this RobotObservedObject msg, ObservedObject obj) {
    obj.UpdateInfo(msg);
  }

  public static void Rotate(this ObservedObject obj, Quaternion rotation) {
    obj.CurrentState().Rotate(rotation).Apply(obj);
  }

  public static void Move(this ObservedObject obj, Vector3 position) {
    obj.CurrentState().Move(position).Apply(obj);
  }

  public static void MoveAndRotate(this ObservedObject obj, Vector3 position, Quaternion rotation) {
    obj.CurrentState().Move(position).Rotate(rotation).Apply(obj);
  }

  public static void SetRotation(this ObservedObject obj, Quaternion rotation) {
    obj.CurrentState().SetRotation(rotation).Apply(obj);
  }

  public static void SetPosition(this ObservedObject obj, Vector3 position) {
    obj.CurrentState().SetPosition(position).Apply(obj);
  }
}

