using UnityEngine;
using Anki.Cozmo.Audio;
using Anki.Cozmo.ExternalInterface;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateAttempt : CubePounceState {

    private float _InitialPitch_deg = 0;
    private bool _UseClosePounce = false;

    public CubePounceStateAttempt(bool useClosePounce = false) {
      _UseClosePounce = useClosePounce;
    }

    public override void Enter() {
      base.Enter();

      _InitialPitch_deg = Mathf.Rad2Deg * _CurrentRobot.PitchAngle;

      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderWaitForPounce);
      _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoWaitForPounce);

      if (!_UseClosePounce) {
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePouncePounce_0, HandlePounceEnd);
      }
      else {
        TriggerClosePounce();
      }
    }

    public override void Update() {
      base.Update();

      // If cozmos current pitch has differed enough from the beginning of the anim, the block was hit
      float currentPitch_deg = Mathf.Rad2Deg * _CurrentRobot.PitchAngle;
      float angleChange_deg = Mathf.Abs(Mathf.DeltaAngle(_InitialPitch_deg, currentPitch_deg));
      if (angleChange_deg > _CubePounceGame.GameConfig.PouncePitchDiffSuccess_deg) {
        _StateMachine.SetNextState(new CubePounceStatePostPoint(cozmoWon: true));
      }
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(HandlePounceEnd);
    }

    private void HandlePounceEnd(bool success) {
      // If we got to the end of the post animation delay without triggering the angle diff, cozmo lost
      _StateMachine.SetNextState(new CubePounceStatePostPoint(cozmoWon: false));
    }

    private void TriggerClosePounce() {

      RobotActionUnion[] actions = new RobotActionUnion[3];
      actions[0] = new RobotActionUnion().Initialize(Singleton<DriveStraight>.Instance.Initialize(500, 15, false));
      actions[1] = new RobotActionUnion().Initialize(new SetLiftHeight(
        height_mm: CozmoUtil.kMinLiftHeightMM,
        accel_rad_per_sec2: 10f,
        max_speed_rad_per_sec: 20f,
        duration_sec: 0f));

      actions[2] = new RobotActionUnion().Initialize(new SetLiftHeight(
        height_mm: CozmoUtil.kMaxLiftHeightMM,
        accel_rad_per_sec2: 10f,
        max_speed_rad_per_sec: 20f,
        duration_sec: 0f));

      _CurrentRobot.SendQueueCompoundAction(actions, HandlePounceEnd);
    }
  }
}
