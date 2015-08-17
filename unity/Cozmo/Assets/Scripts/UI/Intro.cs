using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour {
  [SerializeField] protected InputField engineIP;
  [SerializeField] protected InputField ip;
  [SerializeField] protected InputField simIP;
  [SerializeField] protected InputField visualizerIP;
  [SerializeField] protected Text error;

  private bool simulated = false;
  private string currentRobotIP;
  private string currentScene;
  private string currentVizHostIP;

  private const int ROBOT_ID = 1;

  private Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  private string lastEngineIp
  {
    get { return PlayerPrefs.GetString("LastEngineIP", "127.0.0.1"); }

    set { PlayerPrefs.SetString("LastEngineIP", value); }
  }

  private string lastIp
  {
    get { return PlayerPrefs.GetString("LastIP", "172.31.1.1"); }
    
    set { PlayerPrefs.SetString("LastIP", value); }
  }

  private string lastSimIp
  {
    get { return PlayerPrefs.GetString("LastSimIP", "127.0.0.1"); }
    
    set { PlayerPrefs.SetString("LastSimIP", value); }
  }
  
  private string lastId
  {
    get { return PlayerPrefs.GetString("LastID", "1"); }
    
    set { PlayerPrefs.SetString("LastID", value); }
  }

  private string lastVisualizerIp
  {
    get { return PlayerPrefs.GetString("LastVisualizerIp", "127.0.0.1"); }
    
    set { PlayerPrefs.SetString("LastVisualizerIp", value); }
  }

  protected void OnEnable() {

    engineIP.text = lastEngineIp;
    ip.text = lastIp;
    simIP.text = lastSimIp;
    visualizerIP.text = lastVisualizerIp;

    engineIP.Rebuild(CanvasUpdate.PreRender);
    ip.Rebuild(CanvasUpdate.PreRender);
    simIP.Rebuild(CanvasUpdate.PreRender);
    visualizerIP.Rebuild(CanvasUpdate.PreRender);

    /*if (robot != null) {
      Debug.Log("knownObjects cleared!");
      robot.knownObjects.Clear();
    }*/

  }

  private void Start()
  {
    if(RobotEngineManager.instance != null && RobotEngineManager.instance.IsConnected) RobotEngineManager.instance.Disconnect();

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

  private void Update()
  {

    if (RobotEngineManager.instance != null) {
      DisconnectionReason reason = RobotEngineManager.instance.GetLastDisconnectionReason ();
      if (reason != DisconnectionReason.None) {
        error.text = "Disconnected: " + reason.ToString ();
      }
    }
  }

  public void Play(bool sim) {
    simulated = sim;
    RobotEngineManager.instance.Disconnect ();

    string errorText = null;
    string ipText = simulated ? simIP.text : ip.text;
    if(string.IsNullOrEmpty(engineIP.text)) {
      errorText = "You must enter a device ip address.";
    }
    if(string.IsNullOrEmpty(errorText) && string.IsNullOrEmpty(ipText)) {
      errorText = "You must enter a robot ip address.";
    }

    if (string.IsNullOrEmpty (errorText)) {
      currentRobotIP = ipText;
      currentVizHostIP = visualizerIP.text;

      SaveData ();
      RobotEngineManager.instance.Connect (engineIP.text);
      error.text = "<color=#ffffff>Connecting to engine at " + engineIP.text + "....</color>";
    } else {
      error.text = errorText;
    }
  }

  protected void SaveData() {
    lastIp = ip.text;
    lastSimIp = simIP.text;
    lastEngineIp = engineIP.text;
    lastVisualizerIp = visualizerIP.text;
  }

  public void FakeTest() {
    SaveData();
    Application.LoadLevel("ControlSchemeTest");
  }

  private void Connected(string connectionIdentifier)
  {
    error.text = "<color=#ffffff>Connected to " + connectionIdentifier + ". Force-adding robot...</color>";
    RobotEngineManager.instance.StartEngine (currentVizHostIP);
    RobotEngineManager.instance.ForceAddRobot(ROBOT_ID, currentRobotIP, simulated);
  }

  private void Disconnected(DisconnectionReason reason)
  {
    error.text = "Disconnected: " + reason.ToString();
  }

  private void RobotConnected(int robotID)
  {
    if (!RobotEngineManager.instance.robots.ContainsKey(robotID)) {
      Debug.LogError ("Unknown robot connected: " + robotID.ToString());
      return;
    }

    if(simulated && robot != null) {
      robot.VisionWhileMoving(true);    
      robot.StartFaceAwareness();    
    }

    error.text = "";
    Application.LoadLevel("GameMenu");
  }

}
