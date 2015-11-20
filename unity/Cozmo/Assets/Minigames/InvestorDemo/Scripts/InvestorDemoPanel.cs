using UnityEngine;
using System.Collections;

namespace InvestorDemo {
  public class InvestorDemoPanel : BaseView {
    
    [SerializeField]
    private UnityEngine.UI.Button _NextButton;

    [SerializeField]
    private UnityEngine.UI.Button _PrevButton;

    [SerializeField]
    private UnityEngine.UI.Text _CurrentActionText;

    public delegate void ButtonHandler();

    public ButtonHandler OnNextButtonPressed;
    public ButtonHandler OnPrevButtonPressed;

    void Start() {
      _NextButton.onClick.AddListener(HandleNextButton);
      _PrevButton.onClick.AddListener(HandlePrevButton);
    }

    public void SetActionText(string text) {
      _CurrentActionText.text = text;
    }

    private void HandleNextButton() {
      if (OnNextButtonPressed != null) {
        OnNextButtonPressed();
      }
    }

    private void HandlePrevButton() {
      if (OnPrevButtonPressed != null) {
        OnPrevButtonPressed();
      }
    }

    protected override void CleanUp() {

    }

    protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

    }
  }

}
