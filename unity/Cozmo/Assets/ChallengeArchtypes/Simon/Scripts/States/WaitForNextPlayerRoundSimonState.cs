using UnityEngine;
using System.Collections;

namespace Simon {
  public class WaitForNextPlayerRoundSimonState : State {

    private SimonGame _GameInstance;

    public override void Enter() {
      base.Enter();
      DAS.Error(this, "Enter");
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      SimonGamePanel continueButton = _GameInstance.ShowHowToPlaySlide("ContinueButton").GetComponent<SimonGamePanel>();
      continueButton.EnableContinueButton(true);
      continueButton.OnContinueButtonPressed += HandleContinuePressed;
    }

    private void HandleContinuePressed() {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MusicGroupStates.PLAYFUL);
      _StateMachine.SetNextState(new PlayerSetSequenceSimonState());
    }
  }
}
