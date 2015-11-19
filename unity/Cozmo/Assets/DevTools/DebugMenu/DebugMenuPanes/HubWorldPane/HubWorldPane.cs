using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class HubWorldPane : MonoBehaviour {

  public delegate void HubWorldPaneOpenHandler(HubWorldPane hubWorldPane);

  public static event HubWorldPaneOpenHandler HubWorldPaneOpened;

  private static void RaiseHubWorldPaneOpened(HubWorldPane hubWorldPane) {
    if (HubWorldPaneOpened != null) {
      HubWorldPaneOpened(hubWorldPane);
    }
  }

  private HubWorld _HubWorldInstance = null;

  [SerializeField]
  private Button _LoadHubWorldButton;

  [SerializeField]
  private Button _CompleteChallengeButton;

  [SerializeField]
  private InputField _ChallengeIdToComplete;

  private void Start() {
    RaiseHubWorldPaneOpened(this);
  }

  private void OnDestroy() {
    _LoadHubWorldButton.onClick.RemoveListener(HandleLoadHubWorldButtonClicked);
    _CompleteChallengeButton.onClick.RemoveListener(HandleCompleteChallengeClicked);
  }

  public void Initialize(HubWorld hubWorld) {
    _HubWorldInstance = hubWorld;

    _LoadHubWorldButton.onClick.AddListener(HandleLoadHubWorldButtonClicked);
    _CompleteChallengeButton.onClick.AddListener(HandleCompleteChallengeClicked);
  }

  private void HandleLoadHubWorldButtonClicked() {
    _HubWorldInstance.TestLoadHubWorld();
  }

  private void HandleCompleteChallengeClicked() {
    _HubWorldInstance.TestCompleteChallenge(_ChallengeIdToComplete.text);
  }
}
