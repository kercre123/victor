using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;

namespace Simon {
  public class WaitForPlayerSimonState : State {

    private SimonGame _GameInstance;
    private IList<int> _SequenceList;
    private int _CurrentSequenceIndex = 0;
    private float _LastTappedTime;

    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += OnBlockTapped;
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.ShowHowToPlaySlide("RepeatPattern");
      _SequenceList = _GameInstance.GetCurrentSequence();
      _CurrentRobot.SetHeadAngle(1.0f);
    }

    public override void Update() {
      base.Update();
      if (_CurrentSequenceIndex == _SequenceList.Count) {
        WinGame();
      }
    }

    public override void Exit() {
      base.Exit();
      LightCube.TappedAction -= OnBlockTapped;
    }

    private void HandleOnWinAnimationDone(bool success) {
      BlackoutLights();
      if (_SequenceList.Count == _GameInstance.MaxSequenceLength) {
        _GameInstance.RaiseMiniGameWin();
        return;
      }
      _StateMachine.SetNextState(new WaitForNextTurnState(_SequenceList.Count + 1));
    }

    private void HandleOnLoseAnimationDone(bool success) {
      BlackoutLights();
      if (!_GameInstance.TryDecrementAttempts()) {
        _GameInstance.RaiseMiniGameLose();
        return;
      }
      _StateMachine.SetNextState(new WaitForNextTurnState(_SequenceList.Count));
    }

    private void BlackoutLights() {
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetFlashingLEDs(Color.black, uint.MaxValue, 0, 0);
      }
    }

    private void LoseGame() {
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetFlashingLEDs(Color.red, 100, 100, 0);
      }

      _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, HandleOnLoseAnimationDone));
    }

    private void WinGame() {
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(kvp.Value.Lights[0].OnColor, 0, 100, 100, 0, 0);
      }
      _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorWin, HandleOnWinAnimationDone));
    }

    private void OnBlockTapped(int id, int times) {
      if (Time.time - _LastTappedTime < 0.8f) {
        return;
      }

      _LastTappedTime = Time.time;
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_CLICK_GENERAL);
      if (_SequenceList[_CurrentSequenceIndex] != id) {
        LoseGame();
      }
      _CurrentSequenceIndex++;
    }
  }

}
