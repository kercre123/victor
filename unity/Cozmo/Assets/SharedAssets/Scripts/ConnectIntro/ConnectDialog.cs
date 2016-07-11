using UnityEngine;
using UnityEngine.UI;

public class ConnectDialog : MonoBehaviour {

  [SerializeField]
  private Cozmo.UI.CozmoButton _ConnectButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SimButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _MockButton;

  [SerializeField]
  private PingStatus _PingStatus;

  private const int kRobotID = 1;

  private bool _Simulated = false;
  private string _CurrentRobotIP;
  private string _CurrentScene;
  private bool _Connecting = false;

  private void Start() {
    if (RobotEngineManager.Instance != null) {
      RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToEngine;
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(RobotConnected);
    }

    Application.targetFrameRate = 30;
    Screen.sleepTimeout = SleepTimeout.NeverSleep;

    Input.gyro.enabled = true;
    Input.compass.enabled = true;
    Input.multiTouchEnabled = true;

    _ConnectButton.Initialize(HandleConnectButton, "connect_button", "connect_dialog");
    _SimButton.Initialize(HandleSimButton, "sim_button", "connect_dialog");
    _MockButton.Initialize(HandleMockButton, "mock_button", "connect_dialog");

#if !UNITY_EDITOR
    // hide sim and mock buttons for on device deployments
    _SimButton.gameObject.SetActive(false);
    _MockButton.gameObject.SetActive(false);
#endif

    _ConnectButton.Text = Localization.Get(LocalizationKeys.kLabelConnect);

    UIManager.Instance.BackgroundColorController.SetBackgroundColor(Cozmo.UI.BackgroundColorController.BackgroundColor.Yellow);
  }

  private void HandleMockButton() {
    this.PlayMock();
  }

  private void HandleConnectButton() {
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
    this.Play(false);
  }

  private void HandleSimButton() {
    this.Play(true);
  }

  private void ConnectToEngine() {
    RobotEngineManager.Instance.Disconnect();
    RobotEngineManager.Instance.Connect(RobotEngineManager.kEngineIP);
  }

  private void ConnectToRobot() {
    DAS.Info("ConnectDialog.ConnectToRobot", "Trying to connect to robot");
    RobotEngineManager.Instance.ForceAddRobot(kRobotID, _CurrentRobotIP, _Simulated);
  }

  private void OnDestroy() {
    if (RobotEngineManager.Instance != null) {
      RobotEngineManager.Instance.ConnectedToClient -= HandleConnectedToEngine;
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(RobotConnected);
    }
  }

  private void Update() {
    if (_Connecting || !_PingStatus.GetPingStatus()) {
      _ConnectButton.Interactable = false;
    }
    else {
      _ConnectButton.Interactable = true;
    }
  }

  public void Play(bool sim) {
    _Connecting = true;
    _ConnectButton.Interactable = false;
    _SimButton.Interactable = false;
    _MockButton.Interactable = false;
    _Simulated = sim;

    if (sim) {
      _CurrentRobotIP = RobotEngineManager.kSimRobotIP;
    }
    else {
      _CurrentRobotIP = RobotEngineManager.kRobotIP;
    }

#if UNITY_EDITOR
    // In editor we delay the connection to the engine. This is so we can use things
    // like mock mode which does not require the engine.
    ConnectToEngine();
#else
    ConnectToRobot();
#endif

  }

  public void PlayMock() {
    RobotEngineManager.Instance.MockConnect();
  }

  private void HandleConnectedToEngine(string connectionIdentifier) {
#if UNITY_EDITOR
    // in editor we have to wait to connect to the engine first before connecting
    // to the robot.
    DAS.Info("ConnectDialog.HandleConnectedToEngine", "In Editor so connecting to robot after connecting to engine");
    SetupEngine();
    ConnectToRobot();
#endif
  }

  private void Disconnected(Anki.Cozmo.ExternalInterface.RobotDisconnected message) {
    DAS.Error("ConnectDialog", "Robot Disconnected");
  }

  private void SetupEngine() {
    RobotEngineManager.Instance.StartEngine();
    // Set initial volumes
    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues();
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.CozmoConnect);
  }

  private void RobotConnected(Anki.Cozmo.ExternalInterface.RobotConnected message) {
    int robotID = (int)message.robotID;
    if (!RobotEngineManager.Instance.Robots.ContainsKey(robotID)) {
      DAS.Error(this, "Unknown robot connected: " + robotID.ToString());
      return;
    }

    DAS.Debug("ConnectDialog.RobotConnected", "RobotID: " + robotID);

    if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.SOSLoggerEnabled) {
      ConsoleLogManager.Instance.EnableSOSLogs(true);
    }

    // Set initial Robot Volume when connecting
    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues(new Anki.Cozmo.Audio.VolumeParameters.VolumeType[] { Anki.Cozmo.Audio.VolumeParameters.VolumeType.Robot });
    DAS.Info(this, "Robot Connected!");
  }

}
