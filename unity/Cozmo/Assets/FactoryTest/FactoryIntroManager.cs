using UnityEngine;
using System.Collections;
using System.Collections.Generic;

#if FACTORY_TEST

using Anki.Cozmo.ExternalInterface;

public class FactoryIntroManager : MonoBehaviour {

  public const int kRobotID = 1;
  public const string kEngineIP = "127.0.0.1";
  public const string kRobotSimIP = "127.0.0.1";
  public const string kRobotIP = "172.31.1.1";

  [SerializeField]
  private UnityEngine.UI.Image _Background;

  [SerializeField]
  private UnityEngine.UI.Image _RestartOverlay;

  [SerializeField]
  private UnityEngine.UI.Button _OptionsButton;

  [SerializeField]
  private UnityEngine.UI.Button _RestartButton;

  [SerializeField]
  private UnityEngine.UI.Button _StartButton;

  [SerializeField]
  private UnityEngine.UI.Text _NetworkStatusText;

  [SerializeField]
  private UnityEngine.UI.Text _PingStatusText;

  [SerializeField]
  private FactoryOptionsPanel _FactoryOptionsPanelPrefab;
  private FactoryOptionsPanel _FactoryOptionsPanelInstance;

  [SerializeField]
  private UnityEngine.UI.Text _StatusText;

  [SerializeField]
  private Canvas _Canvas;

  [SerializeField]
  private UnityEngine.UI.Image _InProgressSpinner;

  private PingStatus _PingStatusComponent;
  private bool _IsSim = false;

  void Start() {
    _PingStatusComponent = GetComponent<PingStatus>();

    DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.SOSLoggerEnabled = true;

    SetStatusText("Not Connected");

    RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToClient;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
    
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(HandleRobotConnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.FactoryTestResultEntry>(HandleFactoryResult);

    _RestartButton.gameObject.SetActive(false);

    _RestartButton.onClick.AddListener(HandleRestartButtonClick);
    _OptionsButton.onClick.AddListener(HandleOptionsButtonClick);
    _StartButton.onClick.AddListener(HandleStartButtonClick);

    _InProgressSpinner.gameObject.SetActive(false);

    RobotEngineManager.Instance.Connect(kEngineIP);
  }

  private void HandleSetSimType(bool isSim) {
    _IsSim = isSim;
  }

  private void HandleStartButtonClick() {
    _StartButton.gameObject.SetActive(false);
    _RestartButton.gameObject.SetActive(true);
    _InProgressSpinner.gameObject.SetActive(true);

    SetStatusText("Connecting To Robot");
    RobotEngineManager.Instance.ConnectToRobot(kRobotID, _IsSim ? kRobotSimIP : kRobotIP, _IsSim);
  }

  private void HandleConnectedToClient(string connectionIdentifier) {
    RobotEngineManager.Instance.StartEngine();

  }

  private void HandleRobotConnected(RobotConnectionResponse message) {
    SetStatusText("Connected To Robot");

    RobotEngineManager.Instance.SetDebugConsoleVar("BFT_EnableNVStorageWrites", PlayerPrefs.GetInt("EnableNStorageWritesToggle", 1).ToString());
    RobotEngineManager.Instance.SetDebugConsoleVar("BFT_CheckPrevFixtureResults", PlayerPrefs.GetInt("CheckPreviousResult", 0).ToString());
    RobotEngineManager.Instance.SetDebugConsoleVar("BFT_WipeNVStorage", PlayerPrefs.GetInt("WipeNVStorageAtStart", 0).ToString());
    RobotEngineManager.Instance.SetDebugConsoleVar("BFT_SkipBlockPickup", PlayerPrefs.GetInt("SkipBlockPickup", 0).ToString());
    RobotEngineManager.Instance.SetDebugConsoleVar("BFT_ConnectToRobotOnly", PlayerPrefs.GetInt("ConnectToRobotOnly", 0).ToString());
    RobotEngineManager.Instance.SetDebugConsoleVar("BFT_DisconnectAtEnd", "1");

    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues();

    // Send the volume message
    bool soundEnabled = PlayerPrefs.GetInt("EnableRobotSound", 1) == 1;
    RobotEngineManager.Instance.Message.SetRobotVolume = new SetRobotVolume();
    RobotEngineManager.Instance.Message.SetRobotVolume.volume = soundEnabled ? 1.0f : 0;
    RobotEngineManager.Instance.SendMessage();

    // runs the factory test.
    RobotEngineManager.Instance.CurrentRobot.WakeUp(true);
    RobotEngineManager.Instance.CurrentRobot.EnableReactionaryBehaviors(false);
    RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
    RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
    RobotEngineManager.Instance.CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.FactoryTest);

    _RestartButton.gameObject.SetActive(true);
  }

  private void HandleOptionsButtonClick() {
    if (_FactoryOptionsPanelInstance != null) {
      GameObject.Destroy(_FactoryOptionsPanelInstance);
    }
    _FactoryOptionsPanelInstance = GameObject.Instantiate(_FactoryOptionsPanelPrefab).GetComponent<FactoryOptionsPanel>();
    _FactoryOptionsPanelInstance.transform.SetParent(_Canvas.transform, false);
    _FactoryOptionsPanelInstance.OnSetSim += HandleSetSimType;
    _FactoryOptionsPanelInstance.OnOTAStarted += HandleOTAStarted;
    _FactoryOptionsPanelInstance.OnOTAFinished += HandleOTAFinished;

    _FactoryOptionsPanelInstance.Initialize(_IsSim, _Canvas, _PingStatusComponent);
  }

  private void HandleOTAStarted() {
    RobotEngineManager.Instance.ConnectedToClient -= HandleConnectedToClient;
    RobotEngineManager.Instance.DisconnectedFromClient -= HandleDisconnectedFromClient;

    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(HandleRobotConnected);
  }

  private void HandleOTAFinished() {
    RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToClient;
    RobotEngineManager.Instance.DisconnectedFromClient += HandleDisconnectedFromClient;
    
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(HandleRobotConnected);
  }

  private void SetStatusText(string txt) {
    if (_StatusText != null) {
      _StatusText.text = txt;
    }
  }

  private void HandleDisconnectedFromClient(DisconnectionReason obj) {
    // Don't override result code with disconnect text since the disconnect
    // is expected when the test completes
    DAS.Debug(this, "Disconnected: " + obj.ToString());
    if (!_StatusText.text.Contains("Result Code:")) {
      SetStatusText("Disconnected: " + obj.ToString());
      TestFailed();
    }
  }

  private void TestFailed() {
    if (_Background == null) {
      return; // quit case
    }
    _Background.color = Color.red;
    _InProgressSpinner.gameObject.SetActive(false);
    _RestartButton.gameObject.SetActive(true);
    _StartButton.gameObject.SetActive(false);

    RobotEngineManager.Instance.DisconnectFromRobot(kRobotID);
  }

  private void TestPassed() {
    _Background.color = Color.green;
    _RestartButton.gameObject.SetActive(true);
    _InProgressSpinner.gameObject.SetActive(false);

    RobotEngineManager.Instance.DisconnectFromRobot(kRobotID);
  }

  private void HandleRestartButtonClick() {
    _Background.color = Color.white;
    _RestartButton.gameObject.SetActive(false);
    _StartButton.gameObject.SetActive(true);
    _InProgressSpinner.gameObject.SetActive(false);

    RobotEngineManager.Instance.DisconnectFromRobot(kRobotID);

    if (!RobotEngineManager.Instance.IsConnectedToEngine) {
      RobotEngineManager.Instance.Connect(kEngineIP);
    } 
  }

  private void HandleFactoryResult(Anki.Cozmo.FactoryTestResultEntry result) {
    SetStatusText("Result Code: " + (int)result.result + " (" + result.result + ")");
    if (result.result == Anki.Cozmo.FactoryTestResultCode.SUCCESS) {
      TestPassed();
    }
    else {
      TestFailed();
    }
  }

  void Update() {
    _NetworkStatusText.text = _PingStatusComponent.GetNetworkStatus() ? "Network: Connected" : "Network: Disconnected";

    if (_PingStatusComponent.GetPingStatus() || _IsSim) {
      _StartButton.transform.FindChild("Text").GetComponent<UnityEngine.UI.Text>().text = "START";
      _StartButton.image.color = Color.green;
      _StartButton.interactable = true;
      _PingStatusText.text = "Ping: Success";
    }
    else {
      _StartButton.image.color = Color.gray;
      _StartButton.transform.FindChild("Text").GetComponent<UnityEngine.UI.Text>().text = "NO ROBOT CONNECTED";
      _StartButton.interactable = false;
      _PingStatusText.text = "Ping: Fail";
    }
  }
}

#endif