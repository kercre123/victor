using UnityEngine;
using System.Collections;

public class DevHubWorldDialog : BaseDialog {

  public delegate void DevButtonClickedHandler(GameBase miniGameClicked);
  public event DevButtonClickedHandler OnDevButtonClicked;
  private void RaiseButtonClicked(GameBase minigame){
    if (OnDevButtonClicked != null) {
    }
  }

  [SerializeField]
  private DevHubWorldButton devHubWorldButtonPrefab_;

  [SerializeField]
  private RectTransform buttonContainer_;

  public void Initialize(GameBase[] minigames){
  }

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }
}
