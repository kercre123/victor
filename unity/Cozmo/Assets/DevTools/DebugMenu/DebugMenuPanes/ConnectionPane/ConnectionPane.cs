using UnityEngine;
using Anki.Cozmo.ExternalInterface;
using Cozmo;
using Cozmo.ConnectionFlow;

public class ConnectionPane : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Button _DisconnectButton;

  [SerializeField]
  private UnityEngine.UI.Button _LowBatteryButton;

  [SerializeField]
  private UnityEngine.UI.Button _TooDarkButton;

  [SerializeField]
  private UnityEngine.UI.Button _TooBrightButton;

  [SerializeField]
  private UnityEngine.UI.Button _GoodLightButton;

  // Use this for initialization
  void Start() {
    _DisconnectButton.onClick.AddListener(OnDisconnectButtonClicked);
    _LowBatteryButton.onClick.AddListener(OnLowBatteryButtonClicked);
    _TooDarkButton.onClick.AddListener(OnForceTooDarkButtonClicked);
    _TooBrightButton.onClick.AddListener(OnForceTooBrightButtonClicked);
    _GoodLightButton.onClick.AddListener(OnForceLightingGoodButtonClicked);
  }

  private void OnDisconnectButtonClicked() {
    DebugMenuManager.Instance.CloseDebugMenuDialog();
    if (IntroManager.Instance != null) {
      IntroManager.Instance.ForceBoot();
    }
    else if (NeedsConnectionManager.Instance != null) {
      NeedsConnectionManager.Instance.ForceBoot();
    }
  }

  private void OnLowBatteryButtonClicked() {
#if ENABLE_DEBUG_PANEL
    DebugMenuManager.Instance.CloseDebugMenuDialog();
    PauseManager.Instance.FakeBatteryAlert();
#endif
  }

  private void OnForceTooDarkButtonClicked() {
    MessageEngineToGame messageEngineToGame = new MessageEngineToGame();
    EngineErrorCodeMessage errorCodeMsg = new EngineErrorCodeMessage();
    errorCodeMsg.Initialize(Anki.Cozmo.EngineErrorCode.ImageQualityTooDark);
    messageEngineToGame.EngineErrorCodeMessage = errorCodeMsg;
    RobotEngineManager.Instance.MockCallback(messageEngineToGame);
  }

  private void OnForceTooBrightButtonClicked() {
    MessageEngineToGame messageEngineToGame = new MessageEngineToGame();
    EngineErrorCodeMessage errorCodeMsg = new EngineErrorCodeMessage();
    errorCodeMsg.Initialize(Anki.Cozmo.EngineErrorCode.ImageQualityTooBright);
    messageEngineToGame.EngineErrorCodeMessage = errorCodeMsg;
    RobotEngineManager.Instance.MockCallback(messageEngineToGame);
  }

  private void OnForceLightingGoodButtonClicked() {
    MessageEngineToGame messageEngineToGame = new MessageEngineToGame();
    EngineErrorCodeMessage errorCodeMsg = new EngineErrorCodeMessage();
    errorCodeMsg.Initialize(Anki.Cozmo.EngineErrorCode.ImageQualityGood);
    messageEngineToGame.EngineErrorCodeMessage = errorCodeMsg;
    RobotEngineManager.Instance.MockCallback(messageEngineToGame);
  }
}
