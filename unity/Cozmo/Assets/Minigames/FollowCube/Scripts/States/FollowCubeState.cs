using UnityEngine;
using System.Collections;

public class FollowCubeState : State {

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
      robot.DriveWheels(0.0f, 0.0f);
      return;
    }

    float angle = Vector2.Angle(robot.Forward, closest.WorldPosition - robot.WorldPosition);
    if (angle < 5.0f) {

      float speed = (stateMachine_.GetGame() as FollowCubeGame).ForwardSpeed;
      float distMax = (stateMachine_.GetGame() as FollowCubeGame).DistanceMax;
      float distMin = (stateMachine_.GetGame() as FollowCubeGame).DistanceMin;

      if (dist > distMax) {
        robot.DriveWheels(speed, speed);
      }
      else if (dist < distMin) {
        robot.DriveWheels(-speed, -speed);
      }
      else {
        robot.DriveWheels(0.0f, 0.0f);
      }
    }
    else {
      // we need to turn to face it
      ComputeTurnDirection(closest);
      if (searchTurnRight_) {
        robot.DriveWheels(35.0f, -30.0f);
      }
      else {
        robot.DriveWheels(-30.0f, 35.0f);
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
