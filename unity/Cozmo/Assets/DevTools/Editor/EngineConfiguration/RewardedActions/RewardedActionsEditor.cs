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
/// Rewarded Actions Editor. Used for creating and editing generic rewards.
/// </summary>
public class RewardedActionsEditor : EditorWindow {

  private static bool _IsOpen = false;


  private static string[] _RewardFiles;
  private static string[] _RewardFileOptions;
  private static string[] _FilteredCladNameOptions;
  private static GameEvent[] _FilteredCladList;

  private static RewardedActionListData _CurrentRewardData;
  private static string _CurrentRewardFileName;
  private static string _CurrentRewardFile;
  private static string _EventSearchField = "";
  private static Vector2 _scrollPos = new Vector2();

  // toggle state for foldouts of individual goals
  // uint corresponds to a unique ID
  // bool corresponds to whether or not the foldout is expanded
  private Dictionary<uint, bool> _RewardFoldouts = new Dictionary<uint, bool>();
  // uint refers to the ID of the Reward (that ID must be set to true in RewardFoldouts to reveal conditions)
  // If true in RewardFoldouts, then refer to the CondFoldouts to determine which Condition list
  private Dictionary<uint, bool> _ConditionFoldouts = new Dictionary<uint, bool>();

  private static bool _IdMapComplete = false;

  // get a uint 1 higher than the highest node's Id
  private uint GetNextRewardId() {
    var data = _CurrentRewardData;

    if (data == null) {
      return 0;
    }

    uint maxId = 0;
    for (int i = 0; i < data.RewardedActions.Count; i++) {
      var reward = data.RewardedActions[i];

      maxId = reward.Id > maxId ? reward.Id : maxId;
    }
    return maxId + 1;
  }

  public static string sRewardedActionsDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/RewardedActions"; } }

  public static string sCSVDirectory { get { return Environment.GetFolderPath(Environment.SpecialFolder.Desktop); } }

  public static string[] RewardFileNameOptions { get { return _RewardFileOptions; } }

  const string kRewardedActionsCSV = "RewardedActions.csv";

  static RewardedActionsEditor() {
    LoadData();
  }

  private static void LoadData() {
    // Load all Event Map Configs (Can have multiple, so you can create different configs, game only uses one.)
    if (Directory.Exists(sRewardedActionsDirectory)) {
      _RewardFiles = Directory.GetFiles(sRewardedActionsDirectory);
      _RewardFileOptions = _RewardFiles.Select(x => Path.GetFileNameWithoutExtension(x)).ToArray();
    }
    else {
      Directory.CreateDirectory(sRewardedActionsDirectory);
      _RewardFiles = new string[0];
      _RewardFileOptions = _RewardFiles;
    }
  }

  private bool CheckDiscardUnsaved() {
    bool canOpen = true;

    if (_CurrentRewardData != null && (string.IsNullOrEmpty(_CurrentRewardFileName) || JsonConvert.SerializeObject(_CurrentRewardData, Formatting.Indented, GlobalSerializerSettings.JsonSettings) != File.ReadAllText(_CurrentRewardFileName))) {
      canOpen = EditorUtility.DisplayDialog("Warning", "Opening a different file will Discard Unsaved Changes. Are you Sure?", "Yes", "No");
    }

    return canOpen;
  }

  private void LoadFile(string path) {
    if (CheckDiscardUnsaved()) {
      try {
        _CurrentRewardData = null;
        _CurrentRewardFile = null;

        string json = File.ReadAllText(path);
        _CurrentRewardData = JsonConvert.DeserializeObject<RewardedActionListData>(json, GlobalSerializerSettings.JsonSettings);
        _CurrentRewardFileName = Path.GetFileNameWithoutExtension(path);
        _CurrentRewardFile = path;
        RemapEditorIDs();
      }
      catch (Exception ex) {
        Debug.LogError(ex.Message);
      }
    }
  }

  public void RemapEditorIDs() {
    RewardedActionData existingEntry;
    _ConditionFoldouts.Clear();
    _RewardFoldouts.Clear();
    for (int i = 0; i < _CurrentRewardData.RewardedActions.Count; i++) {
      existingEntry = _CurrentRewardData.RewardedActions[i];
      // Only grab a new id if we have intersections or
      // there is no ID established
      if (existingEntry.Id == 0) {
        existingEntry.Id = GetNextRewardId();
      }
      _RewardFoldouts.Add(existingEntry.Id, false);
      _ConditionFoldouts.Add(existingEntry.Id, false);
    }
    RewardedActionsEditor._IdMapComplete = true;
  }

  public void OnGUI() {
    DrawToolbar();

    if (_CurrentRewardData != null) {
      if (RewardedActionsEditor._IdMapComplete == false) {
        RemapEditorIDs();
      }
      _CurrentRewardFileName = EditorGUILayout.TextField("File Name", _CurrentRewardFileName ?? string.Empty);
      string eventFilter = EditorGUILayout.TextField("Filter CLAD Events", _EventSearchField ?? string.Empty);
      // Only filter if the event filter has been changed
      if (eventFilter != _EventSearchField) {
        _EventSearchField = eventFilter;
        FilterRewardNames();
      }
      DrawRewardedActionData(_CurrentRewardData);
    }
  }

  // Filter Event Names
  private void FilterRewardNames() {
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

      foreach (var file in _RewardFiles.Where(x => x.EndsWith(".json"))) {
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
    if (GUILayout.Button("New Rewarded Actions File", EditorDrawingUtility.ToolbarButtonStyle)) {
      if (CheckDiscardUnsaved()) {
        _CurrentRewardData = new RewardedActionListData();
        _CurrentRewardFileName = string.Empty;
        _CurrentRewardFile = null;
        GUI.FocusControl("EditNameField");
      }
    }
    // Save Current Event Map Button
    if (_CurrentRewardData != null) {
      if (GUILayout.Button("Save", EditorDrawingUtility.ToolbarButtonStyle)) {
        if (string.IsNullOrEmpty(_CurrentRewardFile)) {
          _CurrentRewardFile = EditorUtility.SaveFilePanel("Save EventMap", sRewardedActionsDirectory, _CurrentRewardFileName, "json");
        }

        if (!string.IsNullOrEmpty(_CurrentRewardFile)) {
          if (string.IsNullOrEmpty(_CurrentRewardFileName)) {
            EditorUtility.DisplayDialog("Error!", "Name your file before you save it!", "Ok");
          }
          else if (_CurrentRewardData.RewardedActions.Count == 0) {
            EditorUtility.DisplayDialog("Error!", "Cannot save with no Rewarded Actions!", "Whatever man...");
          }
          else {
            string newFileName = Path.Combine(sRewardedActionsDirectory, _CurrentRewardFileName + ".json");

            bool good = true;
            if (Path.GetFileName(_CurrentRewardFile) != Path.GetFileName(newFileName)) {
              if (EditorUtility.DisplayDialog("Alert!", "Rewarded Action File has been renamed. Save Anyway?", "Yes", "No")) {
                if (File.Exists(_CurrentRewardFile) && EditorUtility.DisplayDialog("Hold up!", "Should we delete this old file?\n" + _CurrentRewardFile, "Yes", "No")) {
                  File.Delete(_CurrentRewardFile);
                }
                _CurrentRewardFile = newFileName;
              }
              else {
                good = false;
              }
            }

            if (good) {

              File.WriteAllText(_CurrentRewardFile, JsonConvert.SerializeObject(_CurrentRewardData, Formatting.Indented, GlobalSerializerSettings.JsonSettings));
              if (EditorUtility.DisplayDialog("Alert!", "Would you like to output a .csv to Desktop?", "Yes", "No")) {
                GenerateCSV();
              }
              // Reload groups
              LoadData();
              EditorUtility.DisplayDialog("Save Successful!", "EventMap '" + _CurrentRewardFileName + "' has been saved to " + _CurrentRewardFile, "OK");
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
    string toCSV = "Description,Reward,Tag\n";
    for (int i = 0; i < _CurrentRewardData.RewardedActions.Count; i++) {
      RewardedActionData data = _CurrentRewardData.RewardedActions[i];
      toCSV += string.Format("{0},{1}\n", LocalizationEditorUtility.GetTranslationSansFormatting(data.Reward.DescriptionKey),
                             data.Reward.Amount);
    }
    string targetCSV = Path.Combine(sCSVDirectory, kRewardedActionsCSV);
    if (File.Exists(targetCSV)) {
      File.Delete(targetCSV);
    }
    File.WriteAllText(targetCSV, toCSV);
  }

  private void DrawRewardedActionData(RewardedActionListData data) {

    EditorGUILayout.BeginVertical();
    _scrollPos = EditorGUILayout.BeginScrollView(_scrollPos);
    EditorGUIUtility.labelWidth = 100;
    EditorDrawingUtility.DrawFilteredGroupedList("Rewarded Actions", data.RewardedActions, DrawRewardedActionEntry, AddRewardedActionEntry, x => x.RewardEvent.Value,
      _EventSearchField, GameEvent.Count, e => e.ToString());
    EditorGUILayout.EndScrollView();
    EditorGUILayout.EndVertical();

  }

  private RewardedActionData AddRewardedActionEntry() {
    RewardedActionData newAction = new RewardedActionData();
    newAction.Id = GetNextRewardId();
    // If the GetNextRewardId returns a key that's still in the Dictionaries, it is because
    // we have removed the data but not cleansed the Dictionary of that ID, free up
    // the ID.
    if (_RewardFoldouts.ContainsKey(newAction.Id)) {
      _RewardFoldouts.Remove(newAction.Id);
      _ConditionFoldouts.Remove(newAction.Id);
    }
    _RewardFoldouts.Add(newAction.Id, false);
    _ConditionFoldouts.Add(newAction.Id, false);
    return newAction;
  }

  private void CopyRewardedActionEntry(RewardedActionData genData) {
    RewardedActionData copy = genData.Copy();
    copy.Id = GetNextRewardId();
    // If the GetNextRewardId returns a key that's still in the Dictionaries, it is because
    // we have removed the genData but not cleansed the Dictionary of that ID, free up
    // the ID.
    if (_RewardFoldouts.ContainsKey(copy.Id)) {
      _RewardFoldouts.Remove(copy.Id);
      _ConditionFoldouts.Remove(copy.Id);
    }
    _RewardFoldouts.Add(copy.Id, false);
    _ConditionFoldouts.Add(copy.Id, false);
    _CurrentRewardData.RewardedActions.Add(copy);
  }

  public RewardedActionData DrawRewardedActionEntry(RewardedActionData data) {
    string rewardDescription = LocalizationEditorUtility.GetTranslationSansFormatting(data.Reward.DescriptionKey);
    bool isExpanded = false;
    EditorGUILayout.BeginVertical();
    if (!_RewardFoldouts.TryGetValue(data.Id, out isExpanded)) {
      return data;
    }

    if (isExpanded) {
      GUI.contentColor = Color.green;
    }
    bool foldoutOpen = EditorGUI.Foldout(EditorGUILayout.GetControlRect(), isExpanded, rewardDescription, EditorDrawingUtility.FoldoutStyle);
    _RewardFoldouts[data.Id] = foldoutOpen;
    GUI.contentColor = Color.white;
    // If the Reward is not expanded, Only show the title
    if (!foldoutOpen) {
      EditorGUILayout.EndVertical();
      return data;
    }
    else {
      GUI.backgroundColor = new Color(0.75f, 0.75f, 1.0f);
    }

    if (GUILayout.Button(string.Format("Delete {0}", rewardDescription), EditorDrawingUtility.RemoveButtonStyle)) {
      if (EditorUtility.DisplayDialog("Hold up!", "Should we delete this Reward?\n" + rewardDescription, "Yes", "No")) {
        _CurrentRewardData.RewardedActions.Remove(data);
      }
    }

    if (GUILayout.Button(string.Format("Copy {0}", rewardDescription), EditorDrawingUtility.AddButtonStyle)) {
      CopyRewardedActionEntry(data);
    }

    DrawLabelFields(data);

    DrawConditionFoldouts(data);

    DrawRewardFields(data);

    EditorGUILayout.EndVertical();
    GUI.backgroundColor = Color.white;
    return data;
  }

  public void DrawLabelFields(RewardedActionData data) {

    EditorGUILayout.LabelField("Rewarded Action", EditorDrawingUtility.SubtitleStyle);
    if (string.IsNullOrEmpty(_EventSearchField)) {
      data.RewardEvent.Value = (GameEvent)EditorGUILayout.EnumPopup("GameEvent", data.RewardEvent.Value);
    }
    else {
      data.RewardEvent.Value = _FilteredCladList[EditorGUILayout.Popup("GameEvent", Mathf.Max(0, Array.IndexOf(_FilteredCladNameOptions, data.RewardEvent.Value.ToString())), _FilteredCladNameOptions)];
    }

    EditorGUILayout.LabelField("Localization", EditorDrawingUtility.SubtitleStyle);
    EditorDrawingUtility.DrawLocalizationString(ref data.Reward.DescriptionKey);
  }

  public void DrawConditionFoldouts(RewardedActionData data) {

    bool condExpanded = false;

    if (!_ConditionFoldouts.TryGetValue(data.Id, out condExpanded)) {
      _ConditionFoldouts.Add(data.Id, false);
    }
    //Draw lists of conditions here
    bool conditionsOpen = condExpanded;

    if (conditionsOpen) {
      GUI.contentColor = Color.green;
    }
    conditionsOpen = EditorGUI.Foldout(EditorGUILayout.GetControlRect(), conditionsOpen,
      new GUIContent(string.Format("Reward Conditions{0}", (data.EventConditions.Count > 0 ? EditorDrawingUtility.GetConditionListString(data.EventConditions) : "")),
        "Conditions that must be met for the Reward to be earned"), EditorDrawingUtility.FoldoutStyle);
    GUI.contentColor = Color.white;
    if (conditionsOpen) {
      EditorDrawingUtility.DrawConditionList(data.EventConditions);
    }

    _ConditionFoldouts[data.Id] = conditionsOpen;
  }

  public void DrawRewardFields(RewardedActionData data) {
    EditorGUILayout.LabelField("Reward", EditorDrawingUtility.SubtitleStyle);
    data.Reward.ItemID = EditorGUILayout.TextField("Reward", data.Reward.ItemID);
    data.Reward.Amount = EditorGUILayout.IntField("Count", data.Reward.Amount);
  }

  [MenuItem("Cozmo/Progression/Rewarded Actions Editor #%r")]
  public static void OpenRewardedActionsEditor() {
    RewardedActionsEditor window = (RewardedActionsEditor)EditorWindow.GetWindow(typeof(RewardedActionsEditor));
    if (_IsOpen) {
      window.Close();
      _IsOpen = false;
    }
    else {
      RewardedActionsEditor.LoadData();
      window.titleContent = new GUIContent("Rewarded Actions Editor");
      window.Show();
      window.Focus();
      window.position = new Rect(0, 0, 600, 600);
      _IsOpen = true;
      _IdMapComplete = false;
    }
  }


}
