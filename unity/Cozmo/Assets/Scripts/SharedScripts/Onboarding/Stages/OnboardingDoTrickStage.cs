using UnityEngine;
using Cozmo.UI;
using System.Collections;

namespace Onboarding {

  public class OnboardingDoTrickStage : OnboardingBaseStage {

    [SerializeField]
    private CozmoText _CubesRequiredLabel;

    private bool _WasSparked = false;

    public override void Start() {
      base.Start();

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(HandleSparkEnded);

      // Most reactionary behaviors are off by default in onboarding so don't worry about hiccups, etc
      // also need to be able to keep looping when hitting okay instead of waiting for input
      StartSparkIfRobotInValidState();

      _CubesRequiredLabel.text = Localization.GetWithArgs(LocalizationKeys.kCoreUpgradeDetailsDialogCubesNeeded,
              1, Cozmo.ItemDataConfig.GetCubeData().GetAmountName(1));
    }


    public override void OnDisable() {
      base.OnDisable();
      StopCoroutine("AttemptSparkRestart");

      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine>(HandleSparkEnded);
    }

    private void StartSparkIfRobotInValidState() {
      IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      if (CurrentRobot != null) {
        if (CurrentRobot.TreadState == Anki.Cozmo.OffTreadsState.OnTreads) {
          StartSpark();
        }
        else {
          var cozmoNotOnTreadsData = new AlertModalData("cozmo_off_treads_alert",
                             LocalizationKeys.kChallengeDetailsCozmoNotOnTreadsTitle,
                             LocalizationKeys.kChallengeDetailsCozmoNotOnTreadsDescription,
                             new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));
          // Usually we wait for a button press to respark again. Since this screen doesn't have a spark button, just go.
          System.Action<AlertModal> errorAlertCreated = (alertModal) => {
            alertModal.DialogClosed += StartSparkIfRobotInValidState;
          };
          ModalPriorityData priorityData = new ModalPriorityData(ModalPriorityLayer.High, 0,
                                                LowPriorityModalAction.CancelSelf, HighPriorityModalAction.Stack);

          // we didn't succeed so let's show an error to the user saying why
          UIManager.OpenAlert(cozmoNotOnTreadsData, priorityData, errorAlertCreated);
        }
      }
    }

    private void StartSpark() {
      IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      if (CurrentRobot != null) {
        _WasSparked = true;
        // switch to freeplay so sparks work
        CurrentRobot.ActivateHighLevelActivity(Anki.Cozmo.HighLevelActivity.Freeplay);
        CurrentRobot.EnableSparkUnlock(Anki.Cozmo.UnlockId.RollCube);
        UnlockableInfo info = UnlockablesManager.Instance.GetUnlockableInfo(Anki.Cozmo.UnlockId.RollCube);
        Anki.AudioMetaData.SwitchState.Sparked sparkedMusicState = info.SparkedMusicState.Sparked;
        if (sparkedMusicState == Anki.AudioMetaData.SwitchState.Sparked.Invalid) {
          sparkedMusicState = SparkedMusicStateWrapper.DefaultState().Sparked;
        }
        RobotEngineManager.Instance.CurrentRobot.SetSparkedMusicState(sparkedMusicState);
      }
    }

    protected override void HandlePauseStateChanged(bool isPaused) {
      base.HandlePauseStateChanged(isPaused);
      if (!isPaused && _WasSparked) {
        StartCoroutine("AttemptSparkRestart");
      }
    }

    private IEnumerator AttemptSparkRestart() {
      // Wait a bit for engine to wake up
      yield return new WaitForSeconds(0.05f);
      StartSparkIfRobotInValidState();
    }

    private void HandleSparkEnded(Anki.Cozmo.ExternalInterface.HardSparkEndedByEngine message) {
      // In the event we were backgrounded, make sure cozmo goes back to sitting around quietly as soon as done.
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.ActivateHighLevelActivity(Anki.Cozmo.HighLevelActivity.Selection);
        robot.ExecuteBehaviorByID(Anki.Cozmo.BehaviorID.Wait);
      }

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Freeplay);
      // we don't want to stop them in the event they got a broken cozmo and it cant work.
      // so success or fail just move on.
      OnboardingManager.Instance.GoToNextStage();
      // just get the users used to having their sparks taken away but don't do it if they quit the app before completion
      // as they will have to go through this phase again.
      int sparkCost = (int)Anki.Cozmo.EnumConcept.GetSparkCosts(Anki.Cozmo.SparkableThings.DoATrick, 1);
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.RemoveItemAmount(
                      RewardedActionManager.Instance.SparkID, sparkCost);
    }

    public override void SkipPressed() {
      base.SkipPressed();
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.StopSparkUnlock();
      }
    }
  }

}
