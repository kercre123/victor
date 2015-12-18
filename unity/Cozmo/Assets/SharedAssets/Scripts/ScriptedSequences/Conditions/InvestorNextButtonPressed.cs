using UnityEngine;
using System.Collections;

namespace ScriptedSequences.Conditions {
  public class InvestorNextButtonPressed : ScriptedSequenceCondition {
    
    #region implemented abstract members of ScriptedSequenceCondition

    protected override void EnableChanged(bool enabled) {
      GameObject investorNextButton = ObjectTagRegistryManager.Instance.GetObjectByTag("InvestorNextButton");
      investorNextButton.GetComponent<UnityEngine.UI.Button>().onClick.AddListener(HandleOnButtonClicked);
      investorNextButton.transform.Find("Text").GetComponent<UnityEngine.UI.Text>().text = "NEXT";
    }

    #endregion

    private void HandleOnButtonClicked() {
      IsMet = true;
      GameObject investorNextButton = ObjectTagRegistryManager.Instance.GetObjectByTag("InvestorNextButton");
      investorNextButton.transform.Find("Text").GetComponent<UnityEngine.UI.Text>().text = "Waiting...";
    }
  }
}