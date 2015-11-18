using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Linq;

public class ChallengeCompiler {
  [MenuItem("Cozmo/Generate Challenge List")]
  public static void GenerateChallengeList() {
    var assets = AssetDatabase.FindAssets(" t:ChallengeData");

    var list = ScriptableObject.CreateInstance(typeof(ChallengeDataList)) as ChallengeDataList;

    list.ChallengeDataNames = assets.Select(x => AssetDatabase.GUIDToAssetPath(x)).Where(x => x.Contains("Resources/")).Select(x => x.Substring(x.LastIndexOf("Resources/") + "Resources/".Length).Replace(".asset","")).ToArray();

    AssetDatabase.CreateAsset(list, "Assets/SharedAssets/Resources/Challenges/ChallengeList.asset");
  }
}
