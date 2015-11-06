using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour {
  [SerializeField] protected InputField engineIP_;
  [SerializeField] protected InputField ip_;
  [SerializeField] protected InputField simIP_;
  [SerializeField] protected InputField visualizerIP_;
  [SerializeField] protected Text error_;

  private bool simulated_ = false;
  private string currentRobotIP_;
  private string currentScene_;
  private string currentVizHostIP_;

  private const int ROBOT_ID = 1;

  private Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.CurrentRobot : null; } }

  private string lastEngineIp {
    get { return PlayerPrefs.GetString("LastEngineIP", "127.0.0.1"); }

    set { PlayerPrefs.SetString("LastEngineIP", value); }
  }

  private string lastIp {
    get { return PlayerPrefs.GetString("LastIP", "172.31.1.1"); }
    
    set { PlayerPrefs.SetString("LastIP", value); }
  }

  private string lastSimIp {
    get { return PlayerPrefs.GetString("LastSimIP", "127.0.0.1"); }
    
    set { PlayerPrefs.SetString("LastSimIP", value); }
  }

  private string lastId {
    get { return PlayerPrefs.GetString("LastID", "1"); }
    
    set { PlayerPrefs.SetString("LastID", value); }
  }

  private string lastVisualizerIp {
    get { return PlayerPrefs.GetString("LastVisualizerIp", "127.0.0.1"); }
    
    set { PlayerPrefs.SetString("LastVisualizerIp", value); }
  }

  protected void OnEnable() {

    engineIP_.text = lastEngineIp;
    ip_.text = lastIp;
    simIP_.text = lastSimIp;
    visualizerIP_.text = lastVisualizerIp;

    engineIP_.Rebuild(CanvasUpdate.PreRender);
    ip_.Rebuild(CanvasUpdate.PreRender);
    simIP_.Rebuild(CanvasUpdate.PreRender);
    visualizerIP_.Rebuild(CanvasUpdate.PreRender);

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
        error_.text = "Disconnected: " + reason.ToString();
      }
    }
  }

  public void Play(bool sim) {
    simulated_ = sim;
    RobotEngineManager.instance.Disconnect();

    string errorText = null;
    string ipText = simulated_ ? simIP_.text : ip_.text;
    if (string.IsNullOrEmpty(engineIP_.text)) {
      errorText = "You must enter a device ip address.";
    }
    if (string.IsNullOrEmpty(errorText) && string.IsNullOrEmpty(ipText)) {
      errorText = "You must enter a robot ip address.";
    }

    if (string.IsNullOrEmpty(errorText)) {
      currentRobotIP_ = ipText;
      currentVizHostIP_ = visualizerIP_.text;

      SaveData();
      RobotEngineManager.instance.Connect(engineIP_.text);
      error_.text = "<color=#ffffff>Connecting to engine at " + engineIP_.text + "....</color>";
    }
    else {
      error_.text = errorText;
    }
  }

  protected void SaveData() {
    lastIp = ip_.text;
    lastSimIp = simIP_.text;
    lastEngineIp = engineIP_.text;
    lastVisualizerIp = visualizerIP_.text;
  }

  private void Connected(string connectionIdentifier) {
    error_.text = "<color=#ffffff>Connected to " + connectionIdentifier + ". Force-adding robot...</color>";
    RobotEngineManager.instance.StartEngine(currentVizHostIP_);
    RobotEngineManager.instance.ForceAddRobot(ROBOT_ID, currentRobotIP_, simulated_);
  }

  private void Disconnected(DisconnectionReason reason) {
    error_.text = "Disconnected: " + reason.ToString();
  }

  private void RobotConnected(int robotID) {
    if (!RobotEngineManager.instance.Robots.ContainsKey(robotID)) {
      DAS.Error("Intro", "Unknown robot connected: " + robotID.ToString());
      return;
    }

    if (simulated_ && robot != null) {
      robot.VisionWhileMoving(true);    
      robot.StartFaceAwareness();
    }

    error_.text = "";
    DAS.Info("Intro", "Robot Connected!");
  }

}
