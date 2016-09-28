using UnityEngine;
using UnityEngine.UI;
using System.Reflection;

namespace Anki.Debug {
  public abstract class ConsoleVarLine : MonoBehaviour {

    [SerializeField]
    protected Text _StatLabel;

    protected DebugConsoleData.DebugConsoleVarData _VarData;

    public virtual void Init(DebugConsoleData.DebugConsoleVarData singleVar, GameObject go) {
      _VarData = singleVar;
      _StatLabel.text = singleVar.VarName;
      _VarData.UIAdded = go;
    }

    public void OnDestroy() {
      // Clear UI reference.
      _VarData.UIAdded = null;
    }

    // Engine has confirmed an update
    public virtual void OnValueRefreshed() {
    }

    protected virtual object GetUnityValue() {
      // The debug console can see privates.
      if (_VarData.UnityObject != null) {
        BindingFlags bindFlags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic
                                                       | BindingFlags.Static;
        // Set directly if the value was static else it's a member of an object
        if (_VarData.UnityObject is System.Type) {
          System.Reflection.FieldInfo info = ((System.Type)_VarData.UnityObject).GetField(_VarData.VarName, bindFlags);
          return info.GetValue(null);
        }
        else {
          System.Reflection.FieldInfo info = _VarData.UnityObject.GetType().GetField(_VarData.VarName, bindFlags);
          return info.GetValue(_VarData.UnityObject);
        }
      }
      return null;
    }

    protected virtual void SetUnityValue(object val) {
      // The debug console can see privates.
      BindingFlags bindFlags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic
                                                      | BindingFlags.Static;
      // Set directly if the value was static else it's a member of an object
      if (_VarData.UnityObject is System.Type) {
        System.Reflection.FieldInfo info = ((System.Type)_VarData.UnityObject).GetField(_VarData.VarName, bindFlags);
        info.SetValue(null, val);
        DebugConsoleData.Instance.UnityConsoleDataUpdated(_VarData.VarName);
      }
      else {
        System.Reflection.FieldInfo info = _VarData.UnityObject.GetType().GetField(_VarData.VarName, bindFlags);
        info.SetValue(_VarData.UnityObject, val);
        DebugConsoleData.Instance.UnityConsoleDataUpdated(_VarData.VarName);
      }
    }

  }
}