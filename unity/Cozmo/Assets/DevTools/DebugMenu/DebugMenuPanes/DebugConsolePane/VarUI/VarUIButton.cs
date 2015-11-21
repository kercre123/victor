using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class VarUIButton : ConsoleVarLine {

  [SerializeField]
  private Button _Button;

  [SerializeField]
  private InputField _StatInputField;

  public override void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
    base.Init(singleVar);

  }
    
}
