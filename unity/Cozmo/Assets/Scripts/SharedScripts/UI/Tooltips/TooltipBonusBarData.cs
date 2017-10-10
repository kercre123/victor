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

    // The tool tip glint will only disable after 2 taps in the same session
    // Once the app restarts, if the user tapped at least once in the previous session,
    // the glint will NOT be re-enabled
    private float _NumToolTipTaps = 0;

    private bool _ToolTipTriggered = false;

    protected override void Start() {
      base.Start();
      // Get tool tip status from DataPersistenceManager
      PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
      if (profile != null) {
        _ToolTipTriggered = profile.HasTriggeredBonusBarToolTip;
        // Set initial states
        if (_ToolTipNotifIconImage != null) {
          _ToolTipNotifIconImage.sprite = _ToolTipTriggered ? _ToolTipNotifOffSprite : _ToolTipNotifOnSprite;
        }

        if (_ToolTipNotifIconGlint != null) {
          _ToolTipNotifIconGlint.EnableGlint(!_ToolTipTriggered);
        }
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

      _NumToolTipTaps++;
      UpdateToolTipIcon();
    }

    private void UpdateToolTipIcon() {
      // Turns the tool tip icon "off" after the first use
      if (!_ToolTipTriggered) {
        _ToolTipTriggered = true;
        // Save status for future app runs
        DataPersistenceManager.Instance.Data.DefaultProfile.HasTriggeredBonusBarToolTip = _ToolTipTriggered;
        DataPersistenceManager.Instance.Save();

        if (_ToolTipNotifIconImage != null) {
          _ToolTipNotifIconImage.sprite = _ToolTipNotifOffSprite;
        }
      }

      if (_NumToolTipTaps >= 2) { // but only disable the glint after the second tap in the session
        if (_ToolTipNotifIconGlint != null) {
          _ToolTipNotifIconGlint.EnableGlint(!_ToolTipTriggered);
        }
      }
    }
  }
}