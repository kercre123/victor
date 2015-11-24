using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class DevHubWorldDialog : BaseView {

  public delegate void DevButtonClickedHandler(ChallengeData challenge);

  public event DevButtonClickedHandler OnDevButtonClicked;

  private void RaiseButtonClicked(ChallengeData challenge) {
    if (OnDevButtonClicked != null) {
      OnDevButtonClicked(challenge);
    }
  }

  [SerializeField]
  private DevHubWorldButton _DevHubWorldButtonPrefab;

  [SerializeField]
  private RectTransform _ButtonContainer;

  [SerializeField]
  private ScrollRect _ScrollRect;

  public void Initialize(ChallengeDataList challengeList) {
    GameObject newButton;
    DevHubWorldButton buttonScript;
    foreach (ChallengeData challenge in challengeList.ChallengeData) {
      newButton = UIManager.CreateUIElement(_DevHubWorldButtonPrefab.gameObject, _ButtonContainer);
      buttonScript = newButton.GetComponent<DevHubWorldButton>();
      buttonScript.Initialize(challenge);
      buttonScript.OnDevButtonClicked += HandleOnDevButtonClicked;
    }
    _ScrollRect.verticalNormalizedPosition = 1.0f;
  }

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }

  private void HandleOnDevButtonClicked(ChallengeData challenge) {
    RaiseButtonClicked(challenge);
  }
}
