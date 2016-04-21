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


  private static string[] _GoalGenNameOptions;
  private static string[] _FilteredCladNameOptions;
  private static GameEvent[] _FilteredCladList;


  private static string[] _DailyGoalGenFiles;

  private static DailyGoalGenerationData _CurrentGenData;
  private static string _CurrentGoalGenFile;
  private static string _CurrentGoalGenName;
  private static string _EventSearchField = "";

  private static Vector2 _scrollPos = new Vector2();

  public static string sDailyGoalDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/DailyGoals"; } }

  public static string[] GoalGenNameOptions { get { return _GoalGenNameOptions; } }

  static DailyGoalEditor() {
    LoadData();
  }

  private static void LoadData() {
    // Load all Event Map Configs (Can have multiple, so you can create different configs, game only uses one.)
    if (Directory.Exists(sDailyGoalDirectory)) {
      _DailyGoalGenFiles = Directory.GetFiles(sDailyGoalDirectory);
      _GoalGenNameOptions = _DailyGoalGenFiles.Select(x => Path.GetFileNameWithoutExtension(x)).ToArray();
    }
    else {
      Directory.CreateDirectory(sDailyGoalDirectory);
      _DailyGoalGenFiles = new string[0];
      _GoalGenNameOptions = _DailyGoalGenFiles;
    }

  }

  private bool CheckDiscardUnsaved() {
    bool canOpen = true;

    if (_CurrentGenData != null && (string.IsNullOrEmpty(_CurrentGoalGenFile) || JsonConvert.SerializeObject(_CurrentGenData, Formatting.Indented, GlobalSerializerSettings.JsonSettings) != File.ReadAllText(_CurrentGoalGenFile))) {
      canOpen = EditorUtility.DisplayDialog("Warning", "Opening a different file will Discard Unsaved Changes. Are you Sure?", "Yes", "No");
    }

    return canOpen;
  }

  // Deserialize the list, potentially create an object like AnimationEventMap that's just a serializable holder for DailyGoals
  private void LoadFile(string path) {
    if (CheckDiscardUnsaved()) {
      try {
        _CurrentGenData = null;
        _CurrentGoalGenFile = null;

        string json = File.ReadAllText(path);
        _CurrentGenData = JsonConvert.DeserializeObject<DailyGoalGenerationData>(json, GlobalSerializerSettings.JsonSettings);
        _CurrentGoalGenName = Path.GetFileNameWithoutExtension(path);
        _CurrentGoalGenFile = path;
      }
      catch (Exception ex) {
        DAS.Error(this, ex.Message);
      }
    }
  }

  public void OnGUI() {
    
    DrawToolbar();

    if (_CurrentGenData != null) {
      _CurrentGoalGenName = EditorGUILayout.TextField("File Name", _CurrentGoalGenName ?? string.Empty);
      string eventFilter = EditorGUILayout.TextField("Filter CLAD Events", _EventSearchField ?? string.Empty);
      // Only filter if the event filter has been changed
      if (eventFilter != _EventSearchField) {
        _EventSearchField = eventFilter;
        FilterGoalNames();
      }
      DrawDailyGoalGenerationData(_CurrentGenData);
    }
  }

  // Filter Event Names
  private void FilterGoalNames() {
    List<string> filterNameList = new List<string>();
    List<GameEvent> filterEventList = new List<GameEvent>();
    for (int i = 0; i < (int)GameEvent.Count; i++) {
      GameEvent gEvent = (GameEvent)i;
      string eName = gEvent.ToString();
      if (eName.Contains(_EventSearchField)) {
        filterNameList.Add(eName);
        filterEventList.Add(gEvent);
      }
    }
    _FilteredCladNameOptions = filterNameList.ToArray();
    _FilteredCladList = filterEventList.ToArray();
  }

  private void DrawToolbar() {
    
    EditorGUILayout.BeginHorizontal(EditorDrawingUtility.ToolbarStyle);

    if (GUILayout.Button("Load", EditorDrawingUtility.ToolbarButtonStyle)) {
      GenericMenu menu = new GenericMenu();

      foreach (var file in _DailyGoalGenFiles.Where(x => x.EndsWith(".json"))) {
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
    if (GUILayout.Button("New DailyGoalGeneration File", EditorDrawingUtility.ToolbarButtonStyle)) {
      if (CheckDiscardUnsaved()) {
        _CurrentGenData = new DailyGoalGenerationData();
        _CurrentGoalGenName = string.Empty;
        _CurrentGoalGenFile = null;
        GUI.FocusControl("EditNameField");
      }
    }
    // Save Current Event Map Button
    if (_CurrentGenData != null) {
      if (GUILayout.Button("Save", EditorDrawingUtility.ToolbarButtonStyle)) {         
        if (string.IsNullOrEmpty(_CurrentGoalGenFile)) {
          _CurrentGoalGenFile = EditorUtility.SaveFilePanel("Save EventMap", sDailyGoalDirectory, _CurrentGoalGenName, "json");
        }

        if (!string.IsNullOrEmpty(_CurrentGoalGenFile)) {
          if (string.IsNullOrEmpty(_CurrentGoalGenName)) {
            EditorUtility.DisplayDialog("Error!", "Name your file before you save it!", "Ok");
          }
          else if (_CurrentGenData.GenList.Count == 0) {
            EditorUtility.DisplayDialog("Error!", "Cannot save with no Daily Goals!", "Ok");
          }
          else {
            string newFileName = Path.Combine(sDailyGoalDirectory, _CurrentGoalGenName + ".json");

            bool good = true;
            if (Path.GetFileName(_CurrentGoalGenFile) != Path.GetFileName(newFileName)) {
              if (EditorUtility.DisplayDialog("Alert!", "Daily Goal File has been renamed. Save Anyway?", "Yes", "No")) {
                if (File.Exists(_CurrentGoalGenFile) && EditorUtility.DisplayDialog("Hold up!", "Should we delete this old file?\n" + _CurrentGoalGenFile, "Yes", "No")) {
                  File.Delete(_CurrentGoalGenFile);
                }
                _CurrentGoalGenFile = newFileName;
              }
              else {
                good = false;
              }
            }

            if (good) {

              File.WriteAllText(_CurrentGoalGenFile, JsonConvert.SerializeObject(_CurrentGenData, Formatting.Indented, GlobalSerializerSettings.JsonSettings));

              // Reload groups
              LoadData();
              EditorUtility.DisplayDialog("Save Successful!", "EventMap '" + _CurrentGoalGenName + "' has been saved to " + _CurrentGoalGenFile, "OK");
            }
          }
        }
      }
    }

    GUILayout.FlexibleSpace();

    EditorGUILayout.EndHorizontal();

  }

  // TODO: Draw DailyGoalGeneration.json -> New class that's holder for all DailyGoalGen classes?
  private void DrawDailyGoalGenerationData(DailyGoalGenerationData genData) {
    
    EditorGUILayout.BeginVertical();
    _scrollPos = EditorGUILayout.BeginScrollView(_scrollPos);
    EditorDrawingUtility.DrawGroupedList("DailyGoals", genData.GenList, DrawGoalDataEntry, () => new DailyGoalGenerationData.GoalEntry(), x => x.CladEvent, e => e.ToString());
    EditorGUILayout.EndScrollView();
    EditorGUILayout.EndVertical();

  }

  // TODO: Draw DailyGoalGenEntry
  public DailyGoalGenerationData.GoalEntry DrawGoalDataEntry(DailyGoalGenerationData.GoalEntry genData) {

    EditorGUILayout.BeginVertical();
    string eventName = genData.CladEvent.ToString();
    if (string.IsNullOrEmpty(_EventSearchField) || eventName.Contains(_EventSearchField) || genData.CladEvent == GameEvent.Count) {
      EditorGUILayout.LabelField(Localization.Get(genData.TitleKey));
      EditorGUILayout.BeginHorizontal();
      genData.TitleKey = EditorGUILayout.TextField("TitleLocKey", genData.TitleKey ?? string.Empty);
      genData.DescKey = EditorGUILayout.TextField("DescriptionLocKey", genData.DescKey ?? string.Empty);
      EditorGUILayout.EndHorizontal();
      if (string.IsNullOrEmpty(_EventSearchField) || genData.CladEvent == GameEvent.Count) {
        genData.CladEvent = (GameEvent)EditorGUILayout.EnumPopup("GameEvent", genData.CladEvent);
      }
      else {
        genData.CladEvent = _FilteredCladList[EditorGUILayout.Popup("GameEvent", Mathf.Max(0, Array.IndexOf(_FilteredCladNameOptions, eventName)), _FilteredCladNameOptions)];
      }
      EditorGUILayout.BeginHorizontal();
      genData.Target = EditorGUILayout.IntField("Target", genData.Target);
      genData.PointsRewarded = EditorGUILayout.IntField("Reward", genData.PointsRewarded);

      EditorGUILayout.EndHorizontal();
    }
    
    EditorGUILayout.EndVertical();

    return genData;
  }

  public GoalCondition DrawGoalCondition(GoalCondition cond) {
    EditorGUILayout.BeginVertical();
    // TODO: Figure out how to display Conditions
    // Look to ScriptedSequenceEditor to see how they get GoalConditions based on the actual classes
    EditorGUILayout.EndVertical();
    return cond;
  }

  [MenuItem("Cozmo/Progression/Daily Goal Editor #%d")]
  public static void OpenDailyGoalEditor() {
    DailyGoalEditor window = (DailyGoalEditor)EditorWindow.GetWindow(typeof(DailyGoalEditor));
    DailyGoalEditor.LoadData();
    window.titleContent = new GUIContent("DailyGoal Editor");
    window.Show();
    window.Focus();
    window.position = new Rect(0, 0, 500, 500);
  }


}
