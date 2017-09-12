using Anki.Cozmo.ExternalInterface;
using Cozmo.Notifications;
using Cozmo.Util;
using UnityEngine;
using UnityEngine.UI;
using System.Linq;
using System;

public class NotificationsPane : MonoBehaviour {

  [SerializeField]
  private Dropdown _NotificationTypeDropDown;

  [SerializeField]
  private InputField _FutureSecondsInput;
  [SerializeField]
  private Button _FutureSecondsApplyButton;

  void Start() {
    _FutureSecondsApplyButton.onClick.AddListener(HandleScheduleButtonPressed);
    _FutureSecondsInput.text = "2";

    _NotificationTypeDropDown.ClearOptions();

    RobotEngineManager.Instance.AddCallback<NotificationTextKeys>(HandleTextKeysFromEngine);
    RobotEngineManager.Instance.Message.RequestNotificationTextKeys = new RequestNotificationTextKeys();
    RobotEngineManager.Instance.SendMessage();

    var enabled = FeatureGate.Instance.IsFeatureEnabled(Anki.Cozmo.FeatureType.LocalNotifications);
    _FutureSecondsApplyButton.enabled = enabled;
  }

  void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<NotificationTextKeys>(HandleTextKeysFromEngine);
  }

  private void HandleTextKeysFromEngine(NotificationTextKeys msg) {
    _NotificationTypeDropDown.ClearOptions();
    var textKeysList = msg.textKeys.ToList();
    _NotificationTypeDropDown.AddOptions(textKeysList);
  }

  private void HandleScheduleButtonPressed() {
    int numSeconds = 0;
    if (Int32.TryParse(_FutureSecondsInput.text, out numSeconds)) {
      string textKey = _NotificationTypeDropDown.options[_NotificationTypeDropDown.value].text;
      const bool persist = true;
      CacheNotificationToSchedule msg = new CacheNotificationToSchedule(numSeconds, textKey, persist);
      NotificationsManager.Instance.HandleCacheNotificationToSchedule(msg);
    }
  }
}
