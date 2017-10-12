using Anki.Cozmo.ExternalInterface;
using Anki.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Settings {
  public class SettingsNotificationsPanel : MonoBehaviour {
    [SerializeField]
    private CozmoButton _YesButton;

    private void Awake() {
      string dasEventViewName = "settings_notification_panel";
      _YesButton.Initialize(HandleWantsNotficationButtonTapped, "settings_notification_button", dasEventViewName);
    }

    private void Start() {
      // deactivate if notification prompt already asked...
      UpdateShownState();
    }


    private void HandleWantsNotficationButtonTapped() {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.OSNotificationsPermissionsPromptShown = true;
      Notifications.NotificationsManager.Instance.InitNotificationsIfAllowed();
      UpdateShownState();
    }

    private void UpdateShownState() {
      if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.OSNotificationsPermissionsPromptShown) {
        gameObject.SetActive(false);
      }
    }

  }
}
