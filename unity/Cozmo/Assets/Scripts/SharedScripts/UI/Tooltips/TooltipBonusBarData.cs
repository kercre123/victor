using DataPersistence;
using UnityEngine;


namespace Cozmo.UI {
  public class TooltipBonusBarData : TooltipGenericBaseDataComponent {

    private UnityEngine.EventSystems.BaseEventData _LastPointerData = null;
    private int _TimeRemainingToNextDay_Sec = 0;

    [SerializeField]
    private CozmoImage _ToolTipNotifIconImage;

    [SerializeField]
    private Sprite _ToolTipNotifOffSprite;

    [SerializeField]
    private Sprite _ToolTipNotifOnSprite;

    [SerializeField]
    private AnkiAnimateGlint _ToolTipNotifIconGlint;

    // To account for possible accidental taps, the notif icon won't disable after the first tap
    private const int _kMaxNumNotifTaps = 3;
    private int _NumNotifTaps = 0;

    protected override void Start() {
      base.Start();
      // Get tool tip status from DataPersistenceManager
      PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
      if (profile != null) {
        _NumNotifTaps = profile.NumTimesToolTipNotifTapped;
        UpdateToolTipIcon();
      }
    }

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

      _NumNotifTaps++;
      // Save status for future app runs
      DataPersistenceManager.Instance.Data.DefaultProfile.NumTimesToolTipNotifTapped = _NumNotifTaps;
      DataPersistenceManager.Instance.Save();

      UpdateToolTipIcon();
    }

    private void UpdateToolTipIcon() {
      if (_ToolTipNotifIconImage != null) {
        _ToolTipNotifIconImage.sprite = (_NumNotifTaps >= _kMaxNumNotifTaps) ? _ToolTipNotifOffSprite : _ToolTipNotifOnSprite;
      }

      if (_ToolTipNotifIconGlint != null) {
        _ToolTipNotifIconGlint.EnableGlint(!(_NumNotifTaps >= _kMaxNumNotifTaps));
      }
    }
  }
}