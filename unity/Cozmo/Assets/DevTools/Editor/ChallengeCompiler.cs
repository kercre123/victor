using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Linq;

public class ChallengeCompiler {
  [MenuItem("Cozmo/Generate HubWorld Challenge List")]
  public static void GenerateHubWorldChallengeList() {
    var assets = AssetDatabase.FindAssets(" t:ChallengeData");

    var list = ScriptableObject.CreateInstance(typeof(ChallengeDataList)) as ChallengeDataList;

    list.ChallengeData = assets.Select(x => AssetDatabase.LoadAssetAtPath<ChallengeData>(AssetDatabase.GUIDToAssetPath(x))).ToArray();

    list.ChallengeData = list.ChallengeData.Where((x, index) => x.EnableHubWorldChallenge).ToArray();

    AssetDatabase.CreateAsset(list, "Assets/SharedAssets/Resources/Challenges/ChallengeList.asset");
  }

  [MenuItem("Cozmo/Generate DevWorld Challenge List")]
  public static void GenerateDevWorldChallengeList() {
    var assets = AssetDatabase.FindAssets(" t:ChallengeData");

    var list = ScriptableObject.CreateInstance(typeof(ChallengeDataList)) as ChallengeDataList;

    list.ChallengeData = assets.Select(x => AssetDatabase.LoadAssetAtPath<ChallengeData>(AssetDatabase.GUIDToAssetPath(x))).ToArray();

    list.ChallengeData = list.ChallengeData.Where((x, index) => x.EnableDevWorldChallenge).ToArray();

    AssetDatabase.CreateAsset(list, "Assets/SharedAssets/Resources/Challenges/DevChallengeList.asset");
  }
}
