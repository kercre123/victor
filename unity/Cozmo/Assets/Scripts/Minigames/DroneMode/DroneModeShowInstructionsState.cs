using UnityEngine;

namespace Cozmo {
  namespace Challenge {
    namespace DroneMode {
      public class DroneModeShowInstructionsState : State {
        private const string idleAnimLock = "DroneModeShowInstructions";


        public override void Enter() {
          GameBase game = _StateMachine.GetGame();
          game.SharedMinigameView.ShowContinueButtonOffset(HandleContinueButtonClicked,
                                                           Localization.Get(LocalizationKeys.kButtonContinue),
                                                           string.Empty,
                                                           Color.clear,
                                                           "drone_mode_instructions_continue_button");

          _CurrentRobot.SetLiftHeight(0);

          // Push neutral idle
          _CurrentRobot.PushIdleAnimation(Anki.Cozmo.AnimationTrigger.DroneModeIdle, idleAnimLock);

          // Show instructions slide
          game.SharedMinigameView.ShowWideAnimationSlide(LocalizationKeys.kDroneModeSafePlayDescription, "drone_mode_safe_play",
                                                         (game as DroneModeGame).DroneModeHowToPlaySlidePrefab,
                                                         null, null);

          game.SharedMinigameView.TitleWidget.SubtitleText = Localization.Get(LocalizationKeys.kDroneModeSafePlayTitle);
        }

        public override void Exit() {
          base.Exit();
        }

        private void HandleContinueButtonClicked() {
          DroneModeDriveCozmoState driveState = new DroneModeDriveCozmoState();
          _StateMachine.SetNextState(driveState);
        }

        public override void Pause(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
          // Don't show the "don't move cozmo" ui

          // DisableInput();
        }
      }
    }
  }
}