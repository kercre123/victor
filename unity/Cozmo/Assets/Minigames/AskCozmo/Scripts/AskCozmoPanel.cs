using UnityEngine;
using System.Collections;

public class AskCozmoPanel : BaseDialog {

  [SerializeField]
  private UnityEngine.UI.Button askButton_;

  public delegate void AskButtonPressedHandler();

  public AskButtonPressedHandler OnAskButtonPressed;

  void Start() {
    askButton_.onClick.AddListener(OnAskButton);
  }

  void OnAskButton() {
    if (OnAskButtonPressed != null) {
      OnAskButtonPressed();
    }
  }

  protected override void CleanUp() {
    
  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
    
  }
}
