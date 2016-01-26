using System;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

public static class EditorDrawingUtility {

  public static void DrawList<T>(string label, List<T> list, Func<T,T> drawControls, Func<T> createFunc) {

    EditorGUILayout.BeginHorizontal();
    GUILayout.Label(label);
    if (GUILayout.Button("+", GUILayout.Width(30))) {
      list.Insert(0, createFunc());
    }
    EditorGUILayout.EndHorizontal();

    for (int i = 0; i < list.Count; i++) {
      EditorGUILayout.BeginHorizontal();
      list[i] = drawControls(list[i]);


      if (GUILayout.Button("-", GUILayout.Width(30))) {
        list.RemoveAt(i);
        i--;
      }

      if (GUILayout.Button("+", GUILayout.Width(30))) {        
        list.Insert(i + 1, createFunc());
      }

      EditorGUILayout.EndHorizontal();
    }
  }

  public static void DrawListWithIndex<T>(string label, List<T> list, Func<T, int, T> drawControls, Func<int, T> createFunc) {

    EditorGUILayout.BeginHorizontal();
    GUILayout.Label(label);
    if (GUILayout.Button("+", GUILayout.Width(30))) {
      list.Insert(0, createFunc(0));
    }
    EditorGUILayout.EndHorizontal();

    for (int i = 0; i < list.Count; i++) {
      EditorGUILayout.BeginHorizontal();
      list[i] = drawControls(list[i], i);


      if (GUILayout.Button("-", GUILayout.Width(30))) {
        list.RemoveAt(i);
        i--;
      }

      if (GUILayout.Button("+", GUILayout.Width(30))) {        
        list.Insert(i + 1, createFunc(i + 1));
      }

      EditorGUILayout.EndHorizontal();
    }
  }


  public static void InitializeLocalizationString(string localizationKey, out string localizedStringFile, out string localizedString) {
    localizedString = LocalizationEditorUtility.GetTranslation(localizationKey, out localizedStringFile);
  }

  public static void DrawLocalizationString(ref string localizationKey, ref string localizedStringFile, ref string localizedString) {
    int selectedFileIndex = EditorGUILayout.Popup("Localization File", 
                              Mathf.Max(0, 
                                System.Array.IndexOf(
                                  LocalizationEditorUtility.LocalizationFiles, 
                                  localizedStringFile)), 
                              LocalizationEditorUtility.LocalizationFiles);
    localizedStringFile = LocalizationEditorUtility.LocalizationFiles[selectedFileIndex];
       
    var lastLocKey = localizationKey;
    localizationKey = EditorGUILayout.TextField("Localization Key", localizationKey);

    if (localizationKey != lastLocKey && string.IsNullOrEmpty(localizedString)) {
      InitializeLocalizationString(localizationKey, out localizedStringFile, out localizedString);
    }

    localizedString = GUILayout.TextArea(localizedString);

    if (LocalizationEditorUtility.GetTranslation(localizedStringFile, localizationKey) != localizedString) {
      EditorGUILayout.BeginHorizontal();
      if (GUILayout.Button("Reset")) {
        localizedString = LocalizationEditorUtility.GetTranslation(localizedStringFile, localizationKey);
      }
      if (GUILayout.Button("Save")) {
        LocalizationEditorUtility.SetTranslation(localizedStringFile, localizationKey, localizedString);
      }
      EditorGUILayout.EndHorizontal();
    }
  }
}

