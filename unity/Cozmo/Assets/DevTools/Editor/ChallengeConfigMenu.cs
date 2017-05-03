using Cozmo.Challenge;
using System;
using System.Reflection;
using System.Linq;
using UnityEditor;
using UnityEngine;

namespace AssemblyCSharpEditor {
  public class ChallengeConfigMenu : EditorWindow {


    private static string[] _typeNames;

    private static Type[] _types;

    private static int[] _indices;

    private int _selectedIndex = 0;

    static ChallengeConfigMenu() {
      var types = Assembly.GetAssembly(typeof(ChallengeConfigBase)).GetTypes().Where(t => typeof(ChallengeConfigBase).IsAssignableFrom(t) && t != typeof(ChallengeConfigBase));
      _types = types.Concat(new System.Type[1] { null }).ToArray();
      _typeNames = _types.Select(x => x != null ? x.Name : "None").ToArray();

      _indices = new int[_typeNames.Length];
      for (int i = 0; i < _indices.Length; i++) {
        _indices[i] = i;
      }
    }

    void OnGUI() {
      _selectedIndex = EditorGUILayout.IntPopup("Type", _selectedIndex, _typeNames, _indices);


      var type = _types[_selectedIndex];

      if (GUILayout.Button("Create")) {
        var path = EditorUtility.SaveFilePanel("New Config", "Assets/SharedAssets/Resources/Challenges", (type != null ? type.Name.Replace("Config", "") : "") + "Challenge", "asset");

        if (!string.IsNullOrEmpty(path)) {
          var challengeDataInstance = ScriptableObject.CreateInstance(typeof(ChallengeData)) as ChallengeData;
          var configInstance = type != null ? ScriptableObject.CreateInstance(type) as ChallengeConfigBase : null;

          if (path.StartsWith(Application.dataPath) && path.Contains("Resources")) {
            path = "Assets" + path.Substring(Application.dataPath.Length);
            string path2 = path.Replace(".asset", "_Config.asset");

            if (type != null) {
              AssetDatabase.CreateAsset(configInstance, path2);
            }

            challengeDataInstance.ChallengeConfig = configInstance;

            AssetDatabase.CreateAsset(challengeDataInstance, path);

            UnityEditor.Selection.activeObject = challengeDataInstance;
          }
          else {
            DAS.Error(this, "Config must be in a Resources Folder!");
          }
        }
      }
    }

    [MenuItem("Cozmo/Challenges/Create Challenge Config", false, 10)]
    public static void OpenChallengeConfigMenu() {
      // Get existing open window or if none, make a new one:
      ChallengeConfigMenu window = (ChallengeConfigMenu)EditorWindow.GetWindow(typeof(ChallengeConfigMenu));
      window.minSize = new Vector2(350, 40);
      window.maxSize = new Vector2(350, 40);
      window.titleContent = new GUIContent("Create New Challenge Config");
      window.Show();
    }
  }
}

