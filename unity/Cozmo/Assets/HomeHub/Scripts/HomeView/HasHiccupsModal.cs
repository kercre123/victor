using UnityEngine;
using System.Collections;
using Anki.Cozmo.ExternalInterface;

namespace Cozmo {
  namespace UI {
    public class HasHiccupsModal : MonoBehaviour {

      private AlertModal _HiccupAlertModal = null;

      // Use this for initialization
      void Start() {
        RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotHiccupsChanged>(HandleRobotHiccupsChanged);
      }

      private void OnDestroy() {
        CloseCozmoHasHiccupsAlert();
        RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotHiccupsChanged>(HandleRobotHiccupsChanged);
      }

      public void OpenCozmoHasHiccupsAlert(ModalPriorityData priorityData, BaseDialog.SimpleBaseDialogHandler dialogCloseAnimationFinishedCallback = null) {
        _HiccupAlertModal = null;

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

      private void HandleRobotHiccupsChanged(Anki.Cozmo.ExternalInterface.RobotHiccupsChanged message) {
        if (!message.hasHiccups) {
          CloseCozmoHasHiccupsAlert();
        }
      }
    }
  }
}
