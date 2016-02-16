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

    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += OnBlockTapped;
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kSimonGameHeaderMakeYourGuess);
      _GameInstance.SharedMinigameView.ShowInfoTextSlideWithKey(LocalizationKeys.kSimonGameLabelMakeYourGuess);
      _GameInstance.SharedMinigameView.CozmoScoreboard.Dim = true;
      _GameInstance.SharedMinigameView.PlayerScoreboard.Dim = false;
      _SequenceList = _GameInstance.GetCurrentSequence();
      _CurrentRobot.SetHeadAngle(1.0f);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
    }

    public override void Update() {
      base.Update();
      if (_StartLightBlinkTime == -1) {
        if (_CurrentSequenceIndex == _SequenceList.Count) {
          PlayerWinGame();
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
        _CurrentRobot.LightCubes[_TargetCube].SetLEDs(_TargetCubeColor);
      }
    }

    public override void Exit() {
      base.Exit();
      LightCube.TappedAction -= OnBlockTapped;
      _CurrentRobot.DriveWheels(0f, 0f);
    }

    private void HandleOnPlayerWinAnimationDone(bool success) {
      BlackoutLights();
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Human));
    }

    private void HandleOnPlayerLoseAnimationDone(bool success) {
      BlackoutLights();
      _GameInstance.RaiseMiniGameLose(Localization.GetWithArgs(
        LocalizationKeys.kSimonGameTextPatternLength, _SequenceList.Count));
    }

    private void BlackoutLights() {
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetFlashingLEDs(Color.black, uint.MaxValue, 0, 0);
      }
    }

    private void PlayerLoseGame() {
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetFlashingLEDs(Color.red, 100, 100, 0);
      }

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kWin, HandleOnPlayerLoseAnimationDone));
    }

    private void PlayerWinGame() {
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(kvp.Value.Lights[0].OnColor, 0, 100, 100, 0, 0);
      }

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);

      // TODO: Need to find a better animation than shocked; Cozmo should be determined to win 
      // and feel a bit thwarted 
      _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, HandleOnPlayerWinAnimationDone));
    }

    private void OnBlockTapped(int id, int times) {
      _CurrentRobot.SetHeadAngle(Random.Range(CozmoUtil.kIdealBlockViewHeadValue, 0f));
      if (Time.time - _LastTappedTime < 0.4f || _StartLightBlinkTime != -1) {
        return;
      }

      _LastTappedTime = Time.time;
      _StartLightBlinkTime = Time.time;

      SimonGame game = _StateMachine.GetGame() as SimonGame;
      GameAudioClient.PostSFXEvent(game.GetPlayerAudioForBlock(id));

      _TargetCube = id;
      LightCube cube = _CurrentRobot.LightCubes[_TargetCube];
      _TargetCubeColor = cube.Lights[0].OnColor;
      cube.TurnLEDsOff();

      _StateMachine.PushSubState(new CozmoTurnToCubeSimonState(cube, false));
    }
  }

}
