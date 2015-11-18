using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class HubWorldDialog : BaseDialog {

  public delegate void ButtonClickedHandler(ChallengeData challengeClicked);

  public event ButtonClickedHandler OnButtonClicked;

  [SerializeField]
  private HubWorldButton _HubWorldButtonPrefab;

  [SerializeField]
  private RectTransform _ButtonContainer;

  [SerializeField]
  private ScrollRect _ScrollRect;

  public void Initialize(ChallengeData[] challengeList) {

    GameObject newButton;
    HubWorldButton buttonScript;

    for (int i = 0; i < challengeList.Length; ++i) {
      newButton = UIManager.CreateUI(_HubWorldButtonPrefab.gameObject, _ButtonContainer);
      buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeList[i]);
      buttonScript.OnButtonClicked += HandleOnButtonClicked;
    }

    _ScrollRect.verticalNormalizedPosition = 1.0f;
  }

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }

  private void HandleOnButtonClicked(ChallengeData challengeClicked) {
    if (OnButtonClicked != null) {
      OnButtonClicked(challengeClicked);
    }
  }
}
