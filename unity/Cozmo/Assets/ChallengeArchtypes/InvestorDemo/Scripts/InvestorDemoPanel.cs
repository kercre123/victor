using UnityEngine;
using System.Collections;
using Cozmo.UI;

namespace InvestorDemo {
  public class InvestorDemoPanel : BaseView {
    
    [SerializeField]
    private UnityEngine.UI.Button _NextButton;

    [SerializeField]
    private UnityEngine.UI.Text _CurrentActionText;

    void OnEnable() {
      ObjectTagRegistryManager.Instance.RegisterTag("InvestorNextButton", _NextButton.gameObject);
    }

    void OnDisable() {
      ObjectTagRegistryManager.Instance.RemoveTag("InvestorNextButton");
    }

    public void SetActionText(string text) {
      _CurrentActionText.text = text;
    }

    protected override void CleanUp() {

    }
  }

}
