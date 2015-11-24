using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

namespace Anki.Debug {
  public abstract class ConsoleVarLine : MonoBehaviour {

    [SerializeField]
    protected Text _StatLabel;

    protected DebugConsoleData.DebugConsoleVarData _VarData;

    public virtual void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
      _VarData = singleVar;
      _StatLabel.text = singleVar.VarName;
    }

  }
}