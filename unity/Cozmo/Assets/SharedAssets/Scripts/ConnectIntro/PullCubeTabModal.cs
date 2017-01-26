using UnityEngine;
using System.Collections.Generic;

public class PullCubeTabModal : Cozmo.UI.BaseModal {

  [SerializeField]
  private const float _kTimeBeforeForceContinue = 15.0f;

  [SerializeField]
  private const float _kMaxDiscoveryTime = 7.0f;

  // minimum threshold time between each object connecting and it being registered to
  // front end UI
  [SerializeField]
  private const float _kTimeBetweenObjectsConnected = 1.5f;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ContinueButton;

  [SerializeField]
  private Anki.Cozmo.ObjectType[] _ObjectConnectedTypeList;

  [SerializeField]
  private UnityEngine.UI.Image[] _ObjectConnectedImagesList;

  [SerializeField]
  private GameObject[] _DoneMarks;


  private List<Anki.Cozmo.ObjectType> _ObjectConnectedList = new List<Anki.Cozmo.ObjectType>();
  // staging area for newly connected objects so we can add a delay / animation before putting it into
  // the _ObjectConnectedList
  private Anki.Cozmo.ObjectType _NewlyConnectedObject = Anki.Cozmo.ObjectType.Invalid;
  private float _NewlyConnectedObjectTime;
  private int _NewlyConnectedObjectId;

  private float _StartTime;
  private bool _AllObjectsConnected = false;

  private const uint kConnectedOnPeriod_ms = 10;
  private const uint kConnectedOffPeriod_ms = 380;
  private const uint kConnectedOnTransition_ms = 200;
  private const uint kConnectedOffTransition_ms = 50;
  private readonly int[] kConnectedOffset = new int[] { 0, 100, 200, 300 };

  private void Awake() {
    _ContinueButton.Initialize(HandleContinueButton, "pull_cube_tab_continue_button", this.DASEventDialogName);
    _ContinueButton.gameObject.SetActive(false);
    _StartTime = Time.time;
    for (int i = 0; i < _ObjectConnectedImagesList.Length; ++i) {
      _ObjectConnectedImagesList[i].gameObject.SetActive(false);
    }

    for (int i = 0; i < _DoneMarks.Length; ++i) {
      _DoneMarks[i].gameObject.SetActive(false);
    }

    // Disable freeplay light states
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      robot.SetEnableFreeplayLightStates(false);
    }

    // Enable the automatic block pool
    RobotEngineManager.Instance.BlockPoolTracker.EnableBlockPool(true, _kMaxDiscoveryTime);

    DasTracker.Instance.TrackCubePromptEntered();

    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Sim) {
      _ContinueButton.gameObject.SetActive(true);
    }
  }

  protected void Update() {
    if (_AllObjectsConnected || IsClosed) {
      return;
    }

    if (_NewlyConnectedObject == Anki.Cozmo.ObjectType.Invalid) {
      CheckForNewObjects();
    }
    else {
      ProcessNewObject();
    }

    if (_ObjectConnectedList.Count == _ObjectConnectedTypeList.Length) {
      _AllObjectsConnected = true;
      Invoke("HandleContinueButton", _kTimeBetweenObjectsConnected);
    }
    else if (Time.time - _StartTime > _kTimeBeforeForceContinue) {
      _ContinueButton.gameObject.SetActive(true);
    }

  }

  private void CheckForNewObjects() {
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      foreach (KeyValuePair<int, LightCube> kvp in robot.LightCubes) {
        if (_ObjectConnectedList.Contains(kvp.Value.ObjectType) == false) {
          _NewlyConnectedObject = kvp.Value.ObjectType;
          _NewlyConnectedObjectTime = Time.time;
          _NewlyConnectedObjectId = kvp.Key;
          DAS.Info("PullCubeTabView.CheckForNewObject", "New object found! " + kvp.Value.ObjectType);
          break;
        }
      }
    }
  }

  private void ProcessNewObject() {
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      if (Time.time - _NewlyConnectedObjectTime > _kTimeBetweenObjectsConnected) {
        DAS.Debug("PullCubeTabView.ProcessNewObject", "Processing: " + _NewlyConnectedObject);
        if (robot.LightCubes.ContainsKey(_NewlyConnectedObjectId)) {
          int typeIndex = (int)robot.LightCubes[_NewlyConnectedObjectId].ObjectType - 1;
          _ObjectConnectedImagesList[typeIndex].gameObject.SetActive(true);
          _DoneMarks[typeIndex].gameObject.SetActive(true);
          _ObjectConnectedList.Add(_NewlyConnectedObject);
          robot.LightCubes[_NewlyConnectedObjectId].SetLEDs(Color.green.ToUInt(), Color.black.ToUInt(),
                                                            kConnectedOnPeriod_ms, kConnectedOffPeriod_ms,
                                                            kConnectedOnTransition_ms, kConnectedOffTransition_ms,
                                                            kConnectedOffset);
          Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Cozmo_Connect);
        }
        _NewlyConnectedObject = Anki.Cozmo.ObjectType.Invalid;
      }
    }
  }

  private void HandleContinueButton() {
    this.CloseDialog();
  }

  protected override void CleanUp() {
    // Turn off cube lights
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      foreach (KeyValuePair<int, LightCube> kvp in robot.LightCubes) {
        kvp.Value.SetLEDsOff();
      }
    }
  }
}
