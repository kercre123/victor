using UnityEngine;
using System.Collections;

namespace ScriptedSequences.Conditions {
  public class InvestorNextButtonPressed : ScriptedSequenceCondition {
    
    #region implemented abstract members of ScriptedSequenceCondition

    protected override void EnableChanged(bool enabled) {
      GameObject investorNextButton = ObjectTagRegistryManager.Instance.GetObjectByTag("InvestorNextButton");
      investorNextButton.GetComponent<UnityEngine.UI.Button>().onClick.AddListener(HandleOnButtonClicked);
    }

    #endregion

    private void HandleOnButtonClicked() {
      IsMet = true;
    }
  }
}