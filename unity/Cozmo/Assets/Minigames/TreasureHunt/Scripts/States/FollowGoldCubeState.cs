using UnityEngine;
using System.Collections;

public class FollowGoldCubeState : State {

  private bool _SearchTurnRight = false;
  float _LastTimeSeenGoldBlock = 0.0f;

  public override void Enter() {
    base.Update();
    _LastTimeSeenGoldBlock = Time.time;
    robot.SetLiftHeight(0.0f);
    robot.SetHeadAngle(-1.0f);
  }

  public override void Update() {
    base.Update();

    if (Time.time - _LastTimeSeenGoldBlock > 5.0f) {
      stateMachine_.SetNextState(new LookForGoldCubeState());
    }

    if (HasGoldBlockInView()) {
      _LastTimeSeenGoldBlock = Time.time;
      ObservedObject followingCube = FollowClosest();
      if (followingCube != null) {
        if ((stateMachine_.GetGame() as TreasureHuntGame).HoveringOverGold((LightCube)followingCube)) {
          stateMachine_.SetNextState(new CelebrateGoldState());
        }
      }
    }
    else {
      robot.DriveWheels(0.0f, 0.0f);
    }
  }

  private bool HasGoldBlockInView() {
    foreach (ObservedObject obj in robot.VisibleObjects) {
      if (obj is LightCube) {
        return true;
      }
    }
    return false;
  }

  private ObservedObject FollowClosest() {
    ObservedObject closest = null;
    float dist = float.MaxValue;
    foreach (ObservedObject obj in robot.VisibleObjects) {
      if (obj is LightCube) {
        float d = Vector3.Distance(robot.WorldPosition, obj.WorldPosition);
        if (d < dist) {
          dist = d;
          closest = obj;
        }
      }
    }

    if (closest == null) {
      robot.DriveWheels(0.0f, 0.0f);
      return closest;
    }

    float angle = Vector2.Angle(robot.Forward, closest.WorldPosition - robot.WorldPosition);
    if (angle < 10.0f) {

      float speed = 60.0f;
      float distMax = 150.0f;
      float distMin = 90.0f;

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
      if (_SearchTurnRight) {
        robot.DriveWheels(35.0f, -30.0f);
      }
      else {
        robot.DriveWheels(-30.0f, 35.0f);
      }
    }

    return closest;

  }

  private void ComputeTurnDirection(ObservedObject followBlock) {
    float turnAngle = Vector3.Cross(robot.Forward, followBlock.WorldPosition - robot.WorldPosition).z;
    if (turnAngle < 0.0f)
      _SearchTurnRight = true;
    else
      _SearchTurnRight = false;
  }

}
