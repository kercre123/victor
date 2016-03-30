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

  public static string[] _EventMap = new string[(int)CladGameEvent.COUNT];

  private static CladEventToAnimGroupMap _CurrentEventMap;
  private static string _CurrentEventMapFile;


  private static readonly HashSet<string> _RecentFiles = new HashSet<string>();

  public static string sAnimationGroupDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/animationGroups/"; } }

  public static string sEventMapDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/animationGroups/mapping"; } }

  public static string[] AnimationGroupNameOptions { get { return _AnimationGroupNameOptions; } }

  static AnimationGroupEventEditor() {
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
    if (Directory.Exists(sEventMapDirectory)) {
      _EventMapFiles = Directory.GetFiles(sAnimationGroupDirectory);
      _EventMapNameOptions = _EventMapFiles.Select(x => Path.GetFileNameWithoutExtension(x)).ToArray();
    }
    else {
      _EventMapFiles = new string[0];
      _EventMapNameOptions = _EventMapFiles;
    }

  }

  private bool CheckDiscardUnsaved() {
    bool canOpen = true;
    if (_CurrentEventMap != null && (string.IsNullOrEmpty(_CurrentEventMapFile) || JsonConvert.SerializeObject(_CurrentEventMap, Formatting.Indented, GlobalSerializerSettings.JsonSettings) != File.ReadAllText(_CurrentEventMapFile))) {
      canOpen = EditorUtility.DisplayDialog("Warning", "Opening an AnimationGroup will Discard Unsaved Changes. Are you Sure?", "Yes");
    }
    return canOpen;
  }

  private void LoadFile(string path) {
    if (CheckDiscardUnsaved()) {
      try {
        _CurrentEventMap = null;
        _CurrentEventMapFile = null;

        string json = File.ReadAllText(path);
        _CurrentEventMap = JsonConvert.DeserializeObject<CladEventToAnimGroupMap>(json, GlobalSerializerSettings.JsonSettings);
        _CurrentEventMapFile = path;
        _RecentFiles.Add(path);
      }
      catch (Exception ex) {
        DAS.Error(this, ex.Message);
      }
    }
  }

  public void OnGUI() {
    DrawToolbar();

    DrawCladEventList();
  }

  private void DrawToolbar() {
    

    GUILayout.FlexibleSpace();
  }

  private void DrawCladEventList() {
    EditorGUILayout.BeginVertical();
    //EditorDrawingUtility.DrawList("CLAD Events", _CurrentEventMap.AnimationGroups, DrawCladEventEntry, () => new CladEventMap.CladEventAnimationEntry());
    EditorGUILayout.EndVertical();
  }

  public CladEventToAnimGroupMap DrawCladEventEntry(CladEventToAnimGroupMap entry) {
    EditorGUILayout.BeginVertical();
    //EditorGUILayout.LabelField(entry.cladEvent.ToString());
    //entry.animationGroupName = _AnimationGroupNameOptions[EditorGUILayout.Popup("Animation Group", Mathf.Max(0, Array.IndexOf(_AnimationGroupNameOptions, entry.animationGroupName)), _AnimationGroupNameOptions)];
    EditorGUILayout.EndVertical();
    return entry;
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
