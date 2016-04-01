using UnityEngine;
using System.Collections;
using UnityEditor;
using System;
using System.Collections.Generic;
using System.Linq;
using AnimationGroups;
using System.IO;
using Newtonsoft.Json;
using Anki.Cozmo;

public class AnimationGroupEventEditor : EditorWindow {

  private static string[] _AnimationGroupFiles;

  private static string[] _AnimationGroupNameOptions;

  private static string[] _EventMapFiles;

  private static string[] _EventMapNameOptions;

  public static string[] _EventMap = new string[(int)GameEvents.Count];

  private static AnimationEventMap _CurrentEventMap;
  private static string _CurrentEventMapFile;
  private static string _CurrentEventMapName;

  private static Vector2 _scrollPos = new Vector2();


  private static readonly HashSet<string> _RecentFiles = new HashSet<string>();

  public static string sAnimationGroupDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/animationGroups/"; } }

  public static string sEventMapDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/animationGroupMaps"; } }

  public static string[] AnimationGroupNameOptions { get { return _AnimationGroupNameOptions; } }

  static AnimationGroupEventEditor() {
    LoadData();
  }

  private static void LoadData() {
    // Load All Animation Groups for reference
    if (Directory.Exists(sAnimationGroupDirectory)) {
      _AnimationGroupFiles = Directory.GetFiles(sAnimationGroupDirectory);
      _AnimationGroupNameOptions = _AnimationGroupFiles.Select(x => Path.GetFileNameWithoutExtension(x)).ToArray();
    }
    else {
      _AnimationGroupFiles = new string[0];
      _AnimationGroupNameOptions = _AnimationGroupFiles;
    }
    // Load all Event Map Configs (Can have multiple, so you can create different configs, game only uses one.)
    if (Directory.Exists(sEventMapDirectory)) {
      _EventMapFiles = Directory.GetFiles(sEventMapDirectory);
      _EventMapNameOptions = _EventMapFiles.Select(x => Path.GetFileNameWithoutExtension(x)).ToArray();
    }
    else {
      _EventMapFiles = new string[0];
      _EventMapNameOptions = _EventMapFiles;
    }

  }

  // TODO : UPDATE
  private bool CheckDiscardUnsaved() {
    bool canOpen = true;
    if (_CurrentEventMap != null && (string.IsNullOrEmpty(_CurrentEventMapFile) || JsonConvert.SerializeObject(_CurrentEventMap, Formatting.Indented, GlobalSerializerSettings.JsonSettings) != File.ReadAllText(_CurrentEventMapFile))) {
      canOpen = EditorUtility.DisplayDialog("Warning", "Opening a different EventMap will Discard Unsaved Changes. Are you Sure?", "Yes", "No");
    }
    return canOpen;
  }

  private void LoadFile(string path) {
    if (CheckDiscardUnsaved()) {
      try {
        _CurrentEventMap = null;
        _CurrentEventMapFile = null;

        string json = File.ReadAllText(path);
        _CurrentEventMap = JsonConvert.DeserializeObject<AnimationEventMap>(json, GlobalSerializerSettings.JsonSettings);
        _CurrentEventMapName = Path.GetFileNameWithoutExtension(path);
        _CurrentEventMapFile = path;
        _RecentFiles.Add(path);
      }
      catch (Exception ex) {
        DAS.Error(this, ex.Message);
      }
    }
  }

  // TODO : UPDATE
  public void OnGUI() {
    DrawToolbar();

    if (_CurrentEventMap != null) {

      _CurrentEventMapName = EditorGUILayout.TextField("Name", _CurrentEventMapName ?? string.Empty);

      DrawEventMap(_CurrentEventMap);
    }
  }

  // TODO : UPDATE
  private void DrawToolbar() {

    EditorGUILayout.BeginHorizontal(EditorDrawingUtility.ToolbarStyle);

    if (GUILayout.Button("Load", EditorDrawingUtility.ToolbarButtonStyle)) {
      GenericMenu menu = new GenericMenu();

      foreach (var file in _EventMapFiles.Where(x => x.EndsWith(".json"))) {
        Action<string> closureAction = (string f) => {

          menu.AddItem(new GUIContent(Path.GetFileNameWithoutExtension(f)), false, () => {
            LoadFile(f);
          });
        };
        closureAction(file);
      }
      menu.ShowAsContext();
    }

    if (_RecentFiles.Count > 0 && GUILayout.Button("Load Recent", EditorDrawingUtility.ToolbarButtonStyle)) {

      GenericMenu menu = new GenericMenu();

      foreach (var file in _RecentFiles) {
        Action<string> closureAction = (string f) => {

          menu.AddItem(new GUIContent(Path.GetFileNameWithoutExtension(f)), false, () => {
            LoadFile(f);
          });
        };
        closureAction(file);
      }
      menu.ShowAsContext();
    }
    // New Event Map Button
    if (GUILayout.Button("New EventToAnimationMap", EditorDrawingUtility.ToolbarButtonStyle)) {
      if (CheckDiscardUnsaved()) {
        _CurrentEventMap = new AnimationEventMap();
        _CurrentEventMap.NewMap();
        GUI.FocusControl("EditNameField");
      }
    }
    // Save Current Event Map Button
    if (_CurrentEventMap != null) {
      if (GUILayout.Button("Save", EditorDrawingUtility.ToolbarButtonStyle)) {         
        if (string.IsNullOrEmpty(_CurrentEventMapFile)) {
          _CurrentEventMapFile = EditorUtility.SaveFilePanel("Save EventMap", sEventMapDirectory, _CurrentEventMapName, "json");
        }

        if (!string.IsNullOrEmpty(_CurrentEventMapFile)) {
          if (_CurrentEventMap.Pairs.Count == 0) {
            EditorUtility.DisplayDialog("Error!", "Cannot save a map with no Pairs!", "Ok");
          }
          else {
            string newFileName = Path.Combine(sEventMapDirectory, _CurrentEventMapName + ".json");

            bool good = true;
            if (Path.GetFileName(_CurrentEventMapFile) != Path.GetFileName(newFileName)) {
              if (EditorUtility.DisplayDialog("Alert!", "EventMap has been renamed. Save Anyway?", "Yes", "No")) {
                if (File.Exists(_CurrentEventMapFile) && EditorUtility.DisplayDialog("Hold up!", "Should we delete this old file?\n" + _CurrentEventMapFile, "Yes", "No")) {
                  File.Delete(_CurrentEventMapFile);
                }
                _CurrentEventMapFile = newFileName;
              }
              else {
                good = false;
              }
            }

            if (good) {
              _RecentFiles.Add(_CurrentEventMapFile);

              File.WriteAllText(_CurrentEventMapFile, JsonConvert.SerializeObject(_CurrentEventMap, Formatting.Indented, GlobalSerializerSettings.JsonSettings));

              // Reload groups
              LoadData();
              EditorUtility.DisplayDialog("Save Successful!", "EventMap '" + _CurrentEventMapName + "' has been saved to " + _CurrentEventMapFile, "OK");
            }
          }
        }
      }
    }

    GUILayout.FlexibleSpace();

    EditorGUILayout.EndHorizontal();
  }

  // TODO : UPDATE
  private void DrawEventMap(AnimationEventMap eMap) {
    EditorGUILayout.BeginVertical();
    _scrollPos = EditorGUILayout.BeginScrollView(_scrollPos);

    EditorGUILayout.BeginHorizontal();
    GUILayout.Label("CLAD Events");
    EditorGUILayout.EndHorizontal();

    for (int i = 0; i < _CurrentEventMap.Pairs.Count; i++) {
      EditorGUILayout.BeginHorizontal();
      _CurrentEventMap.Pairs[i] = DrawCladEventEntry(_CurrentEventMap.Pairs[i]);
      EditorGUILayout.EndHorizontal();
    }

    EditorGUILayout.EndScrollView();
    EditorGUILayout.EndVertical();
  }

  // TODO : UPDATE
  public AnimationEventMap.CladAnimPair DrawCladEventEntry(AnimationEventMap.CladAnimPair pair) {
    EditorGUILayout.BeginVertical();
    EditorGUILayout.LabelField(pair.CladEvent.ToString());
    pair.AnimName = _AnimationGroupNameOptions[EditorGUILayout.Popup("Animation Group", Mathf.Max(0, Array.IndexOf(_AnimationGroupNameOptions, pair.AnimName)), _AnimationGroupNameOptions)];
    EditorGUILayout.EndVertical();
    return pair;
  }

  [MenuItem("Cozmo/Animation Event Map #%g")]
  public static void OpenAnimationGroupEventEditor() {
    AnimationGroupEventEditor window = (AnimationGroupEventEditor)EditorWindow.GetWindow(typeof(AnimationGroupEventEditor));
    window.titleContent = new GUIContent("Animation Event Map");
    window.Show();
    window.Focus();
    window.position = new Rect(0, 0, 500, 500);
  }


}
