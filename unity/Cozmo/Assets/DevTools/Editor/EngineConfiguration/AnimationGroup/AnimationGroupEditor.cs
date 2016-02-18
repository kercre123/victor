using UnityEngine;
using System.Collections;
using UnityEditor;
using System;
using System.Collections.Generic;
using System.Linq;
using AnimationGroups;
using System.IO;
using Newtonsoft.Json;

public class AnimationGroupEditor : EditorWindow {

  private static string[] _AnimationGroupFiles;

  private static string[] _AnimationNameOptions;
  private static string[] _AnimationGroupNameOptions;

  private static AnimationGroup _CurrentAnimationGroup;
  private static string _CurrentAnimationGroupFile;
  private static string _CurrentAnimationGroupName;


  private static readonly HashSet<string> _RecentFiles = new HashSet<string>();

  public static string sAnimationDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/animations/"; } }
  public static string sAnimationGroupDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/animationGroups/"; } }

  public static string[] AnimationNameOptions { get { return _AnimationNameOptions; } }


  public static string[] AnimationGroupNameOptions { get { return _AnimationGroupNameOptions; } }

  static AnimationGroupEditor() {
    LoadAnimationGroups();
  }

  private static void LoadAnimationGroups() {
    if (Directory.Exists(sAnimationGroupDirectory)) {
      _AnimationGroupFiles = Directory.GetFiles(sAnimationGroupDirectory);
      _AnimationGroupNameOptions = _AnimationGroupFiles.Select(x => Path.GetFileNameWithoutExtension(x)).ToArray();
    }
    else {
      _AnimationGroupFiles = new string[0];
      _AnimationGroupNameOptions = _AnimationGroupFiles;
    }

    if (Directory.Exists(sAnimationDirectory)) {
      _AnimationNameOptions = Directory.GetFiles(sAnimationDirectory).Select(x => Path.GetFileNameWithoutExtension(x)).ToArray();
    }
    else {
      _AnimationNameOptions = new string[0];
    }
  }

  private bool CheckDiscardUnsavedAnimationGroup() {
    bool canOpen = true;
    if (_CurrentAnimationGroup != null && (string.IsNullOrEmpty(_CurrentAnimationGroupFile) || JsonConvert.SerializeObject(_CurrentAnimationGroup, Formatting.Indented, GlobalSerializerSettings.JsonSettings) != File.ReadAllText(_CurrentAnimationGroupFile))) {
      canOpen = EditorUtility.DisplayDialog("Warning", "Opening an AnimationGroup will Discard Unsaved Changes. Are you Sure?", "Yes");
    }
    return canOpen;
  }

  private void LoadFile(string path) {
    if (CheckDiscardUnsavedAnimationGroup()) {
      try {
        _CurrentAnimationGroup = null;
        _CurrentAnimationGroupFile = null;
        _CurrentAnimationGroupName = null;

        string json = File.ReadAllText(path);
        _CurrentAnimationGroup = JsonConvert.DeserializeObject<AnimationGroup>(json, GlobalSerializerSettings.JsonSettings);
        _CurrentAnimationGroupFile = path;
        _CurrentAnimationGroupName = Path.GetFileNameWithoutExtension(path);
        _RecentFiles.Add(path);
      }
      catch (Exception ex) {
        Debug.LogException(ex);
      }
    }
  }

  public void OnGUI() {
    DrawToolbar();

    if (_CurrentAnimationGroup != null) {

      _CurrentAnimationGroupName = EditorGUILayout.TextField("Name", _CurrentAnimationGroupName ?? string.Empty);

      DrawAnimationGroup(_CurrentAnimationGroup);
    }
  }

  private void DrawToolbar() {

    EditorGUILayout.BeginHorizontal(EditorDrawingUtility.ToolbarStyle);

    if (GUILayout.Button("Load", EditorDrawingUtility.ToolbarButtonStyle)) {
      GenericMenu menu = new GenericMenu();

      foreach (var file in _AnimationGroupFiles.Where(x => x.EndsWith(".json"))) {
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

    if (GUILayout.Button("New AnimationGroup", EditorDrawingUtility.ToolbarButtonStyle)) {
      if (CheckDiscardUnsavedAnimationGroup()) {
        _CurrentAnimationGroup = new AnimationGroup();
        GUI.FocusControl("EditNameField");
        _CurrentAnimationGroupFile = null;
      }
    }


    if (_CurrentAnimationGroup != null) {
      if (GUILayout.Button("Save", EditorDrawingUtility.ToolbarButtonStyle)) {         
        if (string.IsNullOrEmpty(_CurrentAnimationGroupFile)) {
          _CurrentAnimationGroupFile = EditorUtility.SaveFilePanel("Save AnimationGroup", sAnimationGroupDirectory, _CurrentAnimationGroupName, "json");
        }

        if (!string.IsNullOrEmpty(_CurrentAnimationGroupFile)) {
          if (_CurrentAnimationGroup.Animations.Count == 0) {
            EditorUtility.DisplayDialog("Error!", "Cannot save a group with no Animations!", "OK");
          }
          else {
            string newFileName = Path.Combine(sAnimationGroupDirectory, _CurrentAnimationGroupName + ".json");

            bool good = true;
            if (Path.GetFileName(_CurrentAnimationGroupFile) != Path.GetFileName(newFileName)) {
              if (EditorUtility.DisplayDialog("Alert!", "AnimationGroup has been renamed. Save Anyway?", "Yes", "No")) {
                if (File.Exists(_CurrentAnimationGroupFile) && EditorUtility.DisplayDialog("Alert!", "Should we delete the old file?\n" + _CurrentAnimationGroupFile, "Yes", "No")) {
                  File.Delete(_CurrentAnimationGroupFile);
                }
                _CurrentAnimationGroupFile = newFileName;
              }
              else {
                good = false;
              }
            }

            if (good) {
              _RecentFiles.Add(_CurrentAnimationGroupFile);

              File.WriteAllText(_CurrentAnimationGroupFile, JsonConvert.SerializeObject(_CurrentAnimationGroup, Formatting.Indented, GlobalSerializerSettings.JsonSettings));

              // Reload groups
              LoadAnimationGroups();
              EditorUtility.DisplayDialog("Save Successful!", "AnimationGroup '" + _CurrentAnimationGroupName + "' has been saved to " + _CurrentAnimationGroupFile, "OK");
            }
          }
        }
      }
    }

    GUILayout.FlexibleSpace();

    EditorGUILayout.EndHorizontal();
  }

  private AnimationGroup DrawAnimationGroup(AnimationGroup animGroup) {
    EditorDrawingUtility.DrawGroupedList("Animations", animGroup.Animations, DrawAnimationGroupEntry, () => new AnimationGroup.AnimationGroupEntry(), x => x.Mood, m => m.ToString());
    return animGroup;
  }

  public AnimationGroup.AnimationGroupEntry DrawAnimationGroupEntry(AnimationGroup.AnimationGroupEntry entry) {
    entry.Name = _AnimationNameOptions[EditorGUILayout.Popup("Animation Name", Mathf.Max(0, Array.IndexOf(_AnimationNameOptions, entry.Name)), _AnimationNameOptions)];

    entry.Mood = (Anki.Cozmo.SimpleMoodType)EditorGUILayout.EnumPopup("Mood", entry.Mood);

    entry.Weight = EditorGUILayout.FloatField("Weight", entry.Weight);

    return entry;
  }
    
  [MenuItem("Cozmo/Animation Group Editor %g")]
  public static void OpenAnimationGroupEditor() {
    AnimationGroupEditor window = (AnimationGroupEditor)EditorWindow.GetWindow(typeof(AnimationGroupEditor));
    window.titleContent = new GUIContent("Animation Group Editor");
    window.Show();
  }


}
