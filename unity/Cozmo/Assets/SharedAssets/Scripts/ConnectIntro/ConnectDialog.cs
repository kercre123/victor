using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ConnectDialog : MonoBehaviour {

  [SerializeField]
  private Text _ConnectionStatus;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ConnectButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SimButton;

  [SerializeField]
  private PingStatus _PingStatus;

  private const int kRobotID = 1;

  private bool _Simulated = false;
  private string _CurrentRobotIP;
  private string _CurrentScene;
  private bool _Connecting = false;

  private IRobot _Robot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private string EngineIP {
    get { return PlayerPrefs.GetString("EngineIP", "127.0.0.1"); }

    set { PlayerPrefs.SetString("EngineIP", value); }
  }

  private string RobotIP {
    get { return PlayerPrefs.GetString("RobotIP", RobotEngineManager.kRobotIP); }

    set { PlayerPrefs.SetString("RobotIP", value); }
  }

  private string SimRobotIP {
    get { return PlayerPrefs.GetString("SimRobotIP", "127.0.0.1"); }

    set { PlayerPrefs.SetString("SimRobotIP", value); }
  }

  private string RobotID {
    get { return PlayerPrefs.GetString("RobotID", "1"); }

    set { PlayerPrefs.SetString("RobotID", value); }
  }

  private string RobotName {
    get { return PlayerPrefs.GetString("RobotName", "OK0000"); }

    set { PlayerPrefs.SetString("RobotName", value); }
  }

  private void Start() {
    if (RobotEngineManager.Instance != null && RobotEngineManager.Instance.IsConnected)
      RobotEngineManager.Instance.Disconnect();

    if (RobotEngineManager.Instance != null) {
      RobotEngineManager.Instance.ConnectedToClient += Connected;
      RobotEngineManager.Instance.DisconnectedFromClient += Disconnected;
      RobotEngineManager.Instance.RobotConnected += RobotConnected;
    }

    Application.targetFrameRate = 30;

    Input.gyro.enabled = true;
    Input.compass.enabled = true;
    Input.multiTouchEnabled = true;

    _ConnectButton.Initialize(HandleConnectButton, "connect_button", "connect_dialog");
    _SimButton.Initialize(HandleSimButton, "sim_button", "connect_dialog");

    #if !UNITY_EDITOR
    _SimButton.gameObject.SetActive(false);
    #endif
  }

  private void HandleConnectButton() {
    this.Play(false);
  }

  private void HandleSimButton() {
    this.Play(true);
  }

  private void OnDestroy() {
    if (RobotEngineManager.Instance != null) {
      RobotEngineManager.Instance.ConnectedToClient -= Connected;
      RobotEngineManager.Instance.DisconnectedFromClient -= Disconnected;
      RobotEngineManager.Instance.RobotConnected -= RobotConnected;
    }
  }

  private void Update() {

    if (_Connecting || !_PingStatus.GetPingStatus()) {
      _ConnectButton.Interactable = false;
    }
    else {
      _ConnectButton.Interactable = true;
    }

    if (RobotEngineManager.Instance != null) {
      DisconnectionReason reason = RobotEngineManager.Instance.GetLastDisconnectionReason();
      if (reason != DisconnectionReason.None) {
        _ConnectionStatus.text = "Disconnected: " + reason.ToString();
      }
    }
  }

  public void Play(bool sim) {
    _Connecting = true;
    _ConnectButton.Interactable = false;
    _SimButton.Interactable = false;
    _ConnectButton.Text = "LOADING";

    _Simulated = sim;
    RobotEngineManager.Instance.Disconnect();

    string errorText = null;
    string ipText = _Simulated ? SimRobotIP : RobotIP;

    if (string.IsNullOrEmpty(errorText)) {
      _CurrentRobotIP = ipText;

      RobotEngineManager.Instance.Connect(EngineIP);
      _ConnectionStatus.text = "<color=#ffffff>Connecting to engine at " + EngineIP + "....</color>";
    }
    else {
      _ConnectionStatus.text = errorText;
    }
  }

  public void PlayMock() {
    RobotEngineManager.Instance.MockConnect();
  }

  public void ConfigureWifi() {
    CozmoBinding.WifiSetup(RobotName, "2manysecrets");
  }

  private void Connected(string connectionIdentifier) {
    _ConnectionStatus.text = "<color=#ffffff>Connected to " + connectionIdentifier + ". Force-adding robot...</color>";
    RobotEngineManager.Instance.StartEngine();
    RobotEngineManager.Instance.ForceAddRobot(kRobotID, _CurrentRobotIP, _Simulated);
  }

  private void Disconnected(DisconnectionReason reason) {
    _ConnectionStatus.text = "Disconnected: " + reason.ToString();
  }

  private void RobotConnected(int robotID) {
    if (!RobotEngineManager.Instance.Robots.ContainsKey(robotID)) {
      DAS.Error(this, "Unknown robot connected: " + robotID.ToString());
      return;
    }
      
    // actually load default volume
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.VolumePreferences.ContainsKey(Anki.Cozmo.Audio.VolumeParameters.VolumeType.Robot)) {
      _Robot.SetRobotVolume(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.VolumePreferences[Anki.Cozmo.Audio.VolumeParameters.VolumeType.Robot]);
    }
    else {
      #if UNITY_EDITOR
      _Robot.SetRobotVolume(0.06f);
      #else
      _Robot.SetRobotVolume(1.0f);
      #endif
    }

    if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.SOSLoggerEnabled) {
      ConsoleLogManager.Instance.EnableSOSLogs(true);
    }

    Screen.sleepTimeout = SleepTimeout.NeverSleep;

    _ConnectionStatus.text = "";
    DAS.Info(this, "Robot Connected!");
  }

}
