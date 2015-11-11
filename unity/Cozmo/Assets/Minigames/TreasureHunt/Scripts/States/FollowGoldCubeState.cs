using UnityEngine;
using System.Collections;

namespace TreasureHunt {

  public class FollowGoldCubeState : State {

    private bool _SearchTurnRight = false;
    float _LastTimeSeenGoldBlock = 0.0f;

    public override void Enter() {
      base.Update();
      _LastTimeSeenGoldBlock = Time.time;
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
    }

    public override void Update() {
      base.Update();

      if (Time.time - _LastTimeSeenGoldBlock > 5.0f) {
        _StateMachine.SetNextState(new LookForGoldCubeState());
      }

      if (HasGoldBlockInView()) {
        _LastTimeSeenGoldBlock = Time.time;
        ObservedObject followingCube = FollowClosest();
        if (followingCube != null) {
          TreasureHuntGame treasureHuntGame = (_StateMachine.GetGame() as TreasureHuntGame);
          treasureHuntGame.ClearBlockLights();
          if (treasureHuntGame.HoveringOverGold(followingCube as LightCube)) {
            treasureHuntGame.SetHoveringLight(followingCube as LightCube);
            _StateMachine.SetNextState(new CelebrateGoldState());
          }
          else {
            treasureHuntGame.SetDirectionalLight(followingCube as LightCube);
          }
        }
      }
      else {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }
    }

    private bool HasGoldBlockInView() {
      foreach (ObservedObject obj in _CurrentRobot.VisibleObjects) {
        if (obj is LightCube) {
          return true;
        }
      }
      return false;
    }

    private ObservedObject FollowClosest() {
      ObservedObject closest = null;
      float dist = float.MaxValue;
      foreach (ObservedObject obj in _CurrentRobot.VisibleObjects) {
        if (obj is LightCube) {
          float d = Vector3.Distance(_CurrentRobot.WorldPosition, obj.WorldPosition);
          if (d < dist) {
            dist = d;
            closest = obj;
          }
        }
      }

      if (closest == null) {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
        return closest;
      }

      float angle = Vector2.Angle(_CurrentRobot.Forward, closest.WorldPosition - _CurrentRobot.WorldPosition);
      if (angle < 10.0f) {

        float speed = 60.0f;
        float distMax = 150.0f;
        float distMin = 90.0f;

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

      return closest;

    }

    private void ComputeTurnDirection(ObservedObject followBlock) {
      float turnAngle = Vector3.Cross(_CurrentRobot.Forward, followBlock.WorldPosition - _CurrentRobot.WorldPosition).z;
      if (turnAngle < 0.0f)
        _SearchTurnRight = true;
      else
        _SearchTurnRight = false;
    }

  }

}
