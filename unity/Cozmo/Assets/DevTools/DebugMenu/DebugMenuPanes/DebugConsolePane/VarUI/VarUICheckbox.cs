using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class VarUICheckbox : ConsoleVarLine {

  [SerializeField]
  private Toggle _Checkbox;

  public override void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
    base.Init(singleVar);

    _Checkbox.onValueChanged.AddListener(HandleValueChanged);

    _Checkbox.isOn = singleVar._valueAsUInt64 != 0;
  }

  private void HandleValueChanged(bool val) {
    // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
    // otherwise it will send another Set to a valid value.
    // Empty string just means toggle.
    RobotEngineManager.Instance.SetDebugConsoleVar(_varData._varName, "");
  }
}
