using UnityEngine;
using System.Collections;

public class FollowCubeState : State {

  private bool _SearchTurnRight = false;

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
    foreach (ObservedObject obj in _CurrentRobot.VisibleObjects) {
      float d = Vector3.Distance(_CurrentRobot.WorldPosition, obj.WorldPosition);
      if (d < dist) {
        dist = d;
        closest = obj;
      }
    }

    if (closest == null) {
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      return;
    }

    float angle = Vector2.Angle(_CurrentRobot.Forward, closest.WorldPosition - _CurrentRobot.WorldPosition);
    if (angle < 10.0f) {

      float speed = (_StateMachine.GetGame() as FollowCubeGame).ForwardSpeed;
      float distMax = (_StateMachine.GetGame() as FollowCubeGame).DistanceMax;
      float distMin = (_StateMachine.GetGame() as FollowCubeGame).DistanceMin;

      if (dist > distMax) {
        _CurrentRobot.DriveWheels(speed, speed);
      }
      else if (dist < distMin) {
        _CurrentRobot.DriveWheels(-speed, -speed);
      }
      else {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }
    }
    else {
      // we need to turn to face it
      ComputeTurnDirection(closest);
      if (_SearchTurnRight) {
        _CurrentRobot.DriveWheels(35.0f, -30.0f);
      }
      else {
        _CurrentRobot.DriveWheels(-30.0f, 35.0f);
      }
    }

  }

  private void ComputeTurnDirection(ObservedObject followBlock) {
    float turnAngle = Vector3.Cross(_CurrentRobot.Forward, followBlock.WorldPosition - _CurrentRobot.WorldPosition).z;
    if (turnAngle < 0.0f)
      _SearchTurnRight = true;
    else
      _SearchTurnRight = false;
  }
}
