using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class VarUIText : ConsoleVarLine {

  [SerializeField]
  private InputField _StatInputField;

  public override void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
    base.Init(singleVar);

    switch (singleVar._tagType) {
    case consoleVarUnion.Tag.varDouble:
      _StatInputField.text = singleVar._valueAsDouble.ToString();
      break;
    case consoleVarUnion.Tag.varInt:
      _StatInputField.text = singleVar._valueAsInt64.ToString();
      break;
    case consoleVarUnion.Tag.varUint:
      _StatInputField.text = singleVar._valueAsUInt64.ToString();
      break;
    }

    _StatInputField.onValueChange.AddListener(HandleValueChanged);
  }

  private void HandleValueChanged(string strValue) {
    // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
    // otherwise it will send another Set to a valid value.
    RobotEngineManager.Instance.SetDebugConsoleVar(_varData._varName, strValue);
  }
}
