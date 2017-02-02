using UnityEngine;
using System.Collections;
using UnityEditor;
using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using Newtonsoft.Json;
using Anki.Cozmo;
using System.Text.RegularExpressions;

public class BehaviorEditor : EditorWindow {

  private static string[] _BehaviorFiles;

  private static string[] _BehaviorNameOptions;

  private static string _CopyBuffer;

  private static Behavior _CurrentBehavior;
  private static string _CurrentBehaviorFile;
  private static string _CurrentBehaviorName;


  private static readonly HashSet<string> _RecentFiles = new HashSet<string>();

  public static string sBehaviorDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/behaviors/"; } }

  public static string sBehaviorCppDirectory { get { return Application.dataPath + "/../../../basestation/src/anki/cozmo/basestation/behaviors/"; } }

  public static string sBehaviorHDirectory { get { return Application.dataPath + "/../../../basestation/include/anki/cozmo/basestation/behaviors/"; } }


  public static string[] BehaviorNameOptions { get { return _BehaviorNameOptions; } }

  static BehaviorEditor() {
    LoadBehaviors();
  }

  private static string FindFile(string fileName, string rootPath) {
    var path = Path.Combine(rootPath, fileName);
    if (File.Exists(path)) {
      return path;
    }

    foreach (var dir in Directory.GetDirectories(rootPath)) {
      path = FindFile(fileName, dir);
      if (path != null) {
        return path;
      }
    }
    return null;
  }

  private static string FindScoredCppFile(BehaviorClass behaviorType) {
    string fileName = "behavior" + behaviorType.ToString() + ".cpp";

    return FindFile(fileName, sBehaviorCppDirectory);
  }

  private static string FindReactionaryCppFile(ReactionTrigger behaviorType) {
    string fileName = "behavior" + behaviorType.ToString() + ".cpp";

    return FindFile(fileName, sBehaviorCppDirectory);
  }
    
  private static string FindScoredHFile(BehaviorClass behaviorType) {
    string fileName = "behavior" + behaviorType.ToString() + ".h";

    return FindFile(fileName, sBehaviorHDirectory);
  }

  private static string FindReactionaryHFile(ReactionTrigger behaviorType) {
    string fileName = "behavior" + behaviorType.ToString() + ".h";

    return FindFile(fileName, sBehaviorHDirectory);
  }

  // a heuristic to predict the type of a field based on its name.
  private static object GuessTypeByName(string name) {
    if (name.EndsWith("name", StringComparison.InvariantCultureIgnoreCase)) {
      return string.Empty;
    }
    if (name.EndsWith("_s") || name.EndsWith("_mm") || name.EndsWith("_m") ||
        name.EndsWith("_mmps") || name.EndsWith("_sec") || name.EndsWith("_rad")
        || name.EndsWith("_deg")) {
      return 0f;
    }

    if (name.EndsWith("_ms") || name.IndexOf("index", StringComparison.InvariantCultureIgnoreCase) != -1
        || name.IndexOf("length", StringComparison.InvariantCultureIgnoreCase) != -1
        || name.IndexOf("num", StringComparison.InvariantCultureIgnoreCase) != -1) {
      return 0;
    }

    if (name.StartsWith("is", StringComparison.InvariantCultureIgnoreCase) ||
        name.StartsWith("can", StringComparison.InvariantCultureIgnoreCase) ||
        name.EndsWith("allowed", StringComparison.InvariantCultureIgnoreCase)) {
      return false;
    }

    return string.Empty;
  }

  private static void TryFindCustomParameters(Dictionary<string, object> customParams, BehaviorClass behaviorType) {

    string cppFile = FindScoredCppFile(behaviorType);

    Dictionary<string, object> newParams = new Dictionary<string, object>();

    if (cppFile == null) {
      return;
    }

    Regex constCharStarRegex = new Regex("static const char\\* k(\\S+) = \"(\\S+)\";");
    Regex includeRegex = new Regex("#include \"(\\S+)\"");

    List<string> cppFiles = new List<string>();

    cppFiles.Add(cppFile);


    var header = FindScoredHFile(behaviorType);

    string[] lines;
    Match match;   

    while (header != null) {
      lines = File.ReadAllLines(header);
      header = null;

      foreach (var line in lines) {        
        match = includeRegex.Match(line);

        if (match.Success) {
        
          string path = match.Groups[1].Value;

          DAS.Debug(typeof(BehaviorEditor), "Found include path: " + path);

          string fileName = Path.GetFileName(path);

          // hit the base class.
          if (fileName == "behaviorInterface.h") {
            continue;
          }

          string cppFileName = fileName.Replace(".h", ".cpp");

          header = FindFile(fileName, sBehaviorHDirectory);

          cppFile = FindFile(cppFileName, sBehaviorCppDirectory);

          if (cppFile != null) {
            cppFiles.Add(cppFile);
          }
        }
      }
    }
      
    foreach (var file in cppFiles) {
      lines = File.ReadAllLines(file);

      foreach (var line in lines) {
        match = constCharStarRegex.Match(line);

        if (match.Success) {
          
          string name = match.Groups[2].Value;

          if (!customParams.ContainsKey(name)) {
            newParams[name] = GuessTypeByName(name);
          }
          else {
            newParams[name] = customParams[name];
          }
        }
      }
    }
    customParams.Clear();
    foreach (var kvp in newParams) {
      customParams[kvp.Key] = kvp.Value;
    }
  }


  private static void LoadBehaviors() {
    if (Directory.Exists(sBehaviorDirectory)) {
      _BehaviorFiles = Directory.GetFiles(sBehaviorDirectory);
      _BehaviorNameOptions = _BehaviorFiles.Select(x => Path.GetFileNameWithoutExtension(x)).ToArray();
    }
    else {
      _BehaviorFiles = new string[0];
      _BehaviorNameOptions = _BehaviorFiles;
    }
  }

  private bool CheckDiscardUnsavedBehavior() {
    bool canOpen = true;
    if (_CurrentBehavior != null && (string.IsNullOrEmpty(_CurrentBehaviorFile) || JsonConvert.SerializeObject(_CurrentBehavior, Formatting.Indented, GlobalSerializerSettings.JsonSettings) != File.ReadAllText(_CurrentBehaviorFile))) {
      canOpen = EditorUtility.DisplayDialog("Warning", "Opening an Behavior will Discard Unsaved Changes. Are you Sure?", "Yes");
    }
    return canOpen;
  }

  private void LoadFile(string path) {
    if (CheckDiscardUnsavedBehavior()) {
      try {
        _CurrentBehavior = null;
        _CurrentBehaviorFile = null;
        _CurrentBehaviorName = null;

        string json = File.ReadAllText(path);

        var regex = new Regex("//.*");

        // remove // comments
        json = regex.Replace(json, string.Empty);

        _CurrentBehavior = JsonConvert.DeserializeObject<Behavior>(json, GlobalSerializerSettings.JsonSettings);
        _CurrentBehaviorFile = path;
        _CurrentBehaviorName = Path.GetFileNameWithoutExtension(path);
        _RecentFiles.Add(path);
      }
      catch (Exception ex) {
        DAS.Error(this, ex.Message);
      }
    }
  }

  public void OnGUI() {
    DrawToolbar();

    if (_CurrentBehavior != null) {

      _CurrentBehaviorName = EditorGUILayout.TextField("File Name", _CurrentBehaviorName ?? string.Empty);

      DrawBehavior(_CurrentBehavior);
    }
  }

  private void DrawToolbar() {

    EditorGUILayout.BeginHorizontal(EditorDrawingUtility.ToolbarStyle);

    if (GUILayout.Button("Load", EditorDrawingUtility.ToolbarButtonStyle)) {
      GenericMenu menu = new GenericMenu();

      foreach (var file in _BehaviorFiles.Where(x => x.EndsWith(".json"))) {
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

    if (GUILayout.Button("New Behavior", EditorDrawingUtility.ToolbarButtonStyle)) {
      if (CheckDiscardUnsavedBehavior()) {
        _CurrentBehavior = new Behavior();
        //TryFindCustomParameters(_CurrentBehavior.CustomParams, _CurrentBehavior.BehaviorType);

        GUI.FocusControl("EditNameField");
        _CurrentBehaviorFile = null;
        _CurrentBehaviorName = null;
      }
    }


    if (_CurrentBehavior != null) {
      if (GUILayout.Button("Save", EditorDrawingUtility.ToolbarButtonStyle)) {         
        if (string.IsNullOrEmpty(_CurrentBehaviorFile)) {
          _CurrentBehaviorFile = EditorUtility.SaveFilePanel("Save Behavior", sBehaviorDirectory, _CurrentBehaviorName, "json");
        }

        if (!string.IsNullOrEmpty(_CurrentBehaviorFile)) {
        
          string newFileName = Path.Combine(sBehaviorDirectory, _CurrentBehaviorName + ".json");

          bool good = true;
          if (Path.GetFileName(_CurrentBehaviorFile) != Path.GetFileName(newFileName)) {
            if (EditorUtility.DisplayDialog("Alert!", "Behavior has been renamed. Save Anyway?", "Yes", "No")) {
              if (File.Exists(_CurrentBehaviorFile) && EditorUtility.DisplayDialog("Alert!", "Should we delete the old file?\n" + _CurrentBehaviorFile, "Yes", "No")) {
                File.Delete(_CurrentBehaviorFile);
              }
              _CurrentBehaviorFile = newFileName;
            }
            else {
              good = false;
            }
          }

          if (good) {
            _RecentFiles.Add(_CurrentBehaviorFile);

            File.WriteAllText(_CurrentBehaviorFile, JsonConvert.SerializeObject(_CurrentBehavior, Formatting.Indented, GlobalSerializerSettings.JsonSettings));

            // Reload groups
            LoadBehaviors();
            EditorUtility.DisplayDialog("Save Successful!", "Behavior '" + _CurrentBehaviorName + "' has been saved to " + _CurrentBehaviorFile, "OK");
          }
        }

      }
      if (GUILayout.Button("Copy", EditorDrawingUtility.ToolbarButtonStyle)) {
        _CopyBuffer = JsonConvert.SerializeObject(_CurrentBehavior, Formatting.None, GlobalSerializerSettings.JsonSettings);
      }
    }

    if (_CopyBuffer != null) {
      if (GUILayout.Button("Paste", EditorDrawingUtility.ToolbarButtonStyle)) {
        _CurrentBehavior = JsonConvert.DeserializeObject<Behavior>(_CopyBuffer, GlobalSerializerSettings.JsonSettings);
      }
    }


    GUILayout.FlexibleSpace();

    EditorGUILayout.EndHorizontal();
  }

  public Behavior DrawBehavior(Behavior entry) {
    EditorGUILayout.BeginVertical();

    /**var lastBehaviorType;
    switch (entry.BehaviorCategory) {
      case BehaviorCategory.NoneCategory:
      case BehaviorCategory.Count:
      {
        lastBehaviorType = BehaviorClass.NoneBehavior;
        break;
      }
      case BehaviorCategory.Scored:
      {
        lastBehaviorType = entry.BehaviorClass;
        break;
      }
      case BehaviorCategory.Reactionary:
      {
        lastBehaviorType = entry.ReacitonaryBehaviorType;
        break;
      }
    }
    entry.BehaviorType = (BehaviorType)EditorGUILayout.EnumPopup("Behavior Type", entry.BehaviorType);

    if (entry.BehaviorType != lastBehaviorType) {
      TryFindCustomParameters(entry.CustomParams, entry.BehaviorType);
    }**/

    entry.Name = EditorGUILayout.TextField("Name", entry.Name ?? string.Empty);

    EditorDrawingUtility.DrawDictionary("Behavior Specific Parameters", entry.CustomParams, EditorDrawingUtility.DrawSelectionObjectEditor, () => string.Empty);

    EditorDrawingUtility.DrawList("Emotion Scorers", entry.EmotionScorers, EditorDrawingUtility.DrawEmotionScorer, () => new EmotionScorer());

    entry.RepetitionPenalty = EditorGUILayout.CurveField("Repetition Penalty", entry.RepetitionPenalty);

    EditorDrawingUtility.DrawList("Behavior Groups", entry.BehaviorGroups, bg => (BehaviorGroup)EditorGUILayout.EnumPopup(bg), () => (BehaviorGroup)0);

    EditorGUILayout.EndVertical();
    return entry;
  }


  [MenuItem("Cozmo/Behavior Editor %y")]
  public static void OpenBehaviorEditor() {
    BehaviorEditor window = (BehaviorEditor)EditorWindow.GetWindow(typeof(BehaviorEditor));
    window.titleContent = new GUIContent("Behavior Editor");
    window.Show();
  }


}
