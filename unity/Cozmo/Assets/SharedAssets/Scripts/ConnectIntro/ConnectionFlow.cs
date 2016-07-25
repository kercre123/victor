using UnityEngine;
using System.Collections;

public class ConnectionFlow : MonoBehaviour {

  public System.Action ConnectionFlowComplete;
  public System.Action ConnectionFlowQuit;

  public const float kConnectionFlowDelay = 1.0f;

  [SerializeField]
  private ConnectionFlowBackground _ConnectionFlowBackgroundPrefab;
  private ConnectionFlowBackground _ConnectionFlowBackgroundInstance;

  [SerializeField]
  private SearchForCozmoScreen _SearchForCozmoScreenPrefab;
  private SearchForCozmoScreen _SearchForCozmoScreenInstance;

  [SerializeField]
  private SearchForCozmoFailedScreen _SearchForCozmoFailedScreenPrefab;
  private SearchForCozmoFailedScreen _SearchForCozmoFailedScreenInstance;

  [SerializeField]
  private SecuringConnectionScreen _SecuringConnectionScreenPrefab;
  private SecuringConnectionScreen _SecuringConnectionScreenInstance;

  [SerializeField]
  private ConnectingToCozmoScreen _ConnectingToCozmoScreenPrefab;
  private ConnectingToCozmoScreen _ConnectingToCozmoScreenInstance;

  [SerializeField]
  private GameObject _WakingUpCozmoScreenPrefab;
  private GameObject _WakingUpCozmoScreenInstance;

  [SerializeField]
  private PingStatus _PingStatus;

  private const int kRobotID = 1;

  private bool _Simulated = false;
  private string _CurrentRobotIP;

  private bool _RobotConnected = false;

  private void Start() {
    RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToEngine;
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(RobotConnected);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.ConnectedToClient -= HandleConnectedToEngine;
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(Disconnected);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnected>(RobotConnected);
  }

  public void Play(bool sim) {
    _Simulated = sim;

    if (sim) {
      _CurrentRobotIP = RobotEngineManager.kSimRobotIP;
    }
    else {
      _CurrentRobotIP = RobotEngineManager.kRobotIP;
    }

    CreateConnectionFlowBackground();
    ShowSearchForCozmo();
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Connecting);
  }

  private void RestartConnectionFlow() {
    UIManager.CloseView(_ConnectionFlowBackgroundInstance);
    if (ConnectionFlowQuit != null) {
      ConnectionFlowQuit();
    }
  }

  private void CreateConnectionFlowBackground() {
    _ConnectionFlowBackgroundInstance = UIManager.OpenView<ConnectionFlowBackground>(_ConnectionFlowBackgroundPrefab);
  }

  private void ShowSearchForCozmo() {
    _ConnectionFlowBackgroundInstance.SetStateInProgress(0);
    _SearchForCozmoScreenInstance = UIManager.CreateUIElement(_SearchForCozmoScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<SearchForCozmoScreen>();
    _SearchForCozmoScreenInstance.Initialize(_PingStatus);
    _SearchForCozmoScreenInstance.OnScreenComplete += HandleSearchForCozmoScreenDone;
  }

  private void HandleSearchForCozmoScreenDone(bool success) {

    _SearchForCozmoScreenInstance.OnScreenComplete -= HandleSearchForCozmoScreenDone;
    GameObject.Destroy(_SearchForCozmoScreenInstance.gameObject);

    if (success || _Simulated) {
      _ConnectionFlowBackgroundInstance.SetStateComplete(0);
      ShowSecuringConnectionScreen();
    }
    else {
      _SearchForCozmoFailedScreenInstance = UIManager.CreateUIElement(_SearchForCozmoFailedScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<SearchForCozmoFailedScreen>();
      _SearchForCozmoFailedScreenInstance.OnEndpointFound += HandleEndpointFound;
      _SearchForCozmoFailedScreenInstance.OnQuitFlow += HandleOnQuitFlowFromFailedSearch;
      _SearchForCozmoFailedScreenInstance.Initialize(_PingStatus);
      _ConnectionFlowBackgroundInstance.SetStateFailed(0);
    }
  }

  private void HandleOnQuitFlowFromFailedSearch() {
    _SearchForCozmoFailedScreenInstance.OnEndpointFound -= HandleEndpointFound;
    _SearchForCozmoFailedScreenInstance.OnQuitFlow -= HandleOnQuitFlowFromFailedSearch;
    GameObject.Destroy(_SearchForCozmoFailedScreenInstance.gameObject);
    RestartConnectionFlow();
  }

  private void HandleEndpointFound() {
    _SearchForCozmoFailedScreenInstance.OnEndpointFound -= HandleEndpointFound;
    _SearchForCozmoFailedScreenInstance.OnQuitFlow -= HandleOnQuitFlowFromFailedSearch;
    GameObject.Destroy(_SearchForCozmoFailedScreenInstance.gameObject);
    _ConnectionFlowBackgroundInstance.SetStateComplete(0);
    ShowSecuringConnectionScreen();
  }

  private void ShowSecuringConnectionScreen() {

#if UNITY_EDITOR
    // In editor we delay the connection to the engine. This is so we can use things
    // like mock mode which does not require the engine.
    ConnectToEngine();
#else
    ConnectToRobot();
#endif

    _ConnectionFlowBackgroundInstance.SetStateInProgress(1);
    _SecuringConnectionScreenInstance = UIManager.CreateUIElement(_SecuringConnectionScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<SecuringConnectionScreen>();
    _SecuringConnectionScreenInstance.OnScreenComplete += HandleSecuringConnectionScreenDone;
  }

  private void HandleSecuringConnectionScreenDone(bool success) {
    _SecuringConnectionScreenInstance.OnScreenComplete -= HandleSecuringConnectionScreenDone;
    GameObject.Destroy(_SecuringConnectionScreenInstance.gameObject);
    _ConnectionFlowBackgroundInstance.SetStateComplete(1);
    ShowConnectingToCozmoScreen();

  }

  private void ShowConnectingToCozmoScreen() {
    _ConnectingToCozmoScreenInstance = UIManager.CreateUIElement(_ConnectingToCozmoScreenPrefab.gameObject, _ConnectionFlowBackgroundInstance.transform).GetComponent<ConnectingToCozmoScreen>();
    _ConnectingToCozmoScreenInstance.OnScreenComplete += HandleConnectingToCozmoScreenDone;
    _ConnectionFlowBackgroundInstance.SetStateInProgress(2);
    if (_RobotConnected) {
      _ConnectingToCozmoScreenInstance.RobotConnected();
    }
  }

  private void HandleConnectingToCozmoScreenDone(bool success) {

    GameObject.Destroy(_ConnectingToCozmoScreenInstance.gameObject);

    if (success) {
      _ConnectionFlowBackgroundInstance.SetStateComplete(2);

      if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.RunPressDemo) {
        HandleWakeAnimationComplete(true);
      }
      else {
        _WakingUpCozmoScreenInstance = UIManager.CreateUIElement(_WakingUpCozmoScreenPrefab, _ConnectionFlowBackgroundInstance.transform);
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.ConnectWakeUp, HandleWakeAnimationComplete);
      }

    }
    else {
      // TODO: Handle fail connect to robot case
      _ConnectionFlowBackgroundInstance.SetStateFailed(2);
    }
  }

  private void HandleWakeAnimationComplete(bool success) {
    GameObject.Destroy(_WakingUpCozmoScreenInstance);
    UIManager.CloseView(_ConnectionFlowBackgroundInstance);

    // explicitly enable charger behavior since it should be off by default in engine.
    RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("on_start_wakeup", Anki.Cozmo.BehaviorType.ReactToOnCharger, true);
    if (ConnectionFlowComplete != null) {
      ConnectionFlowComplete();
    }
  }

  private void ConnectToEngine() {
    RobotEngineManager.Instance.Disconnect();
    RobotEngineManager.Instance.Connect(RobotEngineManager.kEngineIP);
  }

  private void ConnectToRobot() {
    DAS.Info("ConnectDialog.ConnectToRobot", "Trying to connect to robot");
    RobotEngineManager.Instance.ForceAddRobot(kRobotID, _CurrentRobotIP, _Simulated);
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

    _RobotConnected = true;
    if (_ConnectingToCozmoScreenInstance != null) {
      _ConnectingToCozmoScreenInstance.RobotConnected();
    }
  }
}
