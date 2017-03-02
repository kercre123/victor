using UnityEngine;
using UnityEditor;
using System.IO;

public class CozmoThemeSystemEditorUtils : Anki.Core.Editor.Components.ThemeSystemEditorUtils {
  #region Static Methods
  public static void Initialize() {
    //Sets our base instance to this, this will make all our override methods below work :)
    _sInstance = new CozmoThemeSystemEditorUtils();
  }
  #endregion

  #region Override Methods
  public override string GetAssetResourcesPath(Object obj) {
    return AssetDatabase.GetAssetPath(obj);
  }
  #endregion
}