using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Linq;

public class ChallengeCompiler {
  [MenuItem("Cozmo/Challenges/Generate HubWorld Challenge List", false, 1)]
  public static void GenerateHubWorldChallengeList() {
    var assets = AssetDatabase.FindAssets(" t:ChallengeData");

    const string path = "Assets/SharedAssets/Resources/Challenges/ChallengeList.asset";

    var list = AssetDatabase.LoadAssetAtPath<ChallengeDataList>(path);

    if (list == null) {
      list = ScriptableObject.CreateInstance(typeof(ChallengeDataList)) as ChallengeDataList;
      AssetDatabase.CreateAsset(list, path);
    }

    list.ChallengeData = assets.Select(x => AssetDatabase.LoadAssetAtPath<ChallengeData>(AssetDatabase.GUIDToAssetPath(x))).ToArray();
    EditorUtility.SetDirty(list);
    list.ChallengeData = list.ChallengeData.Where((x, index) => x.EnableHubWorldChallenge).ToArray();
    AssetDatabase.SaveAssets();

  }

  [MenuItem("Cozmo/Challenges/Generate DevWorld Challenge List", false, 5)]
  public static void GenerateDevWorldChallengeList() {
    var assets = AssetDatabase.FindAssets(" t:ChallengeData");

    const string path = "Assets/SharedAssets/Resources/Challenges/DevChallengeList.asset";

    var list = AssetDatabase.LoadAssetAtPath<ChallengeDataList>(path);

    if (list == null) {
      list = ScriptableObject.CreateInstance(typeof(ChallengeDataList)) as ChallengeDataList;
      AssetDatabase.CreateAsset(list, path);
    }

    list.ChallengeData = assets.Select(x => AssetDatabase.LoadAssetAtPath<ChallengeData>(AssetDatabase.GUIDToAssetPath(x))).ToArray();
    EditorUtility.SetDirty(list);
    list.ChallengeData = list.ChallengeData.Where((x, index) => x.EnableDevWorldChallenge).ToArray();
    AssetDatabase.SaveAssets();
  }
}
