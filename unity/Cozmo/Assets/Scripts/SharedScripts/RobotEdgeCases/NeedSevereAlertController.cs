
using Anki.Cozmo;
using Cozmo.Needs;

namespace Cozmo.UI {
  public class NeedSevereAlertController {
    public delegate void NeedSevereAlertClosed();
    public event NeedSevereAlertClosed OnNeedSevereAlertClosed;

    private bool _AllowAlert;
    public bool AllowAlert {
      get { return _AllowAlert; }
      set {
        if (_AllowAlert != value) {
          _AllowAlert = value;
          if (_AllowAlert) {
            NeedsValue latestValue = NeedsStateManager.Instance.PeekLatestEngineValue(NeedId.Repair);
            if (latestValue.Bracket == NeedBracketId.Critical) {
              OpenNeedSevereAlert(NeedId.Repair);
            }
            else {
              latestValue = NeedsStateManager.Instance.PeekLatestEngineValue(NeedId.Energy);
              if (latestValue.Bracket == NeedBracketId.Critical) {
                OpenNeedSevereAlert(NeedId.Energy);
              }
            }
          }
        }
      }
    }

    private ModalPriorityData _NeedSevereAlertPriority;

    public NeedSevereAlertController(ModalPriorityData needSevereAlertPriority) {
      _NeedSevereAlertPriority = needSevereAlertPriority;

      NeedsStateManager.Instance.OnNeedsBracketChanged += HandleNeedBracketChanged;
    }

    public void CleanUp() {
      NeedsStateManager.Instance.OnNeedsBracketChanged -= HandleNeedBracketChanged;
    }

    private void HandleNeedBracketChanged(NeedsActionId action, NeedId needThatChanged) {
      if (AllowAlert && (needThatChanged == NeedId.Repair || needThatChanged == NeedId.Energy)) {
        NeedsValue latestValue = NeedsStateManager.Instance.PeekLatestEngineValue(needThatChanged);
        if (latestValue.Bracket == NeedBracketId.Critical) {
          OpenNeedSevereAlert(needThatChanged);
        }
      }
    }

    private void OpenNeedSevereAlert(NeedId severeNeed) {
#if UNITY_EDITOR
      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        return;
      }
#endif

      AlertModalButtonData buttonData = new AlertModalButtonData("severe_okay_button",
                                     LocalizationKeys.kButtonOkay,
                                     clickCallback: HandleSevereAlertClosed);

      if (severeNeed == NeedId.Repair) {
        AlertModalData alertData = new AlertModalData("repair_severe_alert",
                          LocalizationKeys.kNeedsSevereAlertRepairTitle,
                          LocalizationKeys.kNeedsSevereAlertRepairDescription,
                          buttonData);
        UIManager.OpenAlert(alertData, _NeedSevereAlertPriority, overrideCloseOnTouchOutside: false);
      }
      else if (severeNeed == NeedId.Energy) {
        AlertModalData alertData = new AlertModalData("energy_severe_alert",
                          LocalizationKeys.kNeedsSevereAlertEnergyTitle,
                          LocalizationKeys.kNeedsSevereAlertEnergyDescription,
                          buttonData);
        UIManager.OpenAlert(alertData, _NeedSevereAlertPriority, overrideCloseOnTouchOutside: false);
      }
    }

    private void HandleSevereAlertClosed() {
      if (OnNeedSevereAlertClosed != null) {
        OnNeedSevereAlertClosed();
      }
    }
  }
}