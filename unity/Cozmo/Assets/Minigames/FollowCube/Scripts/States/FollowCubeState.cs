using UnityEngine;
using System.Collections;

public class FollowCubeState : State {

  private float distanceMin_ = 100.0f;
  private float distanceMax_ = 150.0f;

  private bool searchTurnRight_ = false;

  public override void Enter() {
    base.Update();
  }

  public override void Update() {
    base.Update();
    FollowClosest();
  }

  void FollowClosest() {
    ObservedObject closest = null;
    float dist = float.MaxValue;
    foreach (ObservedObject obj in robot.VisibleObjects) {
      float d = Vector3.Distance(robot.WorldPosition, obj.WorldPosition);
      if (d < dist) {
        dist = d;
        closest = obj;
      }
    }

    if (closest == null) {
      return;
    }

    float angle = Vector2.Angle(robot.Forward, closest.WorldPosition - robot.WorldPosition);
    if (angle < 10.0f) {
      if (dist > distanceMax_) {
        robot.DriveWheels(20.0f, 20.0f);
      }
      else if (dist < distanceMin_) {
        robot.DriveWheels(-20.0f, -20.0f);
      }
      else {
        robot.DriveWheels(0.0f, 0.0f);
      }
    }
    else {
      // we need to turn to face it
      ComputeTurnDirection(closest);
      if (searchTurnRight_) {
        robot.DriveWheels(25.0f, -20.0f);
      }
      else {
        robot.DriveWheels(-20.0f, 25.0f);
      }
    }

  }

  private void ComputeTurnDirection(ObservedObject followBlock) {
    float turnAngle = Vector3.Cross(robot.Forward, followBlock.WorldPosition - robot.WorldPosition).z;
    if (turnAngle < 0.0f)
      searchTurnRight_ = true;
    else
      searchTurnRight_ = false;
  }
}
