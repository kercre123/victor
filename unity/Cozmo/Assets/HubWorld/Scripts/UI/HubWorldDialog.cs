using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class HubWorldDialog : BaseDialog {

  public delegate void ButtonClickedHandler(ChallengeData challengeClicked);

  public event ButtonClickedHandler OnButtonClicked;

  [SerializeField]
  private RectTransform _ButtonContainer;

  [SerializeField]
  private ScrollRect _ScrollRect;

  private List<ChallengeData> _ChallengeList;

  public void Initialize(List<ChallengeData> challengeList) {
    _ChallengeList = challengeList;
    // TODO: populate UI with challenge list
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
