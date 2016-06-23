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

  #region GUIStyle

  // Unity Style for the Main DAILY GOALS title at the top
  private static GUIStyle _MainTitleStyle;

  public static GUIStyle MainTitleStyle {
    get {
      if (_MainTitleStyle == null) {
        _MainTitleStyle = new GUIStyle();
        _MainTitleStyle.fontStyle = FontStyle.Bold;
        _MainTitleStyle.normal.textColor = Color.white;
        _MainTitleStyle.active.textColor = Color.white;
        _MainTitleStyle.fontSize = 28;
      }
      return _MainTitleStyle;
    }
  }

  // Unity Style for event labels that separate each group of daily goals
  private static GUIStyle _TitleStyle;

  public static GUIStyle TitleStyle {
    get {
      if (_TitleStyle == null) {
        _TitleStyle = new GUIStyle();
        _TitleStyle.fontStyle = FontStyle.Bold;
        _TitleStyle.normal.textColor = Color.white;
        _TitleStyle.active.textColor = Color.white;
        _TitleStyle.fontSize = 20;
      }
      return _TitleStyle;
    }
  }

  // Unity style for labels within each individual daily goal
  private static GUIStyle _SubtitleStyle;

  public static GUIStyle SubtitleStyle {
    get {
      if (_SubtitleStyle == null) {
        _SubtitleStyle = new GUIStyle();
        _SubtitleStyle.fontStyle = FontStyle.Bold;
        _SubtitleStyle.normal.textColor = Color.white;
        _SubtitleStyle.active.textColor = Color.white;
        _SubtitleStyle.fontSize = 16;
      }
      return _SubtitleStyle;
    }
  }

  // custom Unity Style for a button
  private static GUIStyle _AddButtonStyle;

  public static GUIStyle AddButtonStyle {
    get {
      if (_AddButtonStyle == null) {
        _AddButtonStyle = new GUIStyle(EditorStyles.miniButton);
        _AddButtonStyle.fontStyle = FontStyle.Bold;
        _AddButtonStyle.normal.textColor = Color.green;
        _AddButtonStyle.active.textColor = Color.white;
        _AddButtonStyle.fontSize = 14;
        _AddButtonStyle.stretchWidth = true;
      }
      return _AddButtonStyle;
    }
  }

  // custom Unity Style for a button
  private static GUIStyle _RemoveButtonStyle;

  public static GUIStyle RemoveButtonStyle {
    get {
      if (_RemoveButtonStyle == null) {
        _RemoveButtonStyle = new GUIStyle(EditorStyles.miniButton);
        _RemoveButtonStyle.fontStyle = FontStyle.Bold;
        _RemoveButtonStyle.normal.textColor = Color.red;
        _RemoveButtonStyle.active.textColor = Color.white;
        _RemoveButtonStyle.fontSize = 14;
        _RemoveButtonStyle.stretchWidth = true;
      }
      return _RemoveButtonStyle;
    }
  }

  // TODO: Add in a proper highlight background for when Foldouts are opened, Box Texture2D with some color
  private static GUIStyle _FoldoutStyle;

  public static GUIStyle FoldoutStyle {
    get {
      if (_FoldoutStyle == null) {
        _FoldoutStyle = new GUIStyle(EditorStyles.foldout);
        Color white = Color.white;
        _FoldoutStyle.fontStyle = FontStyle.Bold;
        _FoldoutStyle.normal.textColor = white;
        _FoldoutStyle.onNormal.textColor = white;
        _FoldoutStyle.hover.textColor = white;
        _FoldoutStyle.onHover.textColor = white;
        _FoldoutStyle.focused.textColor = white;
        _FoldoutStyle.onFocused.textColor = white;
        _FoldoutStyle.active.textColor = white;
        _FoldoutStyle.onActive.textColor = white;
      }
      return _FoldoutStyle;
    }
  }

  #endregion

  private static bool _IsOpen = false;

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
              _IdMapComplete = false;
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
    EditorDrawingUtility.DrawGroupedList("DailyGoals", genData.GenList, DrawGoalDataEntry, AddDailyGoalEntry, x => x.CladEvent, e => e.ToString(), DailyGoalEditor.MainTitleStyle, DailyGoalEditor.TitleStyle);
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

  public DailyGoalGenerationData.GoalEntry DrawGoalDataEntry(DailyGoalGenerationData.GoalEntry genData) {
    string eventName = genData.CladEvent.ToString();
    bool isExpanded = false;
    CondFoldouts condExpanded = new CondFoldouts(false, false);
    if (string.IsNullOrEmpty(_EventSearchField) || eventName.Contains(_EventSearchField) || genData.CladEvent == GameEvent.Count) {
      EditorGUILayout.BeginVertical();
      if (_DailyGoalFoldouts.TryGetValue(genData.Id, out isExpanded)) {
        if (!_ConditionFoldouts.TryGetValue(genData.Id, out condExpanded)) {
          _ConditionFoldouts.Add(genData.Id, new CondFoldouts(false, false));
        }
      }
      else {
        return genData;
      }
      EditorGUILayout.BeginHorizontal();
      if (isExpanded) {
        GUI.contentColor = Color.green;
      }
      bool goalOpen = EditorGUI.Foldout(EditorGUILayout.GetControlRect(), isExpanded, string.Format("{0}", genData.TitleKey), DailyGoalEditor.FoldoutStyle);
      _DailyGoalFoldouts[genData.Id] = goalOpen;
      GUI.contentColor = Color.white;
      // If the Goal is not expanded, Only show the title
      if (!goalOpen) {
        EditorGUILayout.EndHorizontal();
        EditorGUILayout.EndVertical();
        return genData;
      }
      else {
        GUI.backgroundColor = new Color(0.75f, 0.75f, 1.0f);
      }

      if (GUILayout.Button(string.Format("Delete {0}", genData.TitleKey), DailyGoalEditor.RemoveButtonStyle)) {
        _CurrentGenData.GenList.Remove(genData);
      }
      EditorGUILayout.EndHorizontal();
      EditorGUILayout.LabelField("Goal", DailyGoalEditor.SubtitleStyle);
      EditorGUILayout.BeginHorizontal();
      if (string.IsNullOrEmpty(_EventSearchField) || genData.CladEvent == GameEvent.Count) {
        genData.CladEvent = (GameEvent)EditorGUILayout.EnumPopup("GameEvent", genData.CladEvent);
      }
      else {
        genData.CladEvent = _FilteredCladList[EditorGUILayout.Popup("GameEvent", Mathf.Max(0, Array.IndexOf(_FilteredCladNameOptions, eventName)), _FilteredCladNameOptions)];
      }
      genData.Target = EditorGUILayout.IntField(new GUIContent("Target", "Amount of times the goal event needs to fire with all conditions met to complete goal"), genData.Target);
      genData.Priority = EditorGUILayout.IntField(new GUIContent("Priority", "Higher priority goals will appear higher in the Daily Goal panel"), genData.Priority);
      EditorGUILayout.EndHorizontal();

      //Draw lists of conditions here
      bool genCondOpen = condExpanded.FoldoutGenConditions;
      bool proCondOpen = condExpanded.FoldoutProgConditions;

      if (genCondOpen) {
        GUI.contentColor = Color.green;
      }
      genCondOpen = EditorGUI.Foldout(EditorGUILayout.GetControlRect(), genCondOpen,
        new GUIContent(string.Format("Generation Conditions{0}", (genData.GenConditions.Count > 0 ? string.Format(" - ({0})", genData.GenConditions.Count) : "")), 
          "Conditions that must be met for the Goal to be selected for Generation"), DailyGoalEditor.FoldoutStyle);
      GUI.contentColor = Color.white;
      if (genCondOpen) {
        DrawConditionList(genData.GenConditions);
      }

      if (proCondOpen) {
        GUI.contentColor = Color.green;
      }
      proCondOpen = EditorGUI.Foldout(EditorGUILayout.GetControlRect(), proCondOpen,
        new GUIContent(string.Format("Progress Conditions{0}", (genData.ProgressConditions.Count > 0 ? string.Format(" - ({0})", genData.ProgressConditions.Count) : "")), 
          "Conditions that must be met for the Goal to be progressed when its GameEvent fires"), DailyGoalEditor.FoldoutStyle);
      GUI.contentColor = Color.white;
      if (proCondOpen) {
        DrawConditionList(genData.ProgressConditions);
      }

      _ConditionFoldouts[genData.Id] = new CondFoldouts(genCondOpen, proCondOpen);
      
      EditorGUILayout.LabelField("Reward", DailyGoalEditor.SubtitleStyle);
      EditorGUILayout.BeginHorizontal();
      genData.RewardType = EditorGUILayout.TextField("Reward", genData.RewardType);
      genData.PointsRewarded = EditorGUILayout.IntField("Count", genData.PointsRewarded);
      EditorGUILayout.EndHorizontal();

      EditorGUILayout.LabelField("Localization", DailyGoalEditor.SubtitleStyle);
      EditorGUILayout.BeginHorizontal();
      EditorDrawingUtility.DrawLocalizationString(ref genData.TitleKey);
      EditorDrawingUtility.DrawLocalizationString(ref genData.DescKey);
      EditorGUILayout.EndHorizontal();
      EditorGUILayout.EndVertical();
      GUI.backgroundColor = Color.white;
    }
    return genData;
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

  #region GoalConditions and Helpers

  // some things ignore indent. This is a work around
  public Rect GetIndentedLabelRect() {
    var rect = EditorGUILayout.GetControlRect();
    rect.x += 15 * (EditorGUI.indentLevel);
    rect.width -= 15 * (EditorGUI.indentLevel);
    return rect;
  }


  // Draws a list of Conditions
  public void DrawConditionList<T>(List<T> conditions) where T : GoalCondition {    
    for (int i = 0; i < conditions.Count; i++) {
      var cond = conditions[i];
      // clear out any null conditions
      if (cond == null) {
        conditions.RemoveAt(i);
        i--;
        continue;
      }

      EditorGUILayout.BeginHorizontal();

      var name = (cond as T).GetType().Name.ToHumanFriendly();
      (cond as T).OnGUI_DrawUniqueControls();
      if (GUI.Button(EditorGUILayout.GetControlRect(), String.Format("Delete {0}", name), DailyGoalEditor.RemoveButtonStyle)) {
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
    var popupRect = new Rect(rect.x, rect.y, rect.width - 150, rect.height);
    var plusRect = new Rect(rect.x + rect.width - 150, rect.y, 150, rect.height);
    index = EditorGUI.IntPopup(popupRect, index, names, indices);

    if (GUI.Button(plusRect, "New Condition", DailyGoalEditor.AddButtonStyle)) {
      var result = (T)(Activator.CreateInstance(types[index]));
      return result;
    }
    return default(T);
  }

  #endregion


}
