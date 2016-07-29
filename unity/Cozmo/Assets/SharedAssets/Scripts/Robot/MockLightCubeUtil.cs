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
      new Anki.PoseStruct3d(cube.WorldPosition.x,
        cube.WorldPosition.y,
        cube.WorldPosition.z,
        cube.Rotation.w,
        cube.Rotation.x,
        cube.Rotation.y,
        cube.Rotation.z, 
        0),
      cube.TopFaceNorthAngle,
      (byte)(cube.MarkersVisible ? 1 : 0),
      (byte)(cube.HasLights ? 1 : 0));
  }

  public static RobotObservedObject SetRotation(this RobotObservedObject obj, Quaternion rotation) {
    obj.pose.q1 = rotation.x;
    obj.pose.q2 = rotation.y;
    obj.pose.q3 = rotation.z;
    obj.pose.q0 = rotation.w;
    return obj;
  }

  public static Quaternion GetRotation(this RobotObservedObject obj) {
    return new Quaternion(obj.pose.q1, obj.pose.q2, obj.pose.q3, obj.pose.q0);
  }

  public static RobotObservedObject SetPosition(this RobotObservedObject obj, Vector3 worldPosition) {
    obj.pose.x = worldPosition.x;
    obj.pose.y = worldPosition.y;
    obj.pose.z = worldPosition.z;
    return obj;
  }

  public static Vector3 GetPosition(this RobotObservedObject obj) {
    return new Vector3(obj.pose.x, obj.pose.y, obj.pose.z);
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

  public static RobotObservedObject MakeVisible(this RobotObservedObject obj, bool visible) {
    obj.markersVisible = (byte)(visible ? 1 : 0);
    return obj;
  }

  public static RobotObservedObject MakeActive(this RobotObservedObject obj, bool active) {
    obj.isActive = (byte)(active ? 1 : 0);
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

  public static void MakeActiveAndVisible(this ObservedObject obj, bool active, bool visible) {
    obj.CurrentState().MakeActive(active).MakeVisible(visible).Apply(obj);
  }

}

