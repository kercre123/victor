using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Linq;

public class ChallengeCompiler {
  [MenuItem("Cozmo/Generate Challenge List")]
  public static void GenerateChallengeList() {
    var assets = AssetDatabase.FindAssets(" t:ChallengeData");

    var list = ScriptableObject.CreateInstance(typeof(ChallengeDataList)) as ChallengeDataList;

    list.ChallengeData = assets.Select(x => AssetDatabase.LoadAssetAtPath<ChallengeData>(AssetDatabase.GUIDToAssetPath(x))).ToArray();

    AssetDatabase.CreateAsset(list, "Assets/SharedAssets/Resources/Challenges/ChallengeList.asset");
  }
}
