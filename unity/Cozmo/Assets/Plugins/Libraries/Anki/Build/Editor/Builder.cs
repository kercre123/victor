using UnityEngine;
using UnityEditor;
using UnityEditor.Callbacks;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System;

namespace Anki {
  namespace Build {
    
    public class Builder {

      private const string _kProjectName = "Cozmo";
      private const string _kBuildOuputFolder = "../../build/";

      #if UNITY_EDITOR
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
      #endif

      [MenuItem(Build.Builder._kProjectName + "/Build/Build Asset Bundles")]
      public static void BuildAssetBundles() {
        // Rebuild sprite atlases
        UnityEditor.Sprites.Packer.RebuildAtlasCacheIfNeeded(EditorUserBuildSettings.activeBuildTarget);

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
  			
        BuildOptions options = GetBuildOptions(EditorUserBuildSettings.activeBuildTarget, Debug.isDebugBuild, EditorUserBuildSettings.allowDebugging, EditorUserBuildSettings.connectProfiler);
        BuildPlayerInternal(outputFolder, EditorUserBuildSettings.activeBuildTarget, options);
      }

      // Build the project from a set of arguments. This method is used for automated builds
      public static string BuildWithArgs(string[] argv) {

        string outputFolder = GetOutputFolder();
        string platform = null;
        string config = null;
        string sdk = null;
        bool enableDebugging = false;
        bool connectWithProfiler = false;

        int i = 0;
        while (i < argv.Length) {
          string arg = argv[i++];
          switch (arg) {
          case "--platform":
            {
              platform = argv[i++];
              break;
            }
          case "--config":
            {
              config = argv[i++];
              break;
            }
          case "--build-path":
            {
              outputFolder = argv[i++];
              break;
            }
          case "--debug":
            {
              enableDebugging = true;
              break;
            }
          case "--sdk":
            {
              sdk = argv[i++];
              break;
            }
          case "--profile":
            {
              connectWithProfiler = true;
              break;
            }
          default:
            break;
          }
        }

        DAS.Debug(null, String.Format("platform: {0} | config: {1} | buildPath: {2} | enabledDebugging: {3}", platform, config, outputFolder, enableDebugging));
        iOSSdkVersion saveIOSSDKVersion = PlayerSettings.iOS.sdkVersion;
        ScriptCallOptimizationLevel saveIOSScriptLevel = PlayerSettings.iOS.scriptCallOptimization;

        BuildTarget buildTarget = BuildTarget.StandaloneOSXIntel64;
        switch (platform) {
        case "android":
          {
            buildTarget = BuildTarget.Android;
            break;
          }
        case "mac":
        case "osx":
          {
            buildTarget = BuildTarget.StandaloneOSXIntel64;
            break;
          }
        case "ios":
          {
            buildTarget = BuildTarget.iOS;
            if (sdk == "iphoneos") {
              PlayerSettings.iOS.sdkVersion = iOSSdkVersion.DeviceSDK;
            }
            else if (sdk == "iphonesimulator") {
              PlayerSettings.iOS.sdkVersion = iOSSdkVersion.SimulatorSDK;
            }
            break;
          }
        }

        // Later on use this to switch between building for different targets
        // EditorUserBuildSettings.SwitchActiveBuildTarget(BuildTarget.Android);

        BuildOptions buildOptions = GetBuildOptions(buildTarget, config == "debug", enableDebugging, connectWithProfiler);

        ConfigurePlayerSettings(buildTarget, config);

        // Stop playing
        EditorApplication.isPlaying = false;

        // Force-Reimport Assets
        // when build script invokes this Unity freaks out with a Reload Assembly error, which
        // may not be an actual error http://answers.unity3d.com/questions/63184/error-using-importassetoptionsforcesynchronousimpo.html
        AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);

        // run build
        string result = BuildPlayerInternal(outputFolder, buildTarget, buildOptions);

        // restore player settings
        PlayerSettings.iOS.sdkVersion = saveIOSSDKVersion;
        PlayerSettings.iOS.scriptCallOptimization = saveIOSScriptLevel;

        return result;
      }

      private static string BuildPlayerInternal(string outputFolder, BuildTarget buildTarget, BuildOptions buildOptions) {
        if (outputFolder == null)
          return "No output folder specified for the build";
        
        if (!Directory.Exists(outputFolder)) {
          Directory.CreateDirectory(outputFolder);
        }

        // Build and copy asset bundles
        BuildAssetBundles();

        string[] scenes = GetScenesFromBuildSettings();
        string outputPath = outputFolder + "/" + GetOutputName();
        string result = BuildPipeline.BuildPlayer(scenes, outputPath, buildTarget, buildOptions);
        return result;
      }

      // Returns build options apropriate for the given build config
      private static BuildOptions GetBuildOptions(BuildTarget target, bool isDebugBuild, bool enableDebugging, bool connectWithProfiler) {
        BuildOptions options = BuildOptions.AcceptExternalModificationsToPlayer;
        if (target == BuildTarget.iOS) {
          // Overwrite any existing xcodeproject files for iOS
          options = BuildOptions.None;
        }
        if (isDebugBuild) {
          options |= BuildOptions.Development;
        }
        if (enableDebugging) {
          options |= BuildOptions.AllowDebugging;
        }
        if (connectWithProfiler) {
          options |= BuildOptions.ConnectWithProfiler;
        }
        return options;
      }

      // Configures player settings for a target-config pairing.
      // In an ideal world, we would never have to change the player settings.
      // However, on iOS this is necessary to get appropriate debug symbols & C# exception behavior
      // for optimal debugging.
      private static void ConfigurePlayerSettings(BuildTarget target, string config) {
        // We currently only need to set custom options for iOS
        if (target != BuildTarget.iOS) {
          return;
        }

        switch (config) {
        case "debug":
          {
            PlayerSettings.iOS.scriptCallOptimization = ScriptCallOptimizationLevel.SlowAndSafe;
          }
          break;
        case "profile":
        case "release":
          {
            // TODO: BRC - Remove me after Founder Demo
            // Disable FastNoExceptions mode until we know what is causing the exception
            // in the DOTween library.
            PlayerSettings.iOS.scriptCallOptimization = ScriptCallOptimizationLevel.SlowAndSafe;
          }
          break;
        }
      }

      // Copies the asset bundles to the target folder
      private static bool CopyAssetBundlesTo(string outputFolder) {
        try {
          var sourceFolder = Path.Combine(Path.Combine(System.Environment.CurrentDirectory, Assets.AssetBundleManager.kAssetBundlesFolder), Assets.AssetBundleManager.GetPlatformName());
          if (!System.IO.Directory.Exists(sourceFolder)) {
            DAS.Error(null, "No asset bundles folder for the current platform. Build bundles first");
          }

          // CopyFileOrDirectory requires the target to not exist so delete it first
          FileUtil.DeleteFileOrDirectory(outputFolder);
          FileUtil.CopyFileOrDirectory(sourceFolder, outputFolder);
        }
        catch (IOException e) {
          DAS.Error(null, e.Message);
          return false;
        }

        return true;
      }

      // Returns all the enabled scenes from the editor settings
      private static string[] GetScenesFromBuildSettings() {
        List<string> scenes = new List<string>();
        for (int i = 0; i < EditorBuildSettings.scenes.Length; ++i) {
          if (EditorBuildSettings.scenes[i].enabled)
            scenes.Add(EditorBuildSettings.scenes[i].path);
        }
  	
        return scenes.ToArray();
      }

      // Returns the output folder according to the active build target
      private static string GetOutputFolder() {
        switch (EditorUserBuildSettings.activeBuildTarget) {
        case BuildTarget.Android:
        case BuildTarget.iOS:
          {
            string configuration = Debug.isDebugBuild ? "Debug" : "Release";
            string platformName = Assets.AssetBundleManager.GetPlatformName();
            string path = _kBuildOuputFolder + platformName + "/" + "unity-" + platformName + "/" + configuration + "-iphoneos/";
            return path;
          }
        default:
          DAS.Error(null, "Target " + EditorUserBuildSettings.activeBuildTarget + " not implemented.");
          return null;
        }
      }

      // Returns the name of the build output artifact
      private static string GetOutputName() {
        switch (EditorUserBuildSettings.activeBuildTarget) {
        case BuildTarget.Android:
          return _kProjectName + ".apk";
        case BuildTarget.iOS:
          return ""; // The folder name is enough in iOS
        default:
          DAS.Error(null, "Target " + EditorUserBuildSettings.activeBuildTarget + " not implemented.");
          return null;
        }
      }
    }
  }
}
    