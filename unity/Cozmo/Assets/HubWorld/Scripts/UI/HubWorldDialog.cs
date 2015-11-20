using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class HubWorldDialog : BaseView {

  public delegate void ButtonClickedHandler(string challengeClicked);

  public event ButtonClickedHandler OnLockedChallengeClicked;
  public event ButtonClickedHandler OnUnlockedChallengeClicked;
  public event ButtonClickedHandler OnCompletedChallengeClicked;

  [SerializeField]
  private HubWorldButton _HubWorldButtonPrefab;

  [SerializeField]
  private RectTransform _LockedButtonContainer;

  [SerializeField]
  private ScrollRect _LockedScrollRect;

  [SerializeField]
  private RectTransform _UnlockedButtonContainer;

  [SerializeField]
  private ScrollRect _UnlockedScrollRect;

  [SerializeField]
  private RectTransform _CompletedButtonContainer;

  [SerializeField]
  private ScrollRect _CompletedScrollRect;

  public void Initialize(Dictionary<string, ChallengeStatePacket> _challengeStatesById) {
    // For all the challenges
    foreach (ChallengeStatePacket challengeState in _challengeStatesById.Values) {
      // Create the correct button at the correct spot based on current state
      switch (challengeState.currentState) {
      case ChallengeState.LOCKED:
        CreateLockedButton(challengeState.data);
        break;
      case ChallengeState.UNLOCKED:
        CreateUnlockedButton(challengeState.data, challengeState.unlockProgress);
        break;
      case ChallengeState.COMPLETED:
        CreateCompletedButton(challengeState.data);
        break;
      default:
        DAS.Error("HubWorldDialog", "ChallengeState view not implemented! " + challengeState);
        break;
      }
    }

    // Slide all teh
    _LockedScrollRect.verticalNormalizedPosition = 1.0f;
    _UnlockedScrollRect.verticalNormalizedPosition = 1.0f;
    _CompletedScrollRect.verticalNormalizedPosition = 1.0f;
  }

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }

  private void CreateLockedButton(ChallengeData challengeData) {
    GameObject newButton = UIManager.CreateUIElement(_HubWorldButtonPrefab.gameObject, _LockedButtonContainer);
    HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
    buttonScript.Initialize(challengeData.ChallengeID, challengeData.ChallengeTitleKey);
    buttonScript.OnButtonClicked += HandleLockedChallengeClicked;
  }

  private void CreateUnlockedButton(ChallengeData challengeData, float unlockProgress) {
    GameObject newButton = UIManager.CreateUIElement(_HubWorldButtonPrefab.gameObject, _UnlockedButtonContainer);
    HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
    buttonScript.Initialize(challengeData.ChallengeID, challengeData.ChallengeTitleKey);
    buttonScript.OnButtonClicked += HandleUnlockedChallengeClicked;
  }

  private void CreateCompletedButton(ChallengeData challengeData) {
    GameObject newButton = UIManager.CreateUIElement(_HubWorldButtonPrefab.gameObject, _CompletedButtonContainer);
    HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
    buttonScript.Initialize(challengeData.ChallengeID, challengeData.ChallengeTitleKey);
    buttonScript.OnButtonClicked += HandleCompletedChallengeClicked;
  }

  private void HandleLockedChallengeClicked(string challengeClicked) {
    if (OnLockedChallengeClicked != null) {
      OnLockedChallengeClicked(challengeClicked);
    }
  }

  private void HandleUnlockedChallengeClicked(string challengeClicked) {
    if (OnUnlockedChallengeClicked != null) {
      OnUnlockedChallengeClicked(challengeClicked);
    }
  }

  private void HandleCompletedChallengeClicked(string challengeClicked) {
    if (OnCompletedChallengeClicked != null) {
      OnCompletedChallengeClicked(challengeClicked);
    }
  }
}
