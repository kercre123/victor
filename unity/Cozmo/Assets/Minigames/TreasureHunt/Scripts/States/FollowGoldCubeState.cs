using UnityEngine;
using System.Collections;

public class FollowGoldCubeState : State {

  private bool searchTurnRight_ = false;
  float lastTimeSeenGoldBlock_ = 0.0f;

  public override void Enter() {
    base.Update();
    lastTimeSeenGoldBlock_ = Time.time;
  }

  public override void Update() {
    base.Update();

    if (Time.time - lastTimeSeenGoldBlock_ > 5.0f) {
      stateMachine_.SetNextState(new LookForGoldCubeState());
    }

    if (HasGoldBlockInView()) {
      lastTimeSeenGoldBlock_ = Time.time;
      ObservedObject followingCube = FollowClosest();
      if (followingCube != null) {
        if ((stateMachine_.GetGame() as TreasureHuntGame).HoveringOverGold((LightCube)followingCube)) {
          stateMachine_.SetNextState(new CelebrateGoldState());
        }
      }

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
    if (angle < 5.0f) {

      float speed = 60.0f;
      float distMax = 130.0f;
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
      if (searchTurnRight_) {
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
      searchTurnRight_ = true;
    else
      searchTurnRight_ = false;
  }

}
