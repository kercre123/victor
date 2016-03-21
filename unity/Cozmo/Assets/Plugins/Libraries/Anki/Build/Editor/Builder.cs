using UnityEngine;
using UnityEditor;
using UnityEditor.Callbacks;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;

namespace Anki {
  namespace Build {
    
    public class Builder {

      private const string _kProjectName = "Cozmo";
      private const string _kBuildOuputFolder = "Build";
      private const string _kSimulationMode = _kProjectName + "/Build/Asset Bundle Simulation Mode";

      [MenuItem(_kSimulationMode)]
      public static void ToggleSimulationMode() {
        Assets.AssetBundleManager.SimulateAssetBundleInEditor = !Assets.AssetBundleManager.SimulateAssetBundleInEditor;
      }

      [MenuItem(_kSimulationMode, true)]
      public static bool ToggleSimulationModeValidate() {
        Menu.SetChecked(_kSimulationMode, Assets.AssetBundleManager.SimulateAssetBundleInEditor);
        return true;
      }

      [MenuItem(Build.Builder._kProjectName + "/Build/Build Asset Bundles")]
      public static void BuildAssetBundles() {
        string outputPath = Path.Combine(Assets.AssetBundleManager.kAssetBundlesFolder, Assets.AssetBundleManager.GetPlatformName());
                
        if (!Directory.Exists(outputPath)) {
          Directory.CreateDirectory(outputPath);
        }
  	
        BuildPipeline.BuildAssetBundles(outputPath, BuildAssetBundleOptions.None, EditorUserBuildSettings.activeBuildTarget);

        // Copy the asset bundles to the target folder
        if (CopyAssetBundlesTo(Path.Combine(Application.streamingAssetsPath, Assets.AssetBundleManager.kAssetBundlesFolder))) {
          AssetDatabase.Refresh();
        }
      }

      [MenuItem(Build.Builder._kProjectName + "/Build/Build Asset Bundles And Player")]
      public static void BuildPlayer() {
        string outputFolder = GetOutputFolder();
        if (outputFolder == null)
          return;
  			
        if (!Directory.Exists(outputFolder)) {
          Directory.CreateDirectory(outputFolder);
        }

        // Build and copy asset bundles
        BuildAssetBundles();

        // TODO: Determine which scene should be here
        string[] sceneList = new string[0];
        string outputPath = outputFolder + "/" + GetOutputName();

        BuildOptions options = EditorUserBuildSettings.development ? BuildOptions.Development : BuildOptions.None;
        BuildPipeline.BuildPlayer(sceneList, outputPath, EditorUserBuildSettings.activeBuildTarget, options);
      }

      static bool CopyAssetBundlesTo(string outputFolder) {
        try {
          var sourceFolder = Path.Combine(Path.Combine(System.Environment.CurrentDirectory, Assets.AssetBundleManager.kAssetBundlesFolder), Assets.AssetBundleManager.GetPlatformName());
          if (!System.IO.Directory.Exists(sourceFolder)) {
            Debug.LogError("No asset bundles folder for the current platform. Build bundles first");
          }

          // CopyFileOrDirectory requires the target to not exist so delete it first
          FileUtil.DeleteFileOrDirectory(outputFolder);
          FileUtil.CopyFileOrDirectory(sourceFolder, outputFolder);
        }
        catch(IOException e) {
          Debug.LogError(e.Message);
          return false;
        }

        return true;
      }

      static string[] GetLevelsFromBuildSettings() {
        List<string> levels = new List<string>();
        for (int i = 0; i < EditorBuildSettings.scenes.Length; ++i) {
          if (EditorBuildSettings.scenes[i].enabled)
            levels.Add(EditorBuildSettings.scenes[i].path);
        }
  	
        return levels.ToArray();
      }

      private static string GetOutputFolder() {
        switch (EditorUserBuildSettings.activeBuildTarget) {
        case BuildTarget.Android:
        case BuildTarget.iOS:
          return Path.Combine(_kBuildOuputFolder, Assets.AssetBundleManager.GetPlatformName());
        default:
          Debug.LogError("Target " + EditorUserBuildSettings.activeBuildTarget + " not implemented.");
          return null;
        }
      }

      private static string GetOutputName() {
        switch (EditorUserBuildSettings.activeBuildTarget) {
        case BuildTarget.Android:
          return _kProjectName + ".apk";
        case BuildTarget.iOS:
          return ""; // The folder name is enough in iOS
        default:
          Debug.LogError("Target " + EditorUserBuildSettings.activeBuildTarget + " not implemented.");
          return null;
        }
      }
    }
  }
}
    