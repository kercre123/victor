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

  private static bool _IsOpen = false;

  private static string[] _GoalGenNameOptions;
  private static string[] _FilteredCladNameOptions;
  private static GameEvent[] _FilteredCladList;

  private static string[] _DailyGoalGenFiles;

  private static DailyGoalGenerationData _CurrentGenData;
  private static string _CurrentGoalGenFile;
  private static string _CurrentGoalGenName;
  private static string _EventSearchField = "";
  private static Vector2 _scrollPos = new Vector2();

  // toggle state for foldouts of individual goals
  // uint corresponds to a unique ID
  // bool corresponds to whether or not the foldout is expanded
  private Dictionary<uint, bool> _DailyGoalFoldouts = new Dictionary<uint, bool>();
  // uint refers to the ID of the Daily Goal (that ID must be set to true in DailyGoalFoldouts to reveal conditions)
  // If true in DailyGoalFoldouts, then refer to the CondFoldouts to determine which Condition list
  private Dictionary<uint, CondFoldouts> _ConditionFoldouts = new Dictionary<uint, CondFoldouts>();

  private static bool _IdMapComplete = false;

  public class CondFoldouts {
    // If the Gen Condition list is folded out
    public bool FoldoutGenConditions;
    // If the Prog Condition list is folded out
    public bool FoldoutProgConditions;

    public CondFoldouts(bool isGenCon, bool isProCon) {
      FoldoutGenConditions = isGenCon;
      FoldoutProgConditions = isProCon;
    }
  }

  // get a uint 1 higher than the highest node's Id
  private uint GetNextGoalId() {
    var data = _CurrentGenData;

    if (data == null) {
      return 0;
    }

    uint maxId = 0;
    for (int i = 0; i < data.GenList.Count; i++) {
      var goal = data.GenList[i];

      maxId = goal.Id > maxId ? goal.Id : maxId;
    }
    return maxId + 1;
  }

  public static string sDailyGoalDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/DailyGoals"; } }

  public static string sCSVDirectory { get { return Environment.GetFolderPath(Environment.SpecialFolder.Desktop); } }

  public static string[] GoalGenNameOptions { get { return _GoalGenNameOptions; } }

  const string kDailyGoalFileCSV = "GoalReference.csv";

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
        RemapEditorIDs();
      }
      catch (Exception ex) {
        Debug.LogError(ex.Message);
      }
    }
  }

  public void RemapEditorIDs() {
    DailyGoalGenerationData.GoalEntry existingEntry;
    _ConditionFoldouts.Clear();
    _DailyGoalFoldouts.Clear();
    for (int i = 0; i < _CurrentGenData.GenList.Count; i++) {
      existingEntry = _CurrentGenData.GenList[i];
      // Only grab a new id if we have intersections or
      // there is no ID established
      if (existingEntry.Id == 0) {
        existingEntry.Id = GetNextGoalId();
      }
      _DailyGoalFoldouts.Add(existingEntry.Id, false);
      _ConditionFoldouts.Add(existingEntry.Id, new CondFoldouts(false, false));
    }
    DailyGoalEditor._IdMapComplete = true;
  }

  public void OnGUI() {
    DrawToolbar();

    if (_CurrentGenData != null) {
      if (DailyGoalEditor._IdMapComplete == false) {
        RemapEditorIDs();
      }
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
      if (eName.Contains(_EventSearchField) && gEvent != GameEvent.Count) {
        filterNameList.Add(eName);
        filterEventList.Add(gEvent);
      }
    }
    filterNameList.Add(GameEvent.Count.ToString());
    filterEventList.Add(GameEvent.Count);
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
              if (EditorUtility.DisplayDialog("Alert!", "Would you like to output a .csv to Desktop?", "Yes", "No")) {
                GenerateCSV();
              }
              // Reload groups
              LoadData();
              EditorUtility.DisplayDialog("Save Successful!", "EventMap '" + _CurrentGoalGenName + "' has been saved to " + _CurrentGoalGenFile, "OK");
              _IdMapComplete = false;
            }
          }
        }
      }
    }

    GUILayout.FlexibleSpace();

    EditorGUILayout.EndHorizontal();

  }

  // Outputs a CSV file for quick reference in spreadsheets
  private void GenerateCSV() {
    string toCSV = "Title,Reward\n";
    for (int i = 0; i < _CurrentGenData.GenList.Count; i++) {
      DailyGoalGenerationData.GoalEntry goal = _CurrentGenData.GenList[i];
      toCSV += string.Format("{0},{1}\n", LocalizationEditorUtility.GetTranslationSansFormatting(goal.TitleKey), goal.PointsRewarded);
    }
    string targetCSV = Path.Combine(sCSVDirectory, kDailyGoalFileCSV);
    if (File.Exists(targetCSV)) {
      File.Delete(targetCSV);
    }
    File.WriteAllText(targetCSV, toCSV);
  }

  private void DrawDailyGoalGenerationData(DailyGoalGenerationData genData) {

    EditorGUILayout.BeginVertical();
    _scrollPos = EditorGUILayout.BeginScrollView(_scrollPos);
    EditorGUIUtility.labelWidth = 100;
    EditorDrawingUtility.DrawFilteredGroupedList("DailyGoals", genData.GenList, DrawGoalDataEntry, AddDailyGoalEntry, x => x.CladEvent,
      _EventSearchField, GameEvent.Count, e => e.ToString());
    EditorGUILayout.EndScrollView();
    EditorGUILayout.EndVertical();

  }

  private DailyGoalGenerationData.GoalEntry AddDailyGoalEntry() {
    DailyGoalGenerationData.GoalEntry newGoal = new DailyGoalGenerationData.GoalEntry();
    newGoal.Id = GetNextGoalId();
    // If the GetNextGoalId returns a key that's still in the Dictionaries, it is because
    // we have removed the genData but not cleansed the Dictionary of that ID, free up
    // the ID.
    if (_DailyGoalFoldouts.ContainsKey(newGoal.Id)) {
      _DailyGoalFoldouts.Remove(newGoal.Id);
      _ConditionFoldouts.Remove(newGoal.Id);
    }
    _DailyGoalFoldouts.Add(newGoal.Id, false);
    _ConditionFoldouts.Add(newGoal.Id, new CondFoldouts(false, false));
    return newGoal;
  }

  private void CopyDailyGoalEntry(DailyGoalGenerationData.GoalEntry genData) {
    DailyGoalGenerationData.GoalEntry copy = genData.Copy();
    copy.Id = GetNextGoalId();
    // If the GetNextGoalId returns a key that's still in the Dictionaries, it is because
    // we have removed the genData but not cleansed the Dictionary of that ID, free up
    // the ID.
    if (_DailyGoalFoldouts.ContainsKey(copy.Id)) {
      _DailyGoalFoldouts.Remove(copy.Id);
      _ConditionFoldouts.Remove(copy.Id);
    }
    _DailyGoalFoldouts.Add(copy.Id, false);
    _ConditionFoldouts.Add(copy.Id, new CondFoldouts(false, false));
    _CurrentGenData.GenList.Add(copy);
  }

  public DailyGoalGenerationData.GoalEntry DrawGoalDataEntry(DailyGoalGenerationData.GoalEntry genData) {
    string goalName = LocalizationEditorUtility.GetTranslationSansFormatting(genData.TitleKey);
    bool isExpanded = false;
    EditorGUILayout.BeginVertical();
    if (!_DailyGoalFoldouts.TryGetValue(genData.Id, out isExpanded)) {
      return genData;
    }

    if (isExpanded) {
      GUI.contentColor = Color.green;
    }
    bool goalOpen = EditorGUI.Foldout(EditorGUILayout.GetControlRect(), isExpanded, goalName, EditorDrawingUtility.FoldoutStyle);
    _DailyGoalFoldouts[genData.Id] = goalOpen;
    GUI.contentColor = Color.white;
    // If the Goal is not expanded, Only show the title
    if (!goalOpen) {
      EditorGUILayout.EndVertical();
      return genData;
    }
    else {
      GUI.backgroundColor = new Color(0.75f, 0.75f, 1.0f);
    }

    if (GUILayout.Button(string.Format("Delete {0}", goalName), EditorDrawingUtility.RemoveButtonStyle)) {
      if (EditorUtility.DisplayDialog("Hold up!", "Should we delete this Goal?\n" + goalName, "Yes", "No")) {
        _CurrentGenData.GenList.Remove(genData);
      }
    }

    if (GUILayout.Button(string.Format("Copy {0}", goalName), EditorDrawingUtility.AddButtonStyle)) {
      CopyDailyGoalEntry(genData);
    }

    DrawGoalFields(genData);

    DrawConditionFoldouts(genData);

    DrawRewardFields(genData);

    EditorGUILayout.LabelField("Localization", EditorDrawingUtility.SubtitleStyle);
    EditorDrawingUtility.DrawLocalizationString(ref genData.TitleKey);

    EditorGUILayout.EndVertical();
    GUI.backgroundColor = Color.white;
    return genData;
  }

  public void DrawConditionFoldouts(DailyGoalGenerationData.GoalEntry genData) {

    CondFoldouts condExpanded = new CondFoldouts(false, false);

    if (!_ConditionFoldouts.TryGetValue(genData.Id, out condExpanded)) {
      _ConditionFoldouts.Add(genData.Id, new CondFoldouts(false, false));
    }
    //Draw lists of conditions here
    bool genCondOpen = condExpanded.FoldoutGenConditions;
    bool proCondOpen = condExpanded.FoldoutProgConditions;

    if (genCondOpen) {
      GUI.contentColor = Color.green;
    }
    genCondOpen = EditorGUI.Foldout(EditorGUILayout.GetControlRect(), genCondOpen,
      new GUIContent(string.Format("Generation Conditions{0}", (genData.GenConditions.Count > 0 ? EditorDrawingUtility.GetConditionListString(genData.GenConditions) : "")),
        "Conditions that must be met for the Goal to be selected for Generation"), EditorDrawingUtility.FoldoutStyle);
    GUI.contentColor = Color.white;
    if (genCondOpen) {
      EditorDrawingUtility.DrawConditionList(genData.GenConditions);
    }

    if (proCondOpen) {
      GUI.contentColor = Color.green;
    }
    proCondOpen = EditorGUI.Foldout(EditorGUILayout.GetControlRect(), proCondOpen,
      new GUIContent(string.Format("Progress Conditions{0}", (genData.ProgressConditions.Count > 0 ? EditorDrawingUtility.GetConditionListString(genData.ProgressConditions) : "")),
        "Conditions that must be met for the Goal to be progressed when its GameEvent fires"), EditorDrawingUtility.FoldoutStyle);
    GUI.contentColor = Color.white;
    if (proCondOpen) {
      EditorDrawingUtility.DrawConditionList(genData.ProgressConditions);
    }

    _ConditionFoldouts[genData.Id] = new CondFoldouts(genCondOpen, proCondOpen);
  }

  public void DrawGoalFields(DailyGoalGenerationData.GoalEntry genData) {

    EditorGUILayout.LabelField("Goal", EditorDrawingUtility.SubtitleStyle);
    if (string.IsNullOrEmpty(_EventSearchField)) {
      genData.CladEvent = (GameEvent)EditorGUILayout.EnumPopup("GameEvent", genData.CladEvent);
    }
    else {
      genData.CladEvent = _FilteredCladList[EditorGUILayout.Popup("GameEvent", Mathf.Max(0, Array.IndexOf(_FilteredCladNameOptions, genData.CladEvent.ToString())), _FilteredCladNameOptions)];
    }

    EditorDrawingUtility.DrawTagDropDown(ref genData.Tag);
    genData.Target = EditorGUILayout.IntField(new GUIContent("Target", "Amount of times the goal event needs to fire with all conditions met to complete goal"), genData.Target);
    genData.Priority = EditorGUILayout.IntField(new GUIContent("Priority", "Higher priority goals will appear higher in the Daily Goal panel"), genData.Priority);

  }

  public void DrawRewardFields(DailyGoalGenerationData.GoalEntry genData) {
    EditorGUILayout.LabelField("Reward", EditorDrawingUtility.SubtitleStyle);
    genData.RewardType = EditorGUILayout.TextField("Reward", genData.RewardType);
    genData.PointsRewarded = EditorGUILayout.IntField("Count", genData.PointsRewarded);
  }

  [MenuItem("Cozmo/Progression/Daily Goal Editor #%d")]
  public static void OpenDailyGoalEditor() {
    DailyGoalEditor window = (DailyGoalEditor)EditorWindow.GetWindow(typeof(DailyGoalEditor));
    if (DailyGoalEditor._IsOpen) {
      window.Close();
      DailyGoalEditor._IsOpen = false;
    }
    else {
      DailyGoalEditor.LoadData();
      window.titleContent = new GUIContent("DailyGoal Editor");
      window.Show();
      window.Focus();
      window.position = new Rect(0, 0, 600, 600);
      DailyGoalEditor._IsOpen = true;
      DailyGoalEditor._IdMapComplete = false;
    }
  }


}
