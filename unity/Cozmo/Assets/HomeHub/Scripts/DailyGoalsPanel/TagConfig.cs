using UnityEngine;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Config for setting up tags for the daily goal and rewarded action systems
/// </summary>
public class TagConfig : ScriptableObject {

  private static TagConfig _sInstance;

  public static void SetInstance(TagConfig instance) {
    _sInstance = instance;
  }

  public static TagConfig Instance {
    get {
      return _sInstance;
    }
  }

  public const string NoTag = "NONE";

  [SerializeField]
  List<string> Tags = new List<string>();

  public static bool IsValidTag(string toCheck) {
    if (toCheck == NoTag) { return false; }
    return TagConfig.GetAllTags().Contains(toCheck);
  }

  public static List<string> GetAllTags() {
    return _sInstance.Tags;
  }


#if UNITY_EDITOR

  public static string kTagConfigLocation = "Assets/AssetBundles/GameMetadata-Bundle/TagListConfig.asset";

#endif
}
