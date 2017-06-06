using Anki.Cozmo.ExternalInterface;

namespace Cozmo {
  namespace UI {
    public class HasHiccupsAlertController {
      private AlertModal _HiccupAlertModal = null;

      // Use this for initialization
      public HasHiccupsAlertController() {
        RobotEngineManager.Instance.AddCallback<RobotHiccupsChanged>(HandleRobotHiccupsChanged);
      }

      public void Cleanup() {
        CloseCozmoHasHiccupsAlert();
        RobotEngineManager.Instance.RemoveCallback<RobotHiccupsChanged>(HandleRobotHiccupsChanged);
      }

      public void OpenCozmoHasHiccupsAlert(ModalPriorityData priorityData,
                                           BaseDialog.SimpleBaseDialogHandler dialogCloseAnimationFinishedCallback = null) {
        if (_HiccupAlertModal != null) {
          return;
        }

        var cozmoHasHiccupsData = new AlertModalData("cozmo_has_hiccups_alert",
                                                     LocalizationKeys.kChallengeDetailsCozmoHasHiccupsTitle,
                                                     LocalizationKeys.kChallengeDetailsCozmoHasHiccupsDescription,
                                                     new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                                                     dialogCloseAnimationFinishedCallback: dialogCloseAnimationFinishedCallback);

        UIManager.OpenAlert(cozmoHasHiccupsData,
                            ModalPriorityData.CreateSlightlyHigherData(priorityData),
                            HandleHiccupsAlertCreated);
      }

      private void HandleHiccupsAlertCreated(AlertModal modal) {
        _HiccupAlertModal = modal;
      }

      private void CloseCozmoHasHiccupsAlert() {
        if (_HiccupAlertModal != null) {
          UIManager.CloseModal(_HiccupAlertModal);
          _HiccupAlertModal = null;
        }
      }

      private void HandleRobotHiccupsChanged(RobotHiccupsChanged message) {
        if (!message.hasHiccups) {
          CloseCozmoHasHiccupsAlert();
        }
      }
    }
  }
}
