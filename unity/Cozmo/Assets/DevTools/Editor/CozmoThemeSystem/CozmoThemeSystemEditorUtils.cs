using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;

public class CozmoThemeSystemEditorUtils : Anki.Core.Editor.Components.ThemeSystemEditorUtils {
  #region Static Methods
  public static void Initialize() {
    //Sets our base instance to this, this will make all our override methods below work :)
    _sInstance = new CozmoThemeSystemEditorUtils();
  }

  private const string kGeneratedThemeKeysFilePath =
    "Assets/Scripts/SharedScripts/UI/CozmoThemeSystem/GeneratedKeys/ThemeKeys.cs";

  [MenuItem("Cozmo/Generate Theme Key Constants")]
  private static void GenerateThemeKeyConstFile() {
    EditorUtility.DisplayProgressBar("Generating", "Generating Theme Key Constants", 0);
    try {
      //Get method name so that it's clear where this file was created.
      System.Diagnostics.StackTrace st = new System.Diagnostics.StackTrace();
      string methodName = st.GetFrame(0).GetMethod().Name;

      System.Text.StringBuilder fileContents = new System.Text.StringBuilder(
        string.Format("//Generated File made by {0}, do not edit\npublic static class ThemeKeys {{", methodName));

      Anki.Core.UI.Components.ThemesJson.LoadJson();
      var themeData = Anki.Core.UI.Components.ThemesJson.CurrentlyLoadedInstance;

      for (int k = 0; k < themeData.Themes.Count; k++) {
        var theme = themeData.Themes[k];
        if (theme == null)
          continue;
        fileContents.AppendFormat("\n  \n  public static class {0} {{", theme.Id);

        //Track components in a dictionary so we can group and sort them later.
        Dictionary<Anki.Core.UI.Components.ThemesJson.ComponentTypes,
          List<Anki.Core.UI.Components.ThemesJson.ThemeComponentObj>> componentDict =
          new Dictionary<Anki.Core.UI.Components.ThemesJson.ComponentTypes,
          List<Anki.Core.UI.Components.ThemesJson.ThemeComponentObj>>();

        if (theme.Skins.Count > 0) {

          //Only check default skin, the others will be using duplicate keys.
          var skin = theme.Skins[0];
          for (int i = 0; i < skin.Components.Count; i++) {
            var component = skin.Components[i];
            if (!componentDict.ContainsKey(component.Type)) {
              componentDict[component.Type] = new List<Anki.Core.UI.Components.ThemesJson.ThemeComponentObj>();
            }
            componentDict[component.Type].Add(component);
          }

          foreach (var kvp in componentDict) {
            fileContents.AppendFormat("\n    \n    public static class {0} {{", kvp.Key);
            //Sort ids lexicographically
            kvp.Value.Sort((a, b) => {
              return string.Compare(a.Id, b.Id, true);
            });
            for (int i = 0; i < kvp.Value.Count; i++) {
              fileContents.AppendFormat("\n      public const string {0} = \"{1}\";",
                                            VariableNameFromThemeKey(kvp.Value[i].Id), kvp.Value[i].Id);
            }
            fileContents.Append("\n    }");
          }

        }
        fileContents.Append("\n  }");
      }
      fileContents.Append("\n}");
      EditorUtility.DisplayProgressBar("Generating", "Generating Theme Key Constants", 0.5f);
      File.WriteAllText(kGeneratedThemeKeysFilePath, fileContents.ToString());
      var generatedScript = AssetDatabase.LoadMainAssetAtPath(kGeneratedThemeKeysFilePath);
      EditorGUIUtility.PingObject(generatedScript);
    }
    finally {
      EditorUtility.ClearProgressBar();
    }
  }

  private static string VariableNameFromThemeKey(string themeKey) {
    string variableName = "k";
    char[] delimiter = { '.', '_', ' ' };
    string[] keyParts = themeKey.Split(delimiter, System.StringSplitOptions.RemoveEmptyEntries);
    foreach (string part in keyParts) {
      variableName += char.ToUpper(part[0]) + part.Substring(1);
    }
    return variableName;
  }

  #endregion

  #region Override Methods
  public override string GetAssetResourcesPath(Object obj) {
    return AssetDatabase.GetAssetPath(obj);
  }
  #endregion
}