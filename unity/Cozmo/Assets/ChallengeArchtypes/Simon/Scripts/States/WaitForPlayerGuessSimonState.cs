using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;
using Anki.Cozmo;

namespace Simon {
  public class WaitForPlayerGuessSimonState : State {

    private SimonGame _GameInstance;
    private IList<int> _SequenceList;
    private int _CurrentSequenceIndex = 0;
    private float _LastTappedTime;
    private int _TargetCube = -1;
    private uint _TargetCubeColor;
    private float _StartLightBlinkTime = -1;
    private const float _kTapBufferSeconds = 0.1f;
    private bool _IsAnimating = false;

    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += OnBlockTapped;
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _SequenceList = _GameInstance.GetCurrentSequence();
      _CurrentRobot.SetHeadAngle(1.0f);
      // add delay before allowing player taps because cozmo can accidentally tap when setting pattern.
      _LastTappedTime = Time.time;

      _GameInstance.ShowCurrentPlayerTurnStage(PlayerType.Human, false);
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
      if (_StartLightBlinkTime < 0 && !_IsAnimating) {
        if (_CurrentSequenceIndex == _SequenceList.Count) {
          PlayerWinHand();
        }
        if (_TargetCube != -1) {
          _CurrentRobot.DriveWheels(0f, 0f);
          if (_SequenceList[_CurrentSequenceIndex] == _TargetCube) {
            _CurrentSequenceIndex++;
          }
          else {
            PlayerLoseHand();
          }
          _TargetCube = -1;
        }
      }
      else if (Time.time - _StartLightBlinkTime > SimonGame.kLightBlinkLengthSeconds) {
        _StartLightBlinkTime = -1;
      }
    }

    private void HandleOnPlayerWinAnimationDone(bool success) {
      _GameInstance.ShowCenterResult(false);
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(_GameInstance.IsSoloMode() ? PlayerType.Human : PlayerType.Cozmo));
    }

    private void HandleOnPlayerLoseAnimationDone(bool success) {
      _GameInstance.ShowCenterResult(false);
      // Repeat your turn if you have lives left
      if (_GameInstance.GetLivesRemaining(PlayerType.Human) > 0) {
        _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Human));
      }
      else {
        _StateMachine.SetNextState(new WaitForNextRoundSimonState(_GameInstance.IsSoloMode() ? PlayerType.Human : PlayerType.Cozmo));
      }
    }

    private void PlayerLoseHand() {
      _GameInstance.SetCubeLightsGuessWrong(_SequenceList[_CurrentSequenceIndex], _TargetCube);
      _GameInstance.ShowCenterResult(true, false);
      _GameInstance.DecrementLivesRemaining(PlayerType.Human);
      GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      AnimationTrigger trigger = _GameInstance.IsSoloMode() ? AnimationTrigger.MemoryMatchPlayerLoseHandSolo : AnimationTrigger.MemoryMatchPlayerLoseHand;
      _CurrentRobot.SendAnimationTrigger(trigger, HandleOnPlayerLoseAnimationDone);
      _IsAnimating = true;
    }

    private void PlayerWinHand() {
      _GameInstance.SetCubeLightsGuessRight();
      _GameInstance.ShowCenterResult(true, true);
      GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);

      AnimationTrigger trigger = _GameInstance.IsSoloMode() ? AnimationTrigger.MemoryMatchPlayerWinHandSolo : AnimationTrigger.MemoryMatchPlayerWinHand;
      _CurrentRobot.SendAnimationTrigger(trigger, HandleOnPlayerWinAnimationDone);
      _GameInstance.AddPoint(true);
      _IsAnimating = true;

      _GameInstance.ShowBanner(LocalizationKeys.kSimonGameLabelCorrect);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Round_End);
    }

    private void OnBlockTapped(int id, int times, float timeStamp) {
      if (Time.time - _LastTappedTime < _kTapBufferSeconds || _StartLightBlinkTime >= 0) {
        return;
      }

      // Just playing the ending animation
      if (_IsAnimating) {
        return;
      }

      _CurrentRobot.SetHeadAngle(Random.Range(CozmoUtil.kIdealBlockViewHeadValue, 0f));
      _LastTappedTime = Time.time;
      _StartLightBlinkTime = Time.time;
      _TargetCube = id;
      if (_SequenceList[_CurrentSequenceIndex] == _TargetCube) {
        GameAudioClient.PostAudioEvent(_GameInstance.GetAudioForBlock(id));
      }
      else {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Lose);
      }
      _GameInstance.BlinkLight(id, SimonGame.kLightBlinkLengthSeconds, Color.black, _GameInstance.GetColorForBlock(id));

      LightCube cube = _CurrentRobot.LightCubes[_TargetCube];
      _CurrentRobot.TurnTowardsObject(cube, false, SimonGame.kTurnSpeed_rps, SimonGame.kTurnAccel_rps2);
    }
  }

}
