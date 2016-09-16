using UnityEngine;
using UnityEditor;

/// <summary>
/// Scene auto loader.
/// </summary>
/// <description>
/// This class adds a File > Scene Autoload menu containing options to select
/// a "master scene" enable it to be auto-loaded when the user presses play
/// in the editor. When enabled, the selected scene will be loaded on play,
/// then the original scene will be reloaded on stop.
///
/// Based on an idea on this thread:
/// http://forum.unity3d.com/threads/157502-Executing-first-scene-in-build-settings-when-pressing-play-button-in-editor
/// </description>
[InitializeOnLoad]
static class SceneAutoloader {
  // Static constructor binds a playmode-changed callback.
  // [InitializeOnLoad] above makes sure this gets executed.
  static SceneAutoloader() {
    EditorApplication.playmodeStateChanged += OnPlayModeChanged;
  }

  // Menu items to select the "master" scene and control whether or not to load it.
  [MenuItem("File/Scene Autoload/Select Master Scene...")]
  private static void SelectMasterScene() {
    string masterScene = EditorUtility.OpenFilePanel("Select Master Scene", Application.dataPath, "unity");
    if (!string.IsNullOrEmpty(masterScene)) {
      MasterScene = masterScene;
      LoadMasterOnPlay = true;
    }
  }

  [MenuItem("File/Scene Autoload/Load Master On Play", true)]
  private static bool ShowLoadMasterOnPlay() {
    return !LoadMasterOnPlay;
  }

  [MenuItem("File/Scene Autoload/Load Master On Play")]
  private static void EnableLoadMasterOnPlay() {
    LoadMasterOnPlay = true;
  }

  [MenuItem("File/Scene Autoload/Don't Load Master On Play", true)]
  private static bool ShowDontLoadMasterOnPlay() {
    return LoadMasterOnPlay;
  }

  [MenuItem("File/Scene Autoload/Don't Load Master On Play")]
  private static void DisableLoadMasterOnPlay() {
    LoadMasterOnPlay = false;
  }

  // Play mode change callback handles the scene load/reload.
  private static void OnPlayModeChanged() {
    if (!LoadMasterOnPlay) {
      return;
    }

    if (!EditorApplication.isPlaying && EditorApplication.isPlayingOrWillChangePlaymode) {
      // User pressed play -- autoload master scene.
      PreviousScene = UnityEditor.SceneManagement.EditorSceneManager.GetActiveScene().path;
      if (UnityEditor.SceneManagement.EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo()) {
        if (!UnityEditor.SceneManagement.EditorSceneManager.OpenScene(MasterScene).IsValid()) {
          Debug.LogError(string.Format("error: scene not found: {0}", MasterScene));
          EditorApplication.isPlaying = false;
        }
      }
      else {
        // User cancelled the save operation -- cancel play as well.
        EditorApplication.isPlaying = false;
      }
    }
    if (!EditorApplication.isPlaying && !EditorApplication.isPlayingOrWillChangePlaymode) {
      // User pressed stop -- reload previous scene.
      if (!string.IsNullOrEmpty(PreviousScene)) {
        if (!UnityEditor.SceneManagement.EditorSceneManager.OpenScene(PreviousScene).IsValid()) {
          Debug.LogError(string.Format("error: scene not found: {0}", PreviousScene));
        }
      }
      else {
        UnityEditor.SceneManagement.EditorSceneManager.NewScene(UnityEditor.SceneManagement.NewSceneSetup.DefaultGameObjects);
      }
    }
  }

  // Properties are remembered as editor preferences.
  private const string cEditorPrefLoadMasterOnPlay = "SceneAutoLoader.LoadMasterOnPlay";
  private const string cEditorPrefMasterScene = "SceneAutoLoader.MasterScene";
  private const string cEditorPrefPreviousScene = "SceneAutoLoader.PreviousScene";

  private static bool LoadMasterOnPlay {
    get { return EditorPrefs.GetBool(cEditorPrefLoadMasterOnPlay, false); }
    set { EditorPrefs.SetBool(cEditorPrefLoadMasterOnPlay, value); }
  }

  private static string MasterScene {
    get { return EditorPrefs.GetString(cEditorPrefMasterScene, "Master.unity"); }
    set { EditorPrefs.SetString(cEditorPrefMasterScene, value); }
  }

  private static string PreviousScene {
    get { return EditorPrefs.GetString(cEditorPrefPreviousScene, UnityEditor.SceneManagement.EditorSceneManager.GetActiveScene().name); }
    set { EditorPrefs.SetString(cEditorPrefPreviousScene, value); }
  }
}
