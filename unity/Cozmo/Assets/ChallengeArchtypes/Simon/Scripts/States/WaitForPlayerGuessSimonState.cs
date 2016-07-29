using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;

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
      _CurrentRobot.DriveWheels(0f, 0f);
    }

    public override void Update() {
      base.Update();
      if (_StartLightBlinkTime < 0 && !_IsAnimating) {
        if (_CurrentSequenceIndex == _SequenceList.Count) {
          PlayerWinRound();
        }
        if (_TargetCube != -1) {
          _CurrentRobot.DriveWheels(0f, 0f);
          if (_SequenceList[_CurrentSequenceIndex] == _TargetCube) {
            _CurrentSequenceIndex++;
          }
          else {
            PlayerLoseGame();
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
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Human));
    }

    private void HandleOnPlayerLoseAnimationDone(bool success) {
      _GameInstance.ShowCenterResult(false);
      if (_GameInstance.GetLivesRemaining(PlayerType.Human) < 0) {
        _GameInstance.FinalLifeComplete(PlayerType.Human);
      }
      else {
        _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Human));
      }
    }

    private void PlayerLoseGame() {
      _GameInstance.SetCubeLightsGuessWrong(_SequenceList[_CurrentSequenceIndex], _TargetCube);
      _GameInstance.ShowCenterResult(true, false);
      _GameInstance.DecrementLivesRemaining(PlayerType.Human);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSimonCozmoWin, HandleOnPlayerLoseAnimationDone);
      _IsAnimating = true;
    }

    private void PlayerWinRound() {
      _GameInstance.SetCubeLightsGuessRight();
      _GameInstance.ShowCenterResult(true, true);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);

      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSimonPlayerHandComplete, HandleOnPlayerWinAnimationDone);
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

      GameAudioClient.PostAudioEvent(_GameInstance.GetAudioForBlock(id));
      _GameInstance.BlinkLight(id, SimonGame.kLightBlinkLengthSeconds, Color.black, _GameInstance.GetColorForBlock(id));
      _TargetCube = id;
      LightCube cube = _CurrentRobot.LightCubes[_TargetCube];
      _CurrentRobot.TurnTowardsObject(cube, false, SimonGame.kTurnSpeed_rps, SimonGame.kTurnAccel_rps2);
    }
  }

}
