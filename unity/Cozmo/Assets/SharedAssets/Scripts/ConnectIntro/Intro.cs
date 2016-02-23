using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour {
  [SerializeField] protected InputField _EngineIPInputField;
  [SerializeField] protected InputField _RobotIPInputField;
  [SerializeField] protected InputField _SimIPInputField;
  [SerializeField] protected Text _Error;

  private bool _Simulated = false;
  private string _CurrentRobotIP;
  private string _CurrentScene;

  private const int kRobotID = 1;

  private IRobot _Robot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private string LastEngineIP {
    get { return PlayerPrefs.GetString("LastEngineIP", "127.0.0.1"); }

    set { PlayerPrefs.SetString("LastEngineIP", value); }
  }

  private string LastIP {
    get { return PlayerPrefs.GetString("LastIP", "172.31.1.1"); }
    
    set { PlayerPrefs.SetString("LastIP", value); }
  }

  private string LastSimIP {
    get { return PlayerPrefs.GetString("LastSimIP", "127.0.0.1"); }
    
    set { PlayerPrefs.SetString("LastSimIP", value); }
  }

  private string LastID {
    get { return PlayerPrefs.GetString("LastID", "1"); }
    
    set { PlayerPrefs.SetString("LastID", value); }
  }

  protected void OnEnable() {

    _EngineIPInputField.text = LastEngineIP;
    _RobotIPInputField.text = LastIP;
    _SimIPInputField.text = LastSimIP;

    _EngineIPInputField.Rebuild(CanvasUpdate.PreRender);
    _RobotIPInputField.Rebuild(CanvasUpdate.PreRender);
    _SimIPInputField.Rebuild(CanvasUpdate.PreRender);

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
    Input.location.Start();
  }

  private void OnDestroy() {
    if (RobotEngineManager.Instance != null) {
      RobotEngineManager.Instance.ConnectedToClient -= Connected;
      RobotEngineManager.Instance.DisconnectedFromClient -= Disconnected;
      RobotEngineManager.Instance.RobotConnected -= RobotConnected;
    }
  }

  private void Update() {

    if (RobotEngineManager.Instance != null) {
      DisconnectionReason reason = RobotEngineManager.Instance.GetLastDisconnectionReason();
      if (reason != DisconnectionReason.None) {
        _Error.text = "Disconnected: " + reason.ToString();
      }
    }
  }

  public void Play(bool sim) {
    _Simulated = sim;
    RobotEngineManager.Instance.Disconnect();

    string errorText = null;
    string ipText = _Simulated ? _SimIPInputField.text : _RobotIPInputField.text;
    if (string.IsNullOrEmpty(_EngineIPInputField.text)) {
      errorText = "You must enter a device ip address.";
    }
    if (string.IsNullOrEmpty(errorText) && string.IsNullOrEmpty(ipText)) {
      errorText = "You must enter a robot ip address.";
    }

    if (string.IsNullOrEmpty(errorText)) {
      _CurrentRobotIP = ipText;

      SaveData();
      RobotEngineManager.Instance.Connect(_EngineIPInputField.text);
      _Error.text = "<color=#ffffff>Connecting to engine at " + _EngineIPInputField.text + "....</color>";
    }
    else {
      _Error.text = errorText;
    }
  }

  public void PlayMock() {
    RobotEngineManager.Instance.MockConnect();
  }

  protected void SaveData() {
    LastIP = _RobotIPInputField.text;
    LastSimIP = _SimIPInputField.text;
    LastEngineIP = _EngineIPInputField.text;
  }

  private void Connected(string connectionIdentifier) {
    _Error.text = "<color=#ffffff>Connected to " + connectionIdentifier + ". Force-adding robot...</color>";
    RobotEngineManager.Instance.StartEngine();
    RobotEngineManager.Instance.ForceAddRobot(kRobotID, _CurrentRobotIP, _Simulated);
  }

  private void Disconnected(DisconnectionReason reason) {
    _Error.text = "Disconnected: " + reason.ToString();
  }

  private void RobotConnected(int robotID) {
    if (!RobotEngineManager.Instance.Robots.ContainsKey(robotID)) {
      DAS.Error(this, "Unknown robot connected: " + robotID.ToString());
      return;
    }

    #if UNITY_EDITOR
    _Robot.SetRobotVolume(0.06f);
    #endif

    Screen.sleepTimeout = SleepTimeout.NeverSleep;

    _Error.text = "";
    DAS.Info(this, "Robot Connected!");
  }

}
