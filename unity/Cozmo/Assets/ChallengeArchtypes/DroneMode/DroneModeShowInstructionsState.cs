using UnityEngine;

namespace Cozmo {
  namespace Minigame {
    namespace DroneMode {
      public class DroneModeShowInstructionsState : State {
        public override void Enter() {
          GameBase game = _StateMachine.GetGame();
          game.SharedMinigameView.ShowContinueButtonOffset(HandleContinueButtonClicked,
                                                           Localization.Get(LocalizationKeys.kButtonContinue),
                                                           string.Empty,
                                                           Color.clear,
                                                           "drone_mode_instructions_continue_button");

          _CurrentRobot.SetLiftHeight(0);

          // TODO: Set idle animation? (body allowed)

          // TODO: Show instructions slide
        }

        private void HandleContinueButtonClicked() {
          DroneModeDriveCozmoState driveState = new DroneModeDriveCozmoState();
          _StateMachine.SetNextState(driveState);
        }
      }
    }
  }
}