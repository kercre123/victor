using System;
using UnityEditor;
using System.Reflection;
using System.Linq;
using UnityEngine;
using System.IO;

namespace AssemblyCSharpEditor {
  public class MinigameConfigMenu : EditorWindow {


    private static string[] _typeNames;

    private static Type[] _types;

    private static int[] _indices;

    private int _selectedIndex = 0;

    static MinigameConfigMenu()
    {
      var types = Assembly.GetAssembly(typeof(MinigameConfigBase)).GetTypes().Where(t => typeof(MinigameConfigBase).IsAssignableFrom(t) && t != typeof(MinigameConfigBase));
      _types = types.ToArray();
      _typeNames = types.Select(x => x.Name).ToArray();

      _indices = new int[_typeNames.Length];
      for (int i = 0; i < _indices.Length; i++) {
        _indices[i] = i;
      }
    }

    void OnGUI() {
      _selectedIndex = EditorGUILayout.IntPopup("Type", _selectedIndex, _typeNames, _indices);


      var type = _types[_selectedIndex];

      if (GUILayout.Button("Create")) {
        var path = EditorUtility.SaveFilePanel("New Config", "Assets/SharedAssets/Resources/Challenges", type.Name.Replace("Config","")+"Challenge", "asset");

        if (!string.IsNullOrEmpty(path)) {
          var challengeDataInstance = ScriptableObject.CreateInstance(typeof(ChallengeData)) as ChallengeData;
          var configInstance = ScriptableObject.CreateInstance(type) as MinigameConfigBase;
            
          if (path.StartsWith(Application.dataPath) && path.Contains("Resources")) {
            path = "Assets" + path.Substring(Application.dataPath.Length);
            string path2 = path.Replace(".asset", "_Config.asset");

            AssetDatabase.CreateAsset(configInstance, path2);

            challengeDataInstance.MinigameConfig = configInstance;

            AssetDatabase.CreateAsset(challengeDataInstance, path);

            UnityEditor.Selection.activeObject = challengeDataInstance;
          }
          else {
            DAS.Error(this, "Config must be in a Resources Folder!");
          }
        }
      }
    }

    [MenuItem("Cozmo/Create Challenge Config")]
    public static void OpenMinigameConfigMenu()
    {
      // Get existing open window or if none, make a new one:
      MinigameConfigMenu window = (MinigameConfigMenu)EditorWindow.GetWindow (typeof (MinigameConfigMenu));
      window.minSize = new Vector2(350, 40);
      window.maxSize = new Vector2(350, 40);
      window.titleContent = new GUIContent("Create New Challenge Config");
      window.Show();
    }
  }
}

