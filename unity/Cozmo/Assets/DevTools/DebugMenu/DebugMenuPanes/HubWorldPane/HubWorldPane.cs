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


    [SerializeField]
    private Button _SelfieAlbumButton;


    [SerializeField]
    private GameObject _SelfieAlbumPrefab;

    [SerializeField]
    private ChallengeDataList _ChallengeDataList;

    private void Start() {
      _ChallengeIdSelect.options = _ChallengeDataList.ChallengeData.Select(c => new Dropdown.OptionData(c.ChallengeID)).ToList();

      _LoadHubWorldButton.onClick.AddListener(HandleLoadHubWorldButtonClicked);
      _CompleteChallengeButton.onClick.AddListener(HandleCompleteChallengeClicked);
      _SelfieAlbumButton.onClick.AddListener(HandleLoadSelfieAlbumButtonClicked);


      RaiseHubWorldPaneOpened(this);
    }

    private void OnDestroy() {
      _LoadHubWorldButton.onClick.RemoveListener(HandleLoadHubWorldButtonClicked);
      _CompleteChallengeButton.onClick.RemoveListener(HandleCompleteChallengeClicked);
      _SelfieAlbumButton.onClick.RemoveListener(HandleLoadSelfieAlbumButtonClicked);
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
  }
}