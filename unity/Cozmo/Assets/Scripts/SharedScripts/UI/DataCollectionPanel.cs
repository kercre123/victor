using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Cozmo.UI;
using Cozmo.Notifications;

public class DataCollectionPanel : MonoBehaviour {

  [SerializeField]
  private GameObject _DataCollectionPanel;

  [SerializeField]
  private CozmoButton _DataCollectionToggleButton;

  [SerializeField]
  private GameObject _DataCollectionIndicator;

  [SerializeField]
  private string _DASEventViewName;

  void Awake() {
    // hide data collection panel, unhide it later if locale wants it
    _DataCollectionPanel.gameObject.SetActive(false);

    // initialize data collection from first time
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow) {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.DataCollectionEnabled = true;
    }
    bool dataCollectionEnabled = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.DataCollectionEnabled;
    // Request Locale gives us ability to get real platform dependent locale
    // whereas unity just gives us the language
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.ResponseLocale>(HandleLocaleResponse);
    RobotEngineManager.Instance.Message.RequestLocale = Singleton<Anki.Cozmo.ExternalInterface.RequestLocale>.Instance;
    RobotEngineManager.Instance.SendMessage();

    if (!dataCollectionEnabled) {
      // by default on so only needs to get set if false.
      SetDataCollection(dataCollectionEnabled);
    }
    // test code for fake german locale
#if UNITY_EDITOR
    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
      if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.FakeGermanLocale) {
        Invoke("ShowDataCollectionPanel", 0.3f);
      }
    }
#endif
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.ResponseLocale>(HandleLocaleResponse);

  }

  public void HandleLocaleResponse(Anki.Cozmo.ExternalInterface.ResponseLocale message) {
    string[] splitString = message.locale.Split(new char[] { '-', '_' });
    // Only german displays the option to opt out.
    if (splitString.Length >= 2) {
      if (splitString[1].ToLower().Equals("de")) {
        ShowDataCollectionPanel();
        return;
      }
    }
    // Our Braze instance is set not send data by default - non-German users switch back to automatic processing, while
    // German users have the option to keep data processing off or turn it on
    NotificationsManager.Instance.SetBrazeRequestProcessingPolicy(true);
  }

  private void ShowDataCollectionPanel() {
    _DataCollectionPanel.gameObject.SetActive(true);
    _DataCollectionToggleButton.Initialize(HandleDataCollectionToggle, "data_collection_toggle", _DASEventViewName);
  }

  private void HandleDataCollectionToggle() {
    SetDataCollection(!DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.DataCollectionEnabled);
  }

  private void SetDataCollection(bool val) {
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.DataCollectionEnabled = val;
    RobotEngineManager.Instance.Message.RequestDataCollectionOption =
              Singleton<Anki.Cozmo.ExternalInterface.RequestDataCollectionOption>.Instance.Initialize(val);
    RobotEngineManager.Instance.SendMessage();
    _DataCollectionIndicator.gameObject.SetActive(val);

    // If DataCollection is enabled, we want Braze to automatically process its data
    NotificationsManager.Instance.SetBrazeRequestProcessingPolicy(
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.DataCollectionEnabled);
  }
}
