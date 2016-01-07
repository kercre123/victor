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

    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += OnBlockTapped;
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.ShowHowToPlaySlide("CreatePattern");
      _GameInstance.InitColorsAndSounds();
      _CurrentRobot.SetHeadAngle(1.0f);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_POSITIVE_01);
      _TargetSequenceLength = _GameInstance.GetNewSequenceLength(PlayerType.Human);
      _CreatedSequence = new List<int>();
    }

    public override void Update() {
      base.Update();
      if (_TargetSequenceLength == _CreatedSequence.Count) {
        _GameInstance.SetCurrentSequence(_CreatedSequence);
        _StateMachine.SetNextState(new CozmoGuessSimonState());
      }
      if (_TargetCube != -1) {
        _CurrentRobot.DriveWheels(0f, 0f);
        _CurrentRobot.LightCubes[_TargetCube].SetLEDs(_TargetCubeColor);
        _CreatedSequence.Add(_TargetCube);
        _TargetCube = -1;
      }
    }

    public override void Exit() {
      base.Exit();
      LightCube.TappedAction -= OnBlockTapped;
      _CurrentRobot.DriveWheels(0f, 0f);
    }

    private void OnBlockTapped(int id, int times) {
      _CurrentRobot.SetHeadAngle(Random.Range(-0.6f, 0f));
      if (Time.time - _LastTappedTime < 0.8f) {
        return;
      }

      _LastTappedTime = Time.time;
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