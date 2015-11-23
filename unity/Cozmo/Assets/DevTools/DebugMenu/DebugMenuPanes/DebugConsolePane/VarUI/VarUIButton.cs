using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

namespace Anki.Debug {
  public class VarUIButton : ConsoleVarLine {

    [SerializeField]
    private Button _Button;

    [SerializeField]
    private InputField _StatInputField;

    public override void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
      base.Init(singleVar);

      _Button.onClick.AddListener(HandleClick);
    }

    public void HandleClick() {
      if (_varData._unityVarHandler != null) {
        _varData._unityVarHandler(_StatInputField.text);
      }
      else {
        RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_varData._varName, _StatInputField.text);
      }
    }
    
  }
}
