using UnityEngine;
using System.Collections;
using UnityEditor;
using System;

using System.Reflection;
using System.Linq;
using System.Collections.Generic;

public class ScriptableObjectMenu : EditorWindow {

  private class InheritanceTree {
    public Type Type;
    public readonly List<InheritanceTree> SubTypes = new List<InheritanceTree>();
    public bool Foldout;

    public InheritanceTree(Type type) {
      Type = type;
    }
  }

  private static List<InheritanceTree> _Types = new List<InheritanceTree>();

  static ScriptableObjectMenu() {
    // get all types that are in Assembly-CSharp
    var allTypes = typeof(HubWorldBase).Assembly.GetTypes()
      .Where(t => typeof(ScriptableObject).IsAssignableFrom(t)).ToDictionary(t => t, t => new InheritanceTree(t));

    foreach (var t in allTypes) {
      InheritanceTree baseTree;
      if (allTypes.TryGetValue(t.Key.BaseType, out baseTree)) {
        baseTree.SubTypes.Add(t.Value);
      }
      else {
        _Types.Add(t.Value);
      }
    }

  }

  private string _Path;

  private Vector2 _ScrollPosition;

  private void OnGUI() {
  
    _ScrollPosition = EditorGUILayout.BeginScrollView(_ScrollPosition);


    foreach (var type in _Types) {
      DrawTypeControl(type);
    }

    EditorGUILayout.EndScrollView();
  }

  private void DrawTypeControl(InheritanceTree treeType) {
    if (treeType.SubTypes.Count == 0) {
      EditorGUI.indentLevel++;
      if (GUILayout.Button(treeType.Type.Name)) {
        CreateType(treeType.Type, _Path);
      }
      EditorGUI.indentLevel--;
    }
    else {      
      EditorGUILayout.BeginHorizontal();
      treeType.Foldout = EditorGUILayout.Foldout(treeType.Foldout, "");

      if (treeType.Type.IsAbstract) {
        GUILayout.Label(treeType.Type.Name);
      }
      else {
        if (GUILayout.Button(treeType.Type.Name)) {
          CreateType(treeType.Type, _Path);
        }
      }

      EditorGUILayout.EndHorizontal();
      EditorGUI.indentLevel++;
      if (treeType.Foldout) {
        foreach (var subType in treeType.SubTypes) {
          DrawTypeControl(subType);
        }
      }
      EditorGUI.indentLevel--;
    }
  }

  private void CreateType(Type type, string path) {
    var obj = ScriptableObject.CreateInstance(type);

    string filename = System.IO.Path.Combine(path, "New " + obj.GetType().Name);

    int suffixNum = 0;
    string suffix = "";
    while (System.IO.File.Exists(filename + suffix + ".asset")) {
      suffixNum++;
      suffix = " " + suffixNum;
    }

    UnityEditor.AssetDatabase.CreateAsset(obj, filename + suffix + ".asset");
  }


  [UnityEditor.MenuItem("Assets/Create/ScriptableObject")]
  public static void CreateNewMenuOption() {
    var path = Application.dataPath;

    if (UnityEditor.Selection.objects.Length > 0) {
      path = UnityEditor.AssetDatabase.GetAssetPath(UnityEditor.Selection.objects[0]);
    }

    if (!System.IO.Directory.Exists(path)) {
      path = System.IO.Path.GetDirectoryName(path);
    }

    // Get existing open window or if none, make a new one:
    ScriptableObjectMenu window = (ScriptableObjectMenu)EditorWindow.GetWindow(typeof(ScriptableObjectMenu));
    window._Path = path;
    window.minSize = new Vector2(200, 300);
    window.Show();
  }
}
