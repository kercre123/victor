using Anki.Cozmo.ExternalInterface;

namespace Cozmo {
  namespace UI {
    public class HasHiccupsAlertController {
      private static HasHiccupsAlertController _sInstance;
      public static HasHiccupsAlertController Instance {
        get {
          InitializeInstance();
          return _sInstance;
        }
      }
      public static void InitializeInstance() {
        if (_sInstance == null) {
          _sInstance = new HasHiccupsAlertController();
        }
      }

      private AlertModal _HiccupAlertModal = null;
      private ModalPriorityData _CurrentPriorityData = null;

      // Use this for initialization
      private HasHiccupsAlertController() {
        RobotEngineManager.Instance.AddCallback<RobotHiccupsChanged>(HandleRobotHiccupsChanged);
      }

      public void OpenCozmoHasHiccupsAlert(ModalPriorityData basePriority, AlertModalData overrideAlertModalData = null) {
        if (_HiccupAlertModal != null) {
          return;
        }

        var cozmoHasHiccupsData = overrideAlertModalData ??
          new AlertModalData("cozmo_has_hiccups_alert",
                             LocalizationKeys.kChallengeDetailsCozmoHasHiccupsTitle,
                             LocalizationKeys.kChallengeDetailsCozmoHasHiccupsDescription,
                             new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));

        _CurrentPriorityData = ModalPriorityData.CreateSlightlyHigherData(basePriority);
        UIManager.OpenAlert(cozmoHasHiccupsData,
                            _CurrentPriorityData,
                            HandleHiccupsAlertCreated);
      }

      private void HandleHiccupsAlertCreated(AlertModal modal) {
        _HiccupAlertModal = modal;
      }

      public void CloseCozmoHasHiccupsAlert() {
        if (_HiccupAlertModal != null) {
          UIManager.CloseModal(_HiccupAlertModal);
          _HiccupAlertModal = null;
          _CurrentPriorityData = null;
        }
      }

      private void HandleRobotHiccupsChanged(RobotHiccupsChanged message) {
        if (message.hasHiccups) {
          OpenCozmoHasHiccupsAlert(_CurrentPriorityData ?? new ModalPriorityData());
        }
        else {
          CloseCozmoHasHiccupsAlert();
        }
      }
    }
  }
}
