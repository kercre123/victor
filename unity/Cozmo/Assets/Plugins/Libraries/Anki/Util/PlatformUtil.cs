using UnityEngine;
using System;

public class PlatformUtil {

  public static string GetResourcesBaseFolder() {
    // On Android assets are extracted from the jar file (see StartupManager.ExtractResourceFiles) 
    // to persistentDataPath so this is the path we have to send to the engine. On iOS we can access the
    // files on streamingAssetsPath directly

#if UNITY_EDITOR
    return Application.dataPath + "/../../../resources/assets";
#elif UNITY_IOS
    return Application.streamingAssetsPath;
#elif UNITY_ANDROID
    return Application.persistentDataPath + "/cozmo";
#else
    return null;
#endif
  }

  public static string GetResourcesFolder() {
#if UNITY_EDITOR
    return GetResourcesBaseFolder();
#elif UNITY_IOS || UNITY_ANDROID
    return GetResourcesBaseFolder() + "/cozmo_resources";
#else
    return null;
#endif
  }

  public static string GetResourcesFolder(string resourcesSubfolder) {
    // On device the resources are all copied into the "assets" subfolder.
#if UNITY_EDITOR
    return GetResourcesFolder() + "/" + resourcesSubfolder;
#elif UNITY_IOS
    return GetResourcesFolder() + "/assets/" + resourcesSubfolder;
#elif UNITY_ANDROID
    return GetResourcesFolder() + "/assets/" + resourcesSubfolder;
#else
    return null;
#endif
  }
}
