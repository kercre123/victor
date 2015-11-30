using UnityEngine;
using System.Collections;

namespace InvestorDemo {
  public class InvestorDemoPanel : BaseView {
    
    [SerializeField]
    private UnityEngine.UI.Button _NextButton;

    [SerializeField]
    private UnityEngine.UI.Text _CurrentActionText;

    void Start() {
      ObjectTagRegistryManager.Instance.RegisterTag("InvestorNextButton", _NextButton.gameObject);
    }

    public void SetActionText(string text) {
      _CurrentActionText.text = text;
    }

    protected override void CleanUp() {

    }
  }

}
