
namespace Cozmo.UI {
  [System.Flags]
  public enum ChallengeEdgeCases {
    CheckForCubes = 0,
    CheckForDriveOffCharger = 1,
    CheckForDizzy = 2,
    CheckForOnTreads = 4,
    CheckForHiccups = 8,
    CheckForOS = 16
  }

  public class ChallengeStartEdgeCaseAlertController {
    private ModalPriorityData _BasePriority;
    private ChallengeEdgeCases _EdgeCaseBitfield;
    private HasHiccupsAlertController _HasHiccupsAlertController;

    public ChallengeStartEdgeCaseAlertController(ModalPriorityData basePriority, ChallengeEdgeCases checkEdgeCaseBitfield) {
      _BasePriority = basePriority;
      _EdgeCaseBitfield = checkEdgeCaseBitfield;
      _HasHiccupsAlertController = new HasHiccupsAlertController();
    }

    public bool ShowEdgeCaseAlertIfNeeded(string challengeTitleLocKey,
                                           int numCubesRequired,
                                           bool isOSSupported,
                                           string requiredOs) {
      bool edgeCaseFound = true;
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        int currentNumCubes = robot.LightCubes.Count;
        if (CheckEdgeCase(ChallengeEdgeCases.CheckForCubes) && currentNumCubes < numCubesRequired) {
          NeedCubesAlertHelper.OpenNeedCubesAlert(currentNumCubes,
                                                  numCubesRequired,
                                                  Localization.Get(challengeTitleLocKey),
                                                  _BasePriority);
        }
        else if (CheckEdgeCase(ChallengeEdgeCases.CheckForDriveOffCharger)
                 && robot.CurrentBehaviorClass == Anki.Cozmo.BehaviorClass.DriveOffCharger) {
          OpenCozmoNotReadyAlert();
        }
        else if (CheckEdgeCase(ChallengeEdgeCases.CheckForDizzy)
                 && robot.CurrentBehaviorClass == Anki.Cozmo.BehaviorClass.ReactToRobotShaken) {
          OpenCozmoDizzyAlert();
        }
        else if (CheckEdgeCase(ChallengeEdgeCases.CheckForOnTreads)
                 && robot.TreadState != Anki.Cozmo.OffTreadsState.OnTreads) {
          OpenCozmoNotOnTreadsAlert();
        }
        else if (CheckEdgeCase(ChallengeEdgeCases.CheckForHiccups) && robot.HasHiccups) {
          _HasHiccupsAlertController.OpenCozmoHasHiccupsAlert(_BasePriority);
        }
        else if (CheckEdgeCase(ChallengeEdgeCases.CheckForOS) && !isOSSupported) {
          OpenOsNotSupportedAlert(requiredOs);
        }
        else {
          edgeCaseFound = false;
        }
      }

      return edgeCaseFound;
    }

    private bool CheckEdgeCase(ChallengeEdgeCases edgeCase) {
      return (_EdgeCaseBitfield & edgeCase) == edgeCase;
    }

    public void CleanUp() {
      if (_HasHiccupsAlertController != null) {
        _HasHiccupsAlertController.Cleanup();
      }
    }

    // Cozmo isn't done driving off the charger.
    private void OpenCozmoNotReadyAlert() {
      var cozmoNotReadyData = new AlertModalData("cozmo_driving_off_charger_alert",
                    LocalizationKeys.kChallengeDetailsCozmoIsStillWakingUpModalTitle,
                    LocalizationKeys.kChallengeDetailsCozmoIsStillWakingUpModalDescription,
                    new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                    timeoutSec: 10.0f);

      UIManager.OpenAlert(cozmoNotReadyData, ModalPriorityData.CreateSlightlyHigherData(_BasePriority));
    }

    private void OpenCozmoDizzyAlert() {
      var cozmoDizzyData = new AlertModalData("cozmo_dizzy_alert",
                   LocalizationKeys.kChallengeDetailsCozmoDizzyTitle,
                   LocalizationKeys.kChallengeDetailsCozmoDizzyDescription,
                   new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));

      UIManager.OpenAlert(cozmoDizzyData, ModalPriorityData.CreateSlightlyHigherData(_BasePriority));
    }

    private void OpenCozmoNotOnTreadsAlert() {
      var cozmoNotOnTreadsData = new AlertModalData("cozmo_off_treads_alert",
                   LocalizationKeys.kChallengeDetailsCozmoNotOnTreadsTitle,
                   LocalizationKeys.kChallengeDetailsCozmoNotOnTreadsDescription,
                   new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));

      UIManager.OpenAlert(cozmoNotOnTreadsData, ModalPriorityData.CreateSlightlyHigherData(_BasePriority));
    }

    private void OpenOsNotSupportedAlert(string requiredOs) {
      AlertModalData osNotSupportedAlertData = new AlertModalData("os_not_supported",
                   LocalizationKeys.kUnlockableOSNotSupportedTitle,
                   LocalizationKeys.kUnlockableOSNotSupportedDescription,
                   new AlertModalButtonData("os_not_supported_okay_button", LocalizationKeys.kButtonOkay),
                   showCloseButton: true,
                   descLocArgs: new object[] { requiredOs });

      UIManager.OpenAlert(osNotSupportedAlertData, ModalPriorityData.CreateSlightlyHigherData(_BasePriority));
    }
  }
}