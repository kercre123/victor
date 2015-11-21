using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public abstract class ConsoleVarLine : MonoBehaviour {

  [SerializeField]
  protected Text _StatLabel;

  protected DebugConsoleData.DebugConsoleVarData _varData;

  public virtual void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
    _varData = singleVar;
    _StatLabel.text = singleVar._varName;
  }

}
