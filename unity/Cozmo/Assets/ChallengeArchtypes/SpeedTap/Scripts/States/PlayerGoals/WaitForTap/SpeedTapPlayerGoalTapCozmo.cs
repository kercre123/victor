using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;

namespace SpeedTap {
  public class SpeedTapPlayerGoalTapCozmo : PlayerGoalBase {

    private float _CozmoMovementDelay_sec;
    private float _StartTimestamp_sec;
    private bool _IsCozmoMoving;

    private bool _IsMatching;
    private float _CozmoMistakeChance;
    private float _CozmoFakeoutChance;

    public SpeedTapPlayerGoalTapCozmo(float CozmoMovementDelay_sec, bool isMatching, float CozmoMistakeChance, float CozmoFakeoutChance) {

      _IsCozmoMoving = false;
      _StartTimestamp_sec = Time.time;
      _IsMatching = isMatching;
      _CozmoMovementDelay_sec = CozmoMovementDelay_sec;
      _CozmoMistakeChance = CozmoMistakeChance;
      _CozmoFakeoutChance = CozmoFakeoutChance;
    }

    public override void Enter() {
      base.Enter();

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.AnimationEvent>(OnRobotAnimationEvent);
    }
    public override void Exit() {
      base.Exit();

      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.AnimationEvent>(OnRobotAnimationEvent);
    }
    public override void Update() {
      base.Update();
      float secondsElapsed = Time.time - _StartTimestamp_sec;
      if (!_IsCozmoMoving && secondsElapsed > _CozmoMovementDelay_sec) {
        _IsCozmoMoving = true;
        DoCozmoMovement();
      }
    }

    private void DoCozmoMovement() {
      if (_IsMatching) {
        _CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnSpeedtapTap);
      }
      else {
        // Mistake or fakeout or nothing?
        float randomPercent = Random.Range(0f, 1f);
        if (randomPercent < _CozmoMistakeChance) {
          _CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnSpeedtapTap);
        }
        else {
          randomPercent -= _CozmoMistakeChance;
          if (randomPercent < _CozmoFakeoutChance) {
            _CurrentRobot.SendAnimationTrigger(AnimationTrigger.OnSpeedtapFakeout);
          }
        }
      }
    }

    private void OnRobotAnimationEvent(Anki.Cozmo.ExternalInterface.AnimationEvent animEvent) {
      SpeedTapPlayerInfo speedTapPlayerInfo = (SpeedTapPlayerInfo)_ParentPlayer;
      if (animEvent.event_id == AnimEvent.TAPPED_BLOCK) {
        if (animEvent.timestamp < speedTapPlayerInfo.LastTapTimeStamp || speedTapPlayerInfo.LastTapTimeStamp < 0) {
          speedTapPlayerInfo.LastTapTimeStamp = animEvent.timestamp;
        }
      }

    }

  }
}