using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using AnimationGroups;
using System.IO;
using Newtonsoft.Json;
using Anki.Cozmo;
using Cozmo;
using Cozmo.UI;
using System.Reflection;


/// <summary>
/// Daily Goal Editor. Used for creating and editing Daily Goals.
/// </summary>
public class DailyGoalEditor : EditorWindow {


  private static string[] _GoalGenNameOptions;
  private static string[] _FilteredCladNameOptions;
  private static GameEvent[] _FilteredCladList;


  // Required to display the Condition Select Popup
  private static string[] _ConditionTypeNames;
  private static Type[] _ConditionTypes;
  private static int[] _ConditionIndices;
  private int _SelectedConditionIndex = 0;

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

    // get all conditions
    var ctypes = Assembly.GetAssembly(typeof(GoalCondition))
      .GetTypes()
      .Where(t => typeof(GoalCondition).IsAssignableFrom(t) &&
                 !t.IsAbstract);
    _ConditionTypes = ctypes.ToArray();
    _ConditionTypeNames = _ConditionTypes.Select(x => x.Name.ToHumanFriendly()).ToArray();

    _ConditionIndices = new int[_ConditionTypeNames.Length];
    for (int i = 0; i < _ConditionIndices.Length; i++) {
      _ConditionIndices[i] = i;
    }

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
        Debug.LogError(ex.Message);
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

  private void DrawDailyGoalGenerationData(DailyGoalGenerationData genData) {
    
    EditorGUILayout.BeginVertical();
    _scrollPos = EditorGUILayout.BeginScrollView(_scrollPos);
    EditorGUIUtility.labelWidth = 100;
    EditorDrawingUtility.DrawGroupedList("DailyGoals", genData.GenList, DrawGoalDataEntry, () => new DailyGoalGenerationData.GoalEntry(), x => x.CladEvent, e => e.ToString());
    EditorGUILayout.EndScrollView();
    EditorGUILayout.EndVertical();

  }

  public DailyGoalGenerationData.GoalEntry DrawGoalDataEntry(DailyGoalGenerationData.GoalEntry genData) {
    string eventName = genData.CladEvent.ToString();
    if (string.IsNullOrEmpty(_EventSearchField) || eventName.Contains(_EventSearchField) || genData.CladEvent == GameEvent.Count) {
      EditorGUILayout.BeginVertical();
      string titleLocfile = "";
      EditorGUILayout.LabelField(string.Format(">{0}", LocalizationEditorUtility.GetTranslation(genData.TitleKey, out titleLocfile).ToUpper()));
      EditorGUI.indentLevel++;
      EditorGUILayout.LabelField(">>GOAL");
      EditorGUI.indentLevel++;
      EditorGUILayout.BeginHorizontal();
      if (string.IsNullOrEmpty(_EventSearchField) || genData.CladEvent == GameEvent.Count) {
        genData.CladEvent = (GameEvent)EditorGUILayout.EnumPopup("GameEvent", genData.CladEvent, GUILayout.Width(250));
      }
      else {
        genData.CladEvent = _FilteredCladList[EditorGUILayout.Popup("GameEvent", Mathf.Max(0, Array.IndexOf(_FilteredCladNameOptions, eventName)), _FilteredCladNameOptions, GUILayout.Width(250))];
      }
      genData.Target = EditorGUILayout.IntField("Target", genData.Target);
      EditorGUILayout.EndHorizontal();
      EditorGUI.indentLevel--;

      //Draw list of conditions here
      DrawConditionList(new GUIContent(">> GENERATION CONDITIONS", 
        "Conditions that must be met for the Goal to be selected for Generation"), genData.GenConditions);
      
      //Draw list of conditions here
      DrawConditionList(new GUIContent(">> PROGRESS CONDITIONS", 
        "Conditions that must be met for the Goal to be progressed MidGame"), genData.ProgressConditions);
      
      EditorGUILayout.LabelField(">>REWARD");
      EditorGUI.indentLevel++;
      EditorGUILayout.BeginHorizontal();
      genData.RewardType = EditorGUILayout.TextField("Type", genData.RewardType ?? string.Empty);
      genData.PointsRewarded = EditorGUILayout.IntField("Count", genData.PointsRewarded);
      EditorGUILayout.EndHorizontal();
      EditorGUI.indentLevel--;

      EditorGUILayout.LabelField(">>LOC KEYS");
      EditorGUI.indentLevel++;
      EditorGUILayout.BeginHorizontal();
      EditorDrawingUtility.DrawLocalizationString(ref genData.TitleKey, ref titleLocfile);
      EditorDrawingUtility.DrawLocalizationString(ref genData.DescKey);
      EditorGUILayout.EndHorizontal();
      EditorGUI.indentLevel--;
      EditorGUI.indentLevel--;
      EditorGUILayout.EndVertical();
    }
    return genData;
  }

  [MenuItem("Cozmo/Progression/Daily Goal Editor #%d")]
  public static void OpenDailyGoalEditor() {
    DailyGoalEditor window = (DailyGoalEditor)EditorWindow.GetWindow(typeof(DailyGoalEditor));
    DailyGoalEditor.LoadData();
    window.titleContent = new GUIContent("DailyGoal Editor");
    window.Show();
    window.Focus();
    window.position = new Rect(0, 0, 800, 600);
  }

  #region GoalConditions and Helpers

  // some things ignore indent. This is a work around
  public Rect GetIndentedLabelRect() {
    var rect = EditorGUILayout.GetControlRect();
    rect.x += 15 * (EditorGUI.indentLevel);
    rect.width -= 15 * (EditorGUI.indentLevel);
    return rect;
  }


  // Draws a list of Conditions
  public void DrawConditionList<T>(GUIContent label, List<T> conditions) where T : GoalCondition {    
    GUI.Label(GetIndentedLabelRect(), label);
    for (int i = 0; i < conditions.Count; i++) {
      var cond = conditions[i];
      // clear out any null conditions
      if (cond == null) {
        conditions.RemoveAt(i);
        i--;
        continue;
      }

      EditorGUILayout.BeginHorizontal();

      (cond as T).OnGUI_DrawUniqueControls();
      if (GUI.Button(EditorGUILayout.GetControlRect(), "C--")) {
        conditions.RemoveAt(i);
      }

      EditorGUILayout.EndHorizontal();
    }

    var nextRect = EditorGUILayout.GetControlRect();

    // show the add condition/action box at the bottom of the list
    var newObject = DrawAddConditionPopup<T>(nextRect, ref _SelectedConditionIndex, _ConditionTypeNames, _ConditionIndices, _ConditionTypes);

    if (!EqualityComparer<T>.Default.Equals(newObject, default(T))) {
      conditions.Add(newObject);
    }

  }

  // internal function for ShowAddPopup, does the layout of the buttons and actual object creation
  private T DrawAddConditionPopup<T>(Rect rect, ref int index, string[] names, int[] indices, Type[] types) where T : GoalCondition {
    var popupRect = new Rect(rect.x, rect.y, rect.width - 50, rect.height);
    var plusRect = new Rect(rect.x + rect.width - 50, rect.y, 50, rect.height);
    index = EditorGUI.IntPopup(popupRect, index, names, indices);

    if (GUI.Button(plusRect, "C++")) {
      var result = (T)(Activator.CreateInstance(types[index]));
      return result;
    }
    return default(T);
  }


  #endregion


}
