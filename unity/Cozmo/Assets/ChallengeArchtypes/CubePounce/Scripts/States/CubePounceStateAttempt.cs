using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateAttempt : CubePounceState {

    private float _AttemptDelay_s;
    private float _FirstTimestamp = -1;
    private float _InitialPitch_deg = 0;
    private float _PounceEndTimestamp = -1;

    private bool _AttemptTriggered = false;

    public override void Enter() {
      base.Enter();

      GameAudioClient.SetMusicState(_CubePounceGame.GetDefaultMusicState());
      _AttemptDelay_s = _CubePounceGame.GetAttemptDelay();
      _FirstTimestamp = Time.time;
      _InitialPitch_deg = Mathf.Rad2Deg * _CurrentRobot.PitchAngle;

      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderWaitForPounce);
      _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoWaitForPounce);
    }

    public override void Update() {
      base.Update();
      if (!_AttemptTriggered && (Time.time - _FirstTimestamp) > _AttemptDelay_s) {
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePouncePounce, HandlePounceEnd);
        _AttemptTriggered = true;
      }

      // If cozmos current pitch has differed enough from the beginning of the anim, the block was hit
      float currentPitch_deg = Mathf.Rad2Deg * _CurrentRobot.PitchAngle;
      float angleChange_deg = Mathf.Abs(Mathf.DeltaAngle(_InitialPitch_deg, currentPitch_deg));
      if (angleChange_deg > CubePounceGame.kPouncePitchDiffSuccess_deg) {
        _StateMachine.SetNextState(new CubePounceStatePostPoint(cozmoWon: true));
      }

      if (_AttemptTriggered && _PounceEndTimestamp > 0f && (Time.time - _PounceEndTimestamp) > CubePounceGame.kPounceLiftMeasureDelay_s) {
        // If we got to the end of the post animation delay without triggering the angle diff, cozmo lost
        _StateMachine.SetNextState(new CubePounceStatePostPoint(cozmoWon: false));
        _PounceEndTimestamp = -1f;
      }
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(HandlePounceEnd);
    }

    private void HandlePounceEnd(bool success) {
      _PounceEndTimestamp = Time.time;
    }
  }
}
