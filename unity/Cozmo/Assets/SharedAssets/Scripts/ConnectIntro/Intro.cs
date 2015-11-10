using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour {
  [SerializeField] protected InputField _EngineIP;
  [SerializeField] protected InputField _IP;
  [SerializeField] protected InputField _SimIp;
  [SerializeField] protected InputField _VisualizerIP;
  [SerializeField] protected Text _Error;

  private bool _Simulated = false;
  private string _CurrentRobotIP;
  private string _CurrentScene;
  private string _CurrentVizHostIP;

  private const int ROBOT_ID = 1;

  private Robot _Robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.CurrentRobot : null; } }

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

  private string LastVisualizerIP {
    get { return PlayerPrefs.GetString("LastVisualizerIp", "127.0.0.1"); }
    
    set { PlayerPrefs.SetString("LastVisualizerIp", value); }
  }

  protected void OnEnable() {

    _EngineIP.text = LastEngineIP;
    _IP.text = LastIP;
    _SimIp.text = LastSimIP;
    _VisualizerIP.text = LastVisualizerIP;

    _EngineIP.Rebuild(CanvasUpdate.PreRender);
    _IP.Rebuild(CanvasUpdate.PreRender);
    _SimIp.Rebuild(CanvasUpdate.PreRender);
    _VisualizerIP.Rebuild(CanvasUpdate.PreRender);

  }

  private void Start() {
    if (RobotEngineManager.instance != null && RobotEngineManager.instance.IsConnected)
      RobotEngineManager.instance.Disconnect();

    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.ConnectedToClient += Connected;
      RobotEngineManager.instance.DisconnectedFromClient += Disconnected;
      RobotEngineManager.instance.RobotConnected += RobotConnected;
    }

    Application.targetFrameRate = 60;
    
    Input.gyro.enabled = true;
    Input.compass.enabled = true;
    Input.multiTouchEnabled = true;
    Input.location.Start();
  }

  private void OnDestroy() {
    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.ConnectedToClient -= Connected;
      RobotEngineManager.instance.DisconnectedFromClient -= Disconnected;
      RobotEngineManager.instance.RobotConnected -= RobotConnected;
    }
  }

  private void Update() {

    if (RobotEngineManager.instance != null) {
      DisconnectionReason reason = RobotEngineManager.instance.GetLastDisconnectionReason();
      if (reason != DisconnectionReason.None) {
        _Error.text = "Disconnected: " + reason.ToString();
      }
    }
  }

  public void Play(bool sim) {
    _Simulated = sim;
    RobotEngineManager.instance.Disconnect();

    string errorText = null;
    string ipText = _Simulated ? _SimIp.text : _IP.text;
    if (string.IsNullOrEmpty(_EngineIP.text)) {
      errorText = "You must enter a device ip address.";
    }
    if (string.IsNullOrEmpty(errorText) && string.IsNullOrEmpty(ipText)) {
      errorText = "You must enter a robot ip address.";
    }

    if (string.IsNullOrEmpty(errorText)) {
      _CurrentRobotIP = ipText;
      _CurrentVizHostIP = _VisualizerIP.text;

      SaveData();
      RobotEngineManager.instance.Connect(_EngineIP.text);
      _Error.text = "<color=#ffffff>Connecting to engine at " + _EngineIP.text + "....</color>";
    }
    else {
      _Error.text = errorText;
    }
  }

  protected void SaveData() {
    LastIP = _IP.text;
    LastSimIP = _SimIp.text;
    LastEngineIP = _EngineIP.text;
    LastVisualizerIP = _VisualizerIP.text;
  }

  private void Connected(string connectionIdentifier) {
    _Error.text = "<color=#ffffff>Connected to " + connectionIdentifier + ". Force-adding robot...</color>";
    RobotEngineManager.instance.StartEngine(_CurrentVizHostIP);
    RobotEngineManager.instance.ForceAddRobot(ROBOT_ID, _CurrentRobotIP, _Simulated);
  }

  private void Disconnected(DisconnectionReason reason) {
    _Error.text = "Disconnected: " + reason.ToString();
  }

  private void RobotConnected(int robotID) {
    if (!RobotEngineManager.instance.Robots.ContainsKey(robotID)) {
      DAS.Error("Intro", "Unknown robot connected: " + robotID.ToString());
      return;
    }

    if (_Simulated && _Robot != null) {
      _Robot.VisionWhileMoving(true);    
      _Robot.StartFaceAwareness();
    }

    _Error.text = "";
    DAS.Info("Intro", "Robot Connected!");
  }

}
