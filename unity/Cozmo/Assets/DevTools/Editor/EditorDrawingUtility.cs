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

    int quickSelect = EditorGUILayout.Popup("   ", 0, LocalizationEditorUtility.LocalizationKeys);
    if (quickSelect > 0) {
      localizationKey = LocalizationEditorUtility.LocalizationKeys[quickSelect];
      localizedString = string.Empty;
    }
      
    if (localizationKey != lastLocKey && (string.IsNullOrEmpty(localizedString) || 
        localizedString == LocalizationEditorUtility.GetTranslation(localizedStringFile, lastLocKey))) {
      InitializeLocalizationString(localizationKey, out localizedStringFile, out localizedString);
    }

    GUILayout.Label("Text");
    localizedString = GUILayout.TextArea(localizedString, GUILayout.Height(45));

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

  public static EmotionScorer DrawEmotionScorer(EmotionScorer emotionScorer) {
    EditorGUILayout.BeginVertical();

    emotionScorer.EmotionType = (Anki.Cozmo.EmotionType)EditorGUILayout.EnumPopup("Emotion", emotionScorer.EmotionType);

    int trackDeltaMultiplier = emotionScorer.TrackDelta ? 2 : 1;
    emotionScorer.GraphEvaluator = EditorGUILayout.CurveField("Score Graph", emotionScorer.GraphEvaluator, Color.green, new Rect(-1 * trackDeltaMultiplier, 0, 2 * trackDeltaMultiplier, 1));

    emotionScorer.TrackDelta = EditorGUILayout.Toggle("Track Delta", emotionScorer.TrackDelta);

    EditorGUILayout.EndVertical();
    return emotionScorer;
  }

  // custom Unity Style for a toolbar button
  private static GUIStyle _ToolbarButtonStyle;

  public static GUIStyle ToolbarButtonStyle {
    get {
      if (_ToolbarButtonStyle == null) {
        _ToolbarButtonStyle = new GUIStyle(EditorStyles.toolbarButton);
        _ToolbarButtonStyle.normal.textColor = Color.white;
        _ToolbarButtonStyle.active.textColor = Color.white;
      }
      return _ToolbarButtonStyle;
    }
  }

  // custom Unity Style for a toolbar button
  private static GUIStyle _ToolbarStyle;

  public static GUIStyle ToolbarStyle {
    get {
      if (_ToolbarStyle == null) {
        _ToolbarStyle = new GUIStyle(EditorStyles.toolbar);
        _ToolbarStyle.normal.textColor = Color.white;
        _ToolbarStyle.active.textColor = Color.white;
      }
      return _ToolbarStyle;
    }
  }
}

