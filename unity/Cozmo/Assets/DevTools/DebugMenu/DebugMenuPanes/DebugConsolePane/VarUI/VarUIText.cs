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

      switch (singleVar.TagType) {
      case ConsoleVarUnion.Tag.varDouble:
        _StatInputField.text = singleVar.ValueAsDouble.ToString();
        break;
      case ConsoleVarUnion.Tag.varInt:
        _StatInputField.text = singleVar.ValueAsInt64.ToString();
        break;
      case ConsoleVarUnion.Tag.varUint:
        _StatInputField.text = singleVar.ValueAsUInt64.ToString();
        break;
      }

      _StatInputField.onValueChange.AddListener(HandleValueChanged);
    }

    private void HandleValueChanged(string strValue) {

      // If it's a unity variable, pass back with the same type.
      if (_VarData.UnityVarHandler != null) {
        switch (_VarData.TagType) {
        case ConsoleVarUnion.Tag.varDouble:
          {
            double result;
            if (double.TryParse(strValue, out result)) {
              _VarData.UnityVarHandler(result);
            }
          }
          break;
        case ConsoleVarUnion.Tag.varInt:
          {
            int result;
            if (int.TryParse(strValue, out result)) {
              _VarData.UnityVarHandler(result);
            }
          }
          break;
        case ConsoleVarUnion.Tag.varUint:
          {
            uint result;
            if (uint.TryParse(strValue, out result)) {
              _VarData.UnityVarHandler(result);
            }
          }
          break;
        }

      }
    // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
    // otherwise it will send another Set to a valid value.
    else {
        RobotEngineManager.Instance.SetDebugConsoleVar(_VarData.VarName, strValue);
      }
    }
  }
}
