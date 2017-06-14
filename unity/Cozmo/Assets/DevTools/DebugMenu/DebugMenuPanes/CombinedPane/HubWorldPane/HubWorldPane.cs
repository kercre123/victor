using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.HomeHub {
  public class HubWorldPane : MonoBehaviour {

    [SerializeField]
    private Button _CreateMockTrayButton;

    [SerializeField]
    private GameObject _MockTrayPrefab;

    [SerializeField]
    private Cozmo.Challenge.ChallengeDataList _ChallengeDataList;

    private void Start() {
      _CreateMockTrayButton.onClick.AddListener(HandleCreateMockTrayButtonClicked);
    }

    private void OnDestroy() {
      _CreateMockTrayButton.onClick.RemoveListener(HandleCreateMockTrayButtonClicked);
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

    private Transform FindDebugCanvasParent(Transform target) {
      if (target == null) {
        return null;
      }
      if (target.name == "DebugMenuCanvas") {
        return target;
      }
      return FindDebugCanvasParent(target.parent);
    }
  }
}