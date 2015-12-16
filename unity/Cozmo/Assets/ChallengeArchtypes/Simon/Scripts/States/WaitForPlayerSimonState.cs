using UnityEngine;
using System.Collections;
using System.Collections.Generic;

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
      if (_SequenceList[_CurrentSequenceIndex] != id) {
        LoseGame();
      }
      _CurrentSequenceIndex++;
    }
  }

}
