using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;

namespace Simon {
  public class PlayerSetSequenceSimonState : State {

    private SimonGame _GameInstance;
    private List<int> _CreatedSequence;
    private int _TargetSequenceLength;
    private float _LastTappedTime;
    private int _TargetCube = -1;
    private uint _TargetCubeColor;
    private float _StartLightBlinkTime = -1;

    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += OnBlockTapped;
      _CreatedSequence = new List<int>();

      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _TargetSequenceLength = _GameInstance.GetNewSequenceLength(PlayerType.Human);

      _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kSimonGameHeaderCreateYourPattern);
      _GameInstance.UpdateSequenceText(LocalizationKeys.kSimonGameLabelCreateYourPattern,
        0, _TargetSequenceLength);

      _GameInstance.SharedMinigameView.CozmoScoreboard.Dim = true;
      _GameInstance.SharedMinigameView.PlayerScoreboard.Dim = false;

      _CurrentRobot.SetHeadAngle(1.0f);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GenericEvent.PLAY_SFX_UI_POSITIVE_01);
    }

    public override void Update() {
      base.Update();
      if (_StartLightBlinkTime == -1) {
        if (_TargetSequenceLength == _CreatedSequence.Count) {
          _GameInstance.SetCurrentSequence(_CreatedSequence);
          _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, HandleOnCozmoStartAnimationDone));
        }
        if (_TargetCube != -1) {
          _CurrentRobot.DriveWheels(0f, 0f);
          _CreatedSequence.Add(_TargetCube);
          _TargetCube = -1;
          _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kSimonGameHeaderCreateYourPattern);
          _GameInstance.UpdateSequenceText(LocalizationKeys.kSimonGameLabelCreateYourPattern,
            _CreatedSequence.Count, _TargetSequenceLength);
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

    private void OnBlockTapped(int id, int times) {
      _CurrentRobot.SetHeadAngle(Random.Range(-0.6f, 0f));
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

    private void HandleOnCozmoStartAnimationDone(bool success) {
      _StateMachine.SetNextState(new CozmoGuessSimonState());
    }
  }
}