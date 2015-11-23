using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

namespace Anki.Debug {
  public class VarUIText : ConsoleVarLine {

    public static int gTestValue = 3;

    [SerializeField]
    private InputField _StatInputField;

    public override void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
      base.Init(singleVar);

      switch (singleVar._tagType) {
      case ConsoleVarUnion.Tag.varDouble:
        _StatInputField.text = singleVar._valueAsDouble.ToString();
        break;
      case ConsoleVarUnion.Tag.varInt:
        _StatInputField.text = singleVar._valueAsInt64.ToString();
        break;
      case ConsoleVarUnion.Tag.varUint:
        _StatInputField.text = singleVar._valueAsUInt64.ToString();
        break;
      }

      _StatInputField.onValueChange.AddListener(HandleValueChanged);
    }

    private void HandleValueChanged(string strValue) {

      // If it's a unity variable, pass back with the same type.
      if (_varData._unityVarHandler != null) {
        switch (_varData._tagType) {
        case ConsoleVarUnion.Tag.varDouble:
          {
            double result;
            if (double.TryParse(strValue, out result)) {
              _varData._unityVarHandler(result);
            }
          }
          break;
        case ConsoleVarUnion.Tag.varInt:
          {
            int result;
            if (int.TryParse(strValue, out result)) {
              _varData._unityVarHandler(result);
            }
          }
          break;
        case ConsoleVarUnion.Tag.varUint:
          {
            uint result;
            if (uint.TryParse(strValue, out result)) {
              _varData._unityVarHandler(result);
            }
          }
          break;
        }

      }
    // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
    // otherwise it will send another Set to a valid value.
    else {
        RobotEngineManager.Instance.SetDebugConsoleVar(_varData._varName, strValue);
      }
    }
  }
}
