using UnityEngine;
using Cozmo.HomeHub;
using System.Collections;

public class DebugMenuManager : MonoBehaviour {

  private const string kDemoModeKey = "DemoMode";

  [SerializeField]
  private DebugMenuDialog _DebugMenuDialogPrefab;
  private DebugMenuDialog _DebugMenuDialogInstance;

  [SerializeField]
  private LatencyCalculator _LatencyCalculator;

  [SerializeField]
  private Canvas _DebugMenuCanvas;

  private int _LastOpenedDebugTab = 0;

  private bool _DebugPause = false;
  private bool _DemoMode = false;

  private static DebugMenuManager _Instance;

  public static DebugMenuManager Instance {
    get {
      return _Instance;
    }
    private set {
      if (_Instance == null) {
        _Instance = value;
      }
    }
  }

  public bool DemoMode {
    get {
      return _DemoMode;
    }
  }

  public Canvas DebugOverlayCanvas {
    get {
      return _DebugMenuCanvas;
    }
  }

  void Start() {
    // TODO: Destroy self if not production
    _Instance = this;

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleDemoModeVariable);
    RobotEngineManager.Instance.Message.GetDebugConsoleVarMessage = new Anki.Cozmo.ExternalInterface.GetDebugConsoleVarMessage(kDemoModeKey);
    RobotEngineManager.Instance.SendMessage();
  }

  void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleDemoModeVariable);
  }

  public void OnDebugMenuButtonTap() {
#if ENABLE_DEBUG_PANEL
    if (FakeTouchManager.Instance != null && !FakeTouchManager.Instance.IsPlayingTouches && !FakeTouchManager.Instance.IsSoakingTouches) {
      CreateDebugDialog();
    }
#endif
  }

  public GameBase GetCurrChallenge() {
    if (HubWorldBase.Instance != null) {
      if (HubWorldBase.Instance.GetChallengeInstance() != null) {
        return HubWorldBase.Instance.GetChallengeInstance();
      }
    }
    return null;
  }

  private void CreateDebugDialog() {
    GameObject debugMenuDialogInstance = GameObject.Instantiate(_DebugMenuDialogPrefab.gameObject);
    Transform dialogTransform = debugMenuDialogInstance.transform;
    dialogTransform.SetParent(_DebugMenuCanvas.transform, false);
    dialogTransform.localPosition = Vector3.zero;
    _DebugMenuDialogInstance = debugMenuDialogInstance.GetComponent<DebugMenuDialog>();
    _DebugMenuDialogInstance.Initialize(_LastOpenedDebugTab);
    _DebugMenuDialogInstance.OnDebugMenuClosed += OnDebugMenuDialogClose;
    if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.DebugPauseEnabled) {
      GameBase game = GetCurrChallenge();
      if (game != null) {
        if (game.Paused == false) {
          game.PauseStateMachine(State.PauseReason.DEBUG_INPUT, Anki.Cozmo.ReactionTrigger.NoneTrigger);
          _DebugPause = true;
        }
      }
    }
  }

  public void CloseDebugMenuDialog() {
    _DebugMenuDialogInstance.OnDebugMenuCloseTap();
  }

  private void OnDebugMenuDialogClose(int lastOpenTab) {
    _DebugMenuDialogInstance.OnDebugMenuClosed -= OnDebugMenuDialogClose;
    _LastOpenedDebugTab = lastOpenTab;
    if (_DebugPause == true) {
      _DebugPause = false;
      GameBase game = GetCurrChallenge();
      if (game != null) {
        if (game.Paused) {
          game.ResumeStateMachine(State.PauseReason.DEBUG_INPUT, Anki.Cozmo.ReactionTrigger.NoneTrigger);
        }
      }
    }
  }

  private void HandleDemoModeVariable(Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage message) {
    if (message.varName == kDemoModeKey && message.success) {
      _DemoMode = message.varValue.varBool != 0;
    }
  }

  public bool IsDialogOpen() {
    return _DebugMenuDialogInstance != null;
  }

  public void EnableLatencyPopup(bool enable, bool forceShowNow = false) {
    _LatencyCalculator.EnableLatencyPopup(enable, forceShowNow);
  }
}
