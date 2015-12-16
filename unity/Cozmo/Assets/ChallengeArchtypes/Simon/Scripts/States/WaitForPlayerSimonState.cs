using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;

namespace Simon {
  public class WaitForPlayerSimonState : State {

    private SimonGame _GameInstance;
    private IList<int> _SequenceList;
    private int _CurrentSequenceIndex = 0;

    public override void Enter() {
      base.Enter();
      LightCube.TappedAction += OnBlockTapped;
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _SequenceList = _GameInstance.GetCurrentSequence();
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

    private void HandleOnAnimationDone(bool success) {
      _StateMachine.SetNextState(new CozmoSetSimonState());
    }

    private void LoseGame() {
      AnimationState animation = new AnimationState();
      animation.Initialize(AnimationName.kShocked, HandleOnAnimationDone);
      _StateMachine.SetNextState(animation);
    }

    private void WinGame() {
      AnimationState animation = new AnimationState();
      animation.Initialize(AnimationName.kMajorWin, HandleOnAnimationDone);
      _StateMachine.SetNextState(animation);
    }

    private void OnBlockTapped(int id, int times) {
      AudioClient.Instance.PostEvent(Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_CLICK_GENERAL, Anki.Cozmo.Audio.GameObjectType.Default);
      if (_SequenceList[_CurrentSequenceIndex] != id) {
        LoseGame();
      }
      _CurrentSequenceIndex++;
    }
  }

}
