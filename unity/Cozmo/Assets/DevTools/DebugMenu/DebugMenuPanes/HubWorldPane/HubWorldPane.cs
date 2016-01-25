using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
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

    private void Start() {
      RaiseHubWorldPaneOpened(this);
    }

    private void OnDestroy() {
      _LoadHubWorldButton.onClick.RemoveListener(HandleLoadHubWorldButtonClicked);
      _CompleteChallengeButton.onClick.RemoveListener(HandleCompleteChallengeClicked);
    }

    public void Initialize(HomeHub hubWorld) {
      _HubWorldInstance = hubWorld;

      var challengeDataList = typeof(HomeHub).GetField("_ChallengeDataList", 
                                System.Reflection.BindingFlags.NonPublic |
                                System.Reflection.BindingFlags.Instance)
                                              .GetValue(hubWorld) as ChallengeDataList;

      _ChallengeIdSelect.options = challengeDataList.ChallengeData.Select(c => new Dropdown.OptionData(c.ChallengeID)).ToList();

      _LoadHubWorldButton.onClick.AddListener(HandleLoadHubWorldButtonClicked);
      _CompleteChallengeButton.onClick.AddListener(HandleCompleteChallengeClicked);
    }

    private void HandleLoadHubWorldButtonClicked() {
      // _HubWorldInstance.TestLoadHubWorld();
    }

    private void HandleCompleteChallengeClicked() {
      _HubWorldInstance.TestCompleteChallenge(_ChallengeIdSelect.options[_ChallengeIdSelect.value].text);
    }
  }
}