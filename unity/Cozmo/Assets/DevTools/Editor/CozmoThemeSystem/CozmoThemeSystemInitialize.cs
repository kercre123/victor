using UnityEngine;
using UnityEditor;

[InitializeOnLoad]
public class CozmoThemeSystemInitialize : MonoBehaviour {
  static CozmoThemeSystemInitialize() {
    CozmoThemeSystemUtils.Initialize();
    CozmoThemeSystemEditorUtils.Initialize();
  }
}
