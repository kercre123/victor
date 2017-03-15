using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;
using Anki.Cozmo;

namespace MemoryMatch {
  public class WaitForPlayerGuessMemoryMatchState : CanTimeoutState {

    private MemoryMatchGame _GameInstance;
    private IList<int> _SequenceList;
    private int _CurrentSequenceIndex = 0;
    private int _TargetCube = -1;
    private float _StartLightBlinkTime = -1;
    private float _LastTapRobotTime = 0;

    private enum SubState {
      WaitForInput,
      WaitForBlinkDone,
      WaitForTurnOverAnim,
    }
    private SubState _SubState;

    public override void Enter() {
      base.Enter();
      _SubState = SubState.WaitForInput;
      LightCube.TappedAction += OnBlockTapped;
      _GameInstance = _StateMachine.GetGame() as MemoryMatchGame;
      _SequenceList = _GameInstance.GetCurrentSequence();
      _CurrentRobot.SetHeadAngle(Random.Range(CozmoUtil.kIdealBlockViewHeadValue, 0f));

      _GameInstance.ShowCurrentPlayerTurnStage(PlayerType.Human, false);
      SetTimeoutDuration(_GameInstance.Config.IdleTimeoutSec);

      if (_GameInstance.IsSoloMode()) {
        PlayerInfo cozmoPlayer = _GameInstance.GetFirstPlayerByType(PlayerType.Cozmo);
        if (cozmoPlayer != null) {
          cozmoPlayer.SetGoal(new MemoryMatchPlayerGoalCozmoPositionReset(_GameInstance, null));
        }
      }
    }

    public override void Exit() {
      base.Exit();
      LightCube.TappedAction -= OnBlockTapped;
      if (_CurrentRobot != null) {
        _CurrentRobot.DriveWheels(0f, 0f);
      }
    }

    public override void Update() {
      base.Update();
      if (_SubState == SubState.WaitForBlinkDone) {
        if (Time.time - _StartLightBlinkTime > MemoryMatchGame.kLightBlinkLengthSeconds) {

          // Player is done, if cozmo is correcting himself he needs to react now...
          PlayerInfo cozmoPlayer = _GameInstance.GetFirstPlayerByType(PlayerType.Cozmo);
          if (cozmoPlayer != null && cozmoPlayer.HasGoal()) {
            cozmoPlayer.SetGoal(null);
          }

          if (_CurrentSequenceIndex == _SequenceList.Count) {
            PlayerWinHand();
          }
          else {
            PlayerLoseHand();
          }
        }
      }
    }

    private void HandleOnPlayerWinAnimationDone(bool success) {
      _GameInstance.ShowCenterResult(false);
      _StateMachine.SetNextState(new WaitForNextRoundMemoryMatchState(_GameInstance.IsSoloMode() ? PlayerType.Human : PlayerType.Cozmo, true));
    }

    private void HandleOnPlayerLoseAnimationDone(bool success) {
      _GameInstance.ShowCenterResult(false);
      // Repeat your turn if you have lives left
      if (_GameInstance.GetLivesRemaining(PlayerType.Human) > 0) {
        _StateMachine.SetNextState(new WaitForNextRoundMemoryMatchState(PlayerType.Human));
      }
      else {
        _StateMachine.SetNextState(new WaitForNextRoundMemoryMatchState(_GameInstance.IsSoloMode() ? PlayerType.Human : PlayerType.Cozmo));
      }
    }

    private void PlayerLoseHand() {
      _GameInstance.SetCubeLightsGuessWrong(_SequenceList[_CurrentSequenceIndex], _TargetCube);
      _GameInstance.DecrementLivesRemaining(PlayerType.Human);
      AnimationTrigger trigger = _GameInstance.IsSoloMode() ? AnimationTrigger.MemoryMatchPlayerLoseHandSolo : AnimationTrigger.MemoryMatchPlayerLoseHand;
      _CurrentRobot.SendAnimationTrigger(trigger, HandleOnPlayerLoseAnimationDone);
      _SubState = SubState.WaitForTurnOverAnim;
    }

    private void PlayerWinHand() {
      _GameInstance.SetCubeLightsGuessRight();
      _GameInstance.ShowCenterResult(true, true);
      GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Round_End);
      AnimationTrigger trigger = AnimationTrigger.MemoryMatchPlayerWinHand;
      if (_GameInstance.IsSoloMode()) {
        trigger = AnimationTrigger.MemoryMatchPlayerWinHandSolo;
      }
      else if (_GameInstance.GetCurrentTurnNumber >= _GameInstance.MinRoundBeforeLongReact) {
        trigger = AnimationTrigger.MemoryMatchPlayerWinHandLong;
      }

      _CurrentRobot.SendAnimationTrigger(trigger, HandleOnPlayerWinAnimationDone);
      _GameInstance.AddPoint(true);
      _SubState = SubState.WaitForTurnOverAnim;

      _GameInstance.ShowBanner(LocalizationKeys.kMemoryMatchGameLabelCorrect);
    }

    private void OnBlockTapped(int id, int times, float timeStamp) {
      ResetPlayerInputTimer();
      // Just playing the ending animation
      if (_SubState != SubState.WaitForInput) {
        return;
      }

      // We might be getting doubletaps from the same cube and similar issues
      // if it is an incorrect tap. and our last tap was < _kTapBufferMS previously ignore it.
      // Player has the benifit of the doubt. EnableBlockTapFilter helps with this but still
      // Has enough of an offset where it doesn't completely solve it. This is in robot time not unity time to help ignore the tap filter offset
      // In the world of worse errors, it's better for people to think they're moving to fast and a tap didn't register
      // rather than it being wrong for no reason.
      if (timeStamp < _LastTapRobotTime + _GameInstance.Config.TapBufferRobotTimeMS) {
        DAS.Debug("MemoryMatch.OnBlockTapped.Ignore", "Tapped: " + id + " @ " + timeStamp + " last was " + _LastTapRobotTime);
        DAS.Event("MemoryMatch.OnBlockTapped.TooSoonIgnore", (timeStamp - _LastTapRobotTime).ToString());
        return;
      }

      _LastTapRobotTime = timeStamp;
      _StartLightBlinkTime = Time.time;
      _TargetCube = id;
      if (_SequenceList[_CurrentSequenceIndex] == _TargetCube) {
        GameAudioClient.PostAudioEvent(_GameInstance.GetAudioForBlock(id));
        _CurrentSequenceIndex++;
        if (_CurrentSequenceIndex == _SequenceList.Count) {
          //wants win anim when blink done
          _SubState = SubState.WaitForBlinkDone;
        }
      }
      else {
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_St_Lose);
        // wants lose when blink done. No input
        _SubState = SubState.WaitForBlinkDone;
      }

      _GameInstance.BlinkLight(id, MemoryMatchGame.kLightBlinkLengthSeconds, Color.black, _GameInstance.GetColorForBlock(id));

      // Cancel previous action so we can update to a new one
      _CurrentRobot.CancelAction(RobotActionType.TURN_TOWARDS_OBJECT);
      LightCube cube = _CurrentRobot.LightCubes[_TargetCube];
      _CurrentRobot.TurnTowardsObject(cube, false, MemoryMatchGame.kTurnSpeed_rps, MemoryMatchGame.kTurnAccel_rps2,
                                      null, QueueActionPosition.IN_PARALLEL);
      _CurrentRobot.SendAnimationTrigger(AnimationTrigger.MemoryMatchCozmoFollowTapsSoundOnly, null, QueueActionPosition.IN_PARALLEL);

    }
  }

}
