using UnityEngine;
using System.Collections;

namespace AskCozmo {

  public class AskCozmoPanel : BaseView {

    [SerializeField]
    private UnityEngine.UI.Button _AskButton;

    public delegate void AskButtonPressedHandler();

    public AskButtonPressedHandler OnAskButtonPressed;

    void Start() {
      _AskButton.onClick.AddListener(OnAskButton);
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

}
