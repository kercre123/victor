using UnityEngine;
using UnityEngine.UI;
using System.Linq;

namespace Cozmo.HomeHub {
  public class HubWorldPane : MonoBehaviour {

    public delegate void HubWorldPaneOpenHandler(HubWorldPane hubWorldPane);

    public static event HubWorldPaneOpenHandler HubWorldPaneOpened;

    private static void RaiseHubWorldPaneOpened(HubWorldPane hubWorldPane) {
      if (HubWorldPaneOpened != null) {
        HubWorldPaneOpened(hubWorldPane);
      }
    }

    private HomeHub _HubWorldInstance = null;

    [SerializeField]
    private Button _LoadHubWorldButton;

    [SerializeField]
    private Button _CompleteChallengeButton;

    [SerializeField]
    private Dropdown _ChallengeIdSelect;

    [SerializeField]
    private Button _SelfieAlbumButton;

    [SerializeField]
    private Button _CreateMockTrayButton;

    [SerializeField]
    private GameObject _SelfieAlbumPrefab;

    [SerializeField]
    private GameObject _MockTrayPrefab;

    [SerializeField]
    private Cozmo.Challenge.ChallengeDataList _ChallengeDataList;

    private void Start() {
      _ChallengeIdSelect.options = _ChallengeDataList.ChallengeData.Select(c => new Dropdown.OptionData(c.ChallengeID)).ToList();

      _LoadHubWorldButton.onClick.AddListener(HandleLoadHubWorldButtonClicked);
      _CompleteChallengeButton.onClick.AddListener(HandleCompleteChallengeClicked);
      _SelfieAlbumButton.onClick.AddListener(HandleLoadSelfieAlbumButtonClicked);
      _CreateMockTrayButton.onClick.AddListener(HandleCreateMockTrayButtonClicked);


      RaiseHubWorldPaneOpened(this);
    }

    private void OnDestroy() {
      _LoadHubWorldButton.onClick.RemoveListener(HandleLoadHubWorldButtonClicked);
      _CompleteChallengeButton.onClick.RemoveListener(HandleCompleteChallengeClicked);
      _SelfieAlbumButton.onClick.RemoveListener(HandleLoadSelfieAlbumButtonClicked);
      _CreateMockTrayButton.onClick.RemoveListener(HandleCreateMockTrayButtonClicked);
    }

    private void HandleLoadHubWorldButtonClicked() {
      // _HubWorldInstance.TestLoadHubWorld();
    }

    private void HandleLoadSelfieAlbumButtonClicked() {
      UIManager.CreateUIElement(_SelfieAlbumPrefab);
    }

    private void HandleCompleteChallengeClicked() {
      _HubWorldInstance.TestCompleteChallenge(_ChallengeIdSelect.options[_ChallengeIdSelect.value].text);
    }

    private void HandleCreateMockTrayButtonClicked() {
      var mockRobot = RobotEngineManager.Instance.CurrentRobot as MockRobot;

      if (mockRobot != null) {
        // transform.parent.parent relies on the current structure, which may change. we should
        // find the proper canvas by name by walking the parent.
        var tray = UIManager.CreateUIElement(_MockTrayPrefab, FindDebugCanvasParent(transform)).GetComponent<MockRobotTray>();
        tray.Initialize(mockRobot);
      }
    }

    private Transform FindDebugCanvasParent(Transform transform) {
      if (transform == null) {
        return null;
      }
      if (transform.name == "DebugMenuCanvas") {
        return transform;
      }
      return FindDebugCanvasParent(transform.parent);
    }
  }
}