using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class ConsoleVarLine : MonoBehaviour {

  [SerializeField]
  private InputField _StatInputField;

  [SerializeField]
  private Text _StatLabel;
  DebugConsoleData.DebugConsoleVarData _varData;

  public void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
    _StatLabel.text = singleVar._varName;

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
    _varData = singleVar;
    _StatInputField.onValueChange.AddListener(HandleValueChanged);
  }

  private void HandleValueChanged(string strValue) {
    // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
    // otherwise it will send another Set to a valid value.
    RobotEngineManager.Instance.SetDebugConsoleVar(_varData._varName, strValue);
  }
}
