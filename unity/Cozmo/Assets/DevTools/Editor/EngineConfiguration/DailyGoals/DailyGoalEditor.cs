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
using Cozmo.UI;


/// <summary>
/// Daily Goal Editor. Used for creating and editing Daily Goals.
/// </summary>
public class DailyGoalEditor : EditorWindow {


  private static string[] _AnimationGroupNameOptions;
  private static string[] _FilteredAnimationOptions;

  private static string[] _DailyGoalGenFiles;

  private static List<DailyGoal> _DailyGoalList;
  private static string _CurrentEventMapFile;
  private static string _CurrentEventMapName;
  private static string _EventSearchField = "";
  private static string _AnimSearchField = "";

  private static Vector2 _scrollPos = new Vector2();

  public static string sDailyGoalDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/DailyGoals"; } }

  public static string[] AnimationGroupNameOptions { get { return _AnimationGroupNameOptions; } }

  static DailyGoalEditor() {
    LoadData();
  }

  private static void LoadData() {
    // Load all Event Map Configs (Can have multiple, so you can create different configs, game only uses one.)
    if (Directory.Exists(sDailyGoalDirectory)) {
      _DailyGoalGenFiles = Directory.GetFiles(sDailyGoalDirectory);
    }
    else {
      Directory.CreateDirectory(sDailyGoalDirectory);
      _DailyGoalGenFiles = new string[0];
    }

  }

  private bool CheckDiscardUnsaved() {
    bool canOpen = true;
    /*
    if (_CurrentEventMap != null && (string.IsNullOrEmpty(_CurrentEventMapFile) || JsonConvert.SerializeObject(_CurrentEventMap, Formatting.Indented, GlobalSerializerSettings.JsonSettings) != File.ReadAllText(_CurrentEventMapFile))) {
      canOpen = EditorUtility.DisplayDialog("Warning", "Opening a different EventMap will Discard Unsaved Changes. Are you Sure?", "Yes", "No");
    }
    */
    return canOpen;
  }

  // Deserialize the list, potentially create an object like AnimationEventMap that's just a serializable holder for DailyGoals
  private void LoadFile(string path) {
    /*
    if (CheckDiscardUnsaved()) {
      try {
        _CurrentEventMap = null;
        _CurrentEventMapFile = null;

        string json = File.ReadAllText(path);
        _CurrentEventMap = JsonConvert.DeserializeObject<AnimationEventMap>(json, GlobalSerializerSettings.JsonSettings);
        _CurrentEventMapName = Path.GetFileNameWithoutExtension(path);
        _CurrentEventMapFile = path;
        // Update the map in case there are new events since we last opened it
        string newStuff = "";
        if (_CurrentEventMap.MapUpdate(out newStuff)) {
          EditorUtility.DisplayDialog("Heads up", string.Format("Just so you know, there are new game events added since you last opened this file, including {0}", newStuff), "Thanks!");
        }
      }
      catch (Exception ex) {
        DAS.Error(this, ex.Message);
      }
    }*/
  }

  public void OnGUI() {
    /*
    DrawToolbar();

    if (_CurrentEventMap != null) {

      _CurrentEventMapName = EditorGUILayout.TextField("File Name", _CurrentEventMapName ?? string.Empty);
      _EventSearchField = EditorGUILayout.TextField("Filter CLAD Events", _EventSearchField ?? string.Empty);
      _AnimSearchField = EditorGUILayout.TextField("Filter Animation Groups", _AnimSearchField ?? string.Empty);
      FilterAnimFields();
      DrawEventMap(_CurrentEventMap);
    }
    */
  }

  // Filter Event Names
  private void FilterAnimFields() {
    List<string> filterList = new List<string>();
    for (int i = 0; i < _AnimationGroupNameOptions.Length; i++) {
      string aName = _AnimationGroupNameOptions[i];
      if (aName.Contains(_AnimSearchField)) {
        filterList.Add(aName);
      }
    }
    filterList.Insert(0, "");
    _FilteredAnimationOptions = filterList.ToArray();
  }

  private void DrawToolbar() {
    /*
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

    // New Event Map Button
    if (GUILayout.Button("New EventToAnimationMap", EditorDrawingUtility.ToolbarButtonStyle)) {
      if (CheckDiscardUnsaved()) {
        _CurrentEventMap = new AnimationEventMap();
        _CurrentEventMap.NewMap();
        _CurrentEventMapName = string.Empty;
        _CurrentEventMapFile = null;
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
          if (string.IsNullOrEmpty(_CurrentEventMapName)) {
            EditorUtility.DisplayDialog("Error!", "Name your file before you save it!", "Ok");
          }
          else if (_CurrentEventMap.Pairs.Count == 0) {
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
    */
  }

  // TODO: Draw DailyGoalGeneration.json -> New class that's holder for all DailyGoalGen classes?
  private void DrawEventMap(AnimationEventMap eMap) {
    /*
    EditorGUILayout.BeginVertical();
    _scrollPos = EditorGUILayout.BeginScrollView(_scrollPos);

    EditorGUILayout.BeginHorizontal();
    GUILayout.Label("CLAD Events");
    EditorGUILayout.EndHorizontal();

    for (int i = 0; i < _CurrentEventMap.Pairs.Count; i++) {
      _CurrentEventMap.Pairs[i] = DrawCladEventEntry(_CurrentEventMap.Pairs[i]);
    }

    EditorGUILayout.EndScrollView();
    EditorGUILayout.EndVertical();
    */
  }

  // TODO:
  public AnimationEventMap.CladAnimPair DrawCladEventEntry(AnimationEventMap.CladAnimPair pair) {
    /*
    EditorGUILayout.BeginHorizontal();
    string eventName = pair.CladEvent.ToString();
    if (string.IsNullOrEmpty(_EventSearchField) || eventName.Contains(_EventSearchField)) {
      if (string.IsNullOrEmpty(_AnimSearchField) || pair.AnimName == "" || pair.AnimName.Contains(_AnimSearchField)) {
        pair.AnimName = _FilteredAnimationOptions[EditorGUILayout.Popup(eventName, Mathf.Max(0, Array.IndexOf(_FilteredAnimationOptions, pair.AnimName)), _FilteredAnimationOptions)];
      }
      else if (!string.IsNullOrEmpty(_AnimSearchField) && !pair.AnimName.Contains(_AnimSearchField)) {
        pair.AnimName = _AnimationGroupNameOptions[EditorGUILayout.Popup(eventName, Mathf.Max(0, Array.IndexOf(_AnimationGroupNameOptions, pair.AnimName)), _AnimationGroupNameOptions)];
      }
      if (GUILayout.Button("X", GUILayout.Width(30))) {
        pair.AnimName = "";
      }
    }
    EditorGUILayout.EndHorizontal();
    */
    return pair;
  }

  [MenuItem("Cozmo/Progression/Daily Goal Editor #%d")]
  public static void OpenAnimationGroupEventEditor() {
    DailyGoalEditor window = (DailyGoalEditor)EditorWindow.GetWindow(typeof(DailyGoalEditor));
    DailyGoalEditor.LoadData();
    window.titleContent = new GUIContent("DailyGoal Editor");
    window.Show();
    window.Focus();
    window.position = new Rect(0, 0, 500, 500);
  }


}
