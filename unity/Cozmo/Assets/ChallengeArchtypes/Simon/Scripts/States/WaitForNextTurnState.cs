using UnityEngine;
using System.Collections;

namespace Simon {
  public class WaitForNextTurnState : State {

    private SimonGame _GameInstance;
    private int _SequenceCount;

    public WaitForNextTurnState(int sequenceCount) {
      _SequenceCount = sequenceCount;
    }

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      SimonGamePanel continueButton = _GameInstance.ShowHowToPlaySlide("ContinueButton").GetComponent<SimonGamePanel>();
      continueButton.EnableContinueButton(true);
      continueButton.OnContinueButtonPressed += HandleContinuePressed;
    }

    private void HandleContinuePressed() {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MusicGroupStates.PLAYFUL);
      _StateMachine.SetNextState(new CozmoSetSimonState(_SequenceCount));
    }

    public override void Exit() {
      base.Exit();
    }

  }
}