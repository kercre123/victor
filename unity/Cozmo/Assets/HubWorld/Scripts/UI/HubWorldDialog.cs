using UnityEngine;
using System.Collections;

public class HubWorldDialog : BaseDialog {

  public delegate void ButtonClickedHandler(GameBase miniGameClicked);

  public event ButtonClickedHandler OnButtonClicked;

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }

  private void HandleOnDevButtonClicked(GameBase minigame) {
    if (OnButtonClicked != null) {
      OnButtonClicked(minigame);
    }
  }
}
