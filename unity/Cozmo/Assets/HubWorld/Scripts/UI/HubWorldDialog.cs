using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class HubWorldDialog : BaseDialog {

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

    GameObject newButton;
    HubWorldButton buttonScript;

    // TODO: For all the challenges
    // Create the correct button at the correct spot based on current state

    /*for (int i = 0; i < unlockedChallenges.Count; ++i) {
      newButton = UIManager.CreateUI(_HubWorldButtonPrefab.gameObject, _UnlockedButtonContainer);
      buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(unlockedChallenges[i]);
      buttonScript.OnButtonClicked += HandleUnlockedChallengeClicked;
    }*/

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
    // TODO: Create the button
  }

  private void CreateUnlockedButton(ChallengeData challengeData, float unlockProgress) {
    // TODO: Create the button
  }

  private void CreateCompletedButton(ChallengeData challengeData) {
    // TODO; Create the button
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
