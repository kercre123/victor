using UnityEngine;
using System.Collections.Generic;

public class PullCubeTabView : Cozmo.UI.BaseView {

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
  private Sprite _ConnectedStateSprite;

  private List<Anki.Cozmo.ObjectType> _ObjectConnectedList = new List<Anki.Cozmo.ObjectType>();
  // staging area for newly connected objects so we can add a delay / animation before putting it into
  // the _ObjectConnectedList
  private Anki.Cozmo.ObjectType _NewlyConnectedObject = Anki.Cozmo.ObjectType.Invalid;
  private float _NewlyConnectedObjectTime;
  private int _NewlyConnectedObjectId;

  private float _StartTime;
  private bool _AllObjectsConnected = false;

  private void Awake() {
    _ContinueButton.Initialize(HandleContinueButton, "pull_cube_tab_continue_button", this.DASEventViewName);
    _ContinueButton.gameObject.SetActive(false);
    _StartTime = Time.time;
    for (int i = 0; i < _ObjectConnectedImagesList.Length; ++i) {
      _ObjectConnectedImagesList[i].gameObject.SetActive(false);
    }

    // Enable the automatic block pool
    Anki.Cozmo.ExternalInterface.BlockPoolEnabledMessage blockPoolEnabledMessage = new Anki.Cozmo.ExternalInterface.BlockPoolEnabledMessage();
    blockPoolEnabledMessage.enabled = true;
    blockPoolEnabledMessage.discoveryTimeSecs = _kMaxDiscoveryTime;
    RobotEngineManager.Instance.Message.BlockPoolEnabledMessage = blockPoolEnabledMessage;
    RobotEngineManager.Instance.SendMessage();
  }

  protected override void Update() {
    base.Update();
    if (_AllObjectsConnected || ClosingAnimationPlaying) {
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
        int typeIndex = (int)robot.LightCubes[_NewlyConnectedObjectId].ObjectType - 1;
        _ObjectConnectedImagesList[typeIndex].gameObject.SetActive(true);
        _ObjectConnectedList.Add(_NewlyConnectedObject);
        _NewlyConnectedObject = Anki.Cozmo.ObjectType.Invalid;
        robot.LightCubes[_NewlyConnectedObjectId].SetLEDs(Color.cyan);
      }
    }
  }

  private void HandleContinueButton() {
    if (ClosingAnimationPlaying) {
      return;
    }
    this.CloseView();
  }

  protected override void CleanUp() {
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
        kvp.Value.SetLEDsOff();
      }
    }
  }
}
