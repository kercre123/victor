using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;

public static class SceneOpener {
  [MenuItem("Cozmo/Scenes/Open Bootstrap Scene %#1")]
  public static void OpenBootstrapScene() {
    EditorSceneManager.OpenScene("Assets/Scenes/Bootstrap.unity", OpenSceneMode.Single);
  }

  [MenuItem("Cozmo/Scenes/Open HomeHub Scene %#2")]
  public static void OpenHomeHubScene() {
    EditorSceneManager.OpenScene("Assets/AssetBundles/MainScene-Bundle/HomeHub.unity", OpenSceneMode.Single);
  }

  [MenuItem("Cozmo/Scenes/Open UI Sandbox Scene %#5")]
  public static void OpenUISandboxScene() {
    EditorSceneManager.OpenScene("Assets/Scenes/UISandbox.unity", OpenSceneMode.Single);
  }
}
