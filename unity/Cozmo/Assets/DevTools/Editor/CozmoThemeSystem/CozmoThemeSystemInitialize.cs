using UnityEngine;
using UnityEditor;
using Anki.Core.UI.Components;

[InitializeOnLoad]
public class CozmoThemeSystemInitialize : MonoBehaviour {
  static CozmoThemeSystemInitialize() {
    CozmoThemeSystemUtils.Initialize();
    CozmoThemeSystemEditorUtils.Initialize();

    ThemesJson.LoadJson();
    ThemeSystemConfigJson.LoadJson();
  }
}
