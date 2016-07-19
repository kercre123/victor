using UnityEngine;
using UnityEngine.UI;
using Anki.Cozmo;

namespace Anki.Debug {
  public class VarUIText : ConsoleVarLine {

    [SerializeField]
    private InputField _StatInputField;

    public override void Init(DebugConsoleData.DebugConsoleVarData singleVar, GameObject go) {
      base.Init(singleVar, go);

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

      if (singleVar.UnityObject != null) {
        _StatInputField.text = GetUnityValue().ToString();
      }

      _StatInputField.onValueChanged.AddListener(HandleValueChanged);
    }

    private void HandleValueChanged(string strValue) {
      // If it's a unity variable, pass back with the same type.
      if (_VarData.UnityObject != null) {
        switch (_VarData.TagType) {
        case ConsoleVarUnion.Tag.varDouble: {
            float result;
            if (float.TryParse(strValue, out result)) {
              SetUnityValue(result);
            }
          }
          break;
        case ConsoleVarUnion.Tag.varInt: {
            int result;
            if (int.TryParse(strValue, out result)) {
              SetUnityValue(result);
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
