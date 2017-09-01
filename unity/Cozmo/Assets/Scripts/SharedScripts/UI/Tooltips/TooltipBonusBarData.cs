using UnityEngine;


namespace Cozmo.UI {
  public class TooltipBonusBarData : TooltipGenericBaseDataComponent {

    private UnityEngine.EventSystems.BaseEventData _LastPointerData = null;
    private int _TimeRemainingToNextDay_Sec = 0;

    private void Awake() {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.StarStatus>(HandleStarStatus);
      _LastPointerData = null;
    }

    private void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.StarStatus>(HandleStarStatus);
    }

    private void HandleStarStatus(Anki.Cozmo.ExternalInterface.StarStatus msg) {
      _ShowBodyIndex = msg.completedToday ? 1 : 0;
      _TimeRemainingToNextDay_Sec = msg.timeToNextDay_s;
      base.HandlePointerDown(_LastPointerData);
      _LastPointerData = null;
    }

    protected override string GetBodyString() {
      const int kMinutesPerHour = 60;
      const int kSecondsPerMinute = 60;
      int hours = (_TimeRemainingToNextDay_Sec / kMinutesPerHour / kSecondsPerMinute);
      int minutes = (_TimeRemainingToNextDay_Sec / kMinutesPerHour) - (hours * kSecondsPerMinute);
      return string.Format(base.GetBodyString(), hours, minutes);
    }

    protected override void HandlePointerDown(UnityEngine.EventSystems.BaseEventData data) {
      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        base.HandlePointerDown(data);
      }
      else if (_LastPointerData == null) {
        _LastPointerData = data;
        RobotEngineManager.Instance.Message.GetStarStatus =
                                  Singleton<Anki.Cozmo.ExternalInterface.GetStarStatus>.Instance;
        RobotEngineManager.Instance.SendMessage();
      }
    }
  }

}