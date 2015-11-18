using UnityEngine;
using System.Collections;

namespace InvestorDemo {
  public class InvestorDemoPanel : BaseDialog {


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
      _NextButton.onClick.AddListener(OnNextButton);
      _PrevButton.onClick.AddListener(OnPrevButton);
    }

    public void SetActionText(string text) {
      _CurrentActionText.text = text;
    }

    private void OnNextButton() {
      if (OnNextButtonPressed != null) {
        OnNextButtonPressed();
      }
    }

    private void OnPrevButton() {
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
