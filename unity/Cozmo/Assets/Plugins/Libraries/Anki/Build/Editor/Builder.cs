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

      public const string _kProjectName = "Cozmo";
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
        
      [MenuItem(Build.Builder._kProjectName + "/Build/Find Duplicate Assets in Bundles")]
      public static void FindDuplicateAssetsInBundles() {
        BuildTarget buildTarget = EditorUserBuildSettings.activeBuildTarget;
        string platformName = Assets.AssetBundleManager.GetPlatformName(buildTarget);
        string assetBundleRoot = Path.Combine(Assets.AssetBundleManager.kAssetBundlesFolder, platformName);

        if (Directory.Exists(assetBundleRoot)) {
          // Foreach manifest file in AssetBundles folder
          Dictionary<string, List<string>> assetToBundle = new Dictionary<string, List<string>>();
          string[] manifestFileNames = Directory.GetFiles(assetBundleRoot, "*.manifest", SearchOption.AllDirectories);
          string[] splitFileName;
          StreamReader file;
          string line;
          foreach (string fileName in manifestFileNames) {
            // Only check if not the main manifest file and if any variants, an hd variant 
            splitFileName = fileName.Split('.');
            if (splitFileName[0] != platformName && (splitFileName.Length <= 2 || splitFileName[1] == "hd")) {
              Debug.Log("Searching... " + fileName);
              file = new StreamReader(fileName);
              try {
                // Skip to assets section
                while ((line = file.ReadLine()) != null) {
                  if (line.StartsWith("Assets:")) {
                    break;
                  }
                }

                // Foreach asset
                while ((line = file.ReadLine()) != null && !line.StartsWith("Dependencies:")) {
                  // Check if asset is already in dictionary
                  if (!assetToBundle.ContainsKey(line)) {
                    List<string> assetBundles = new List<string>();
                    assetBundles.Add(fileName);
                    assetToBundle.Add(line, assetBundles);
                  }
                  else {
                    assetToBundle[line].Add(fileName);
                  }
                }
              }
              catch (Exception e) {
                Debug.LogError("Builder.FindDuplicateAssetsInBundles: Error reading file: " + fileName + " error: " + e.Message);
              }
            }
            else {
              Debug.Log("Skipping... " + fileName);
            }
          }

          // Iterate over dictionary to find duplicates
          bool foundDuplicates = false;
          System.Text.StringBuilder sb = new System.Text.StringBuilder("ERROR: Duplicate Assets:");
          foreach (KeyValuePair<string, List<string>> kvp in assetToBundle) {
            // If count > 1 append error with offending asset and offending asset bundles
            if (kvp.Value.Count > 1) {
              sb.AppendLine("-- " + kvp.Key);
              foreach (string assetBundle in kvp.Value) {
                sb.AppendLine("---- " + assetBundle);
              }
              foundDuplicates = true;
            }
          }

          // Print error / log depending on success
          if (foundDuplicates) {
            Debug.LogError(sb.ToString());
          }
          else {
            Debug.Log("SUCCESS! No duplicate assets found!");
          }
        }
        else {
          Debug.LogError("Builder.FindDuplicateAssetsInBundles: Could not find <projectroot>/AssetBundles folder for platform: " + platformName);
        }
      }

      [MenuItem(Build.Builder._kProjectName + "/Build/Copy Engine Assets to StreamingAssets")]
      public static void CopyEngineAssetsToStreamingAssets() {

        // Delete and create the directory to make sure we don't leave stale assets
        FileUtil.DeleteFileOrDirectory("Assets/StreamingAssets/cozmo_resources");
        Directory.CreateDirectory("Assets/StreamingAssets/cozmo_resources");

        // Copy engine resources
        FileUtil.CopyFileOrDirectoryFollowSymlinks("../../lib/anki/products-cozmo-assets", "Assets/StreamingAssets/cozmo_resources/assets");
        FileUtil.CopyFileOrDirectoryFollowSymlinks("../../resources/config", "Assets/StreamingAssets/cozmo_resources/config");
        FileUtil.CopyFileOrDirectoryFollowSymlinks("../../generated/resources/pocketsphinx", "Assets/StreamingAssets/cozmo_resources/pocketsphinx");

        // Delete compressed animation files that we don't need
        string[] tarFiles = Directory.GetFiles("Assets/StreamingAssets/cozmo_resources/assets/animations", "*.tar", SearchOption.AllDirectories);
        foreach (string tf in tarFiles) {
          File.Delete(tf);
        }

        // Copy audio banks from a specific folder depending on the platform
        string soundFolder;
        switch (EditorUserBuildSettings.activeBuildTarget) {
        case BuildTarget.Android: 
          soundFolder = "Android";
          break;

        case BuildTarget.iOS:
          soundFolder = "iOS";
          break;

        case BuildTarget.StandaloneOSXIntel:
        case BuildTarget.StandaloneOSXIntel64:
        case BuildTarget.StandaloneOSXUniversal:
          soundFolder = "Mac";
          break;

        default:
          throw new NotImplementedException();
        }
        FileUtil.CopyFileOrDirectoryFollowSymlinks("../../EXTERNALS/cozmosoundbanks/GeneratedSoundBanks/" + soundFolder, "Assets/StreamingAssets/cozmo_resources/sound");

        Debug.Log("Engine assets copied to StreamingAssets");
      }

      [MenuItem(Build.Builder._kProjectName + "/Build/Build Asset Bundles %#a")]
      public static void BuildAssetBundles() {
        BuildAssetBundlesInternal(EditorUserBuildSettings.activeBuildTarget);
      }

      [MenuItem(Build.Builder._kProjectName + "/Build/Build Asset Bundles And Player")]
      public static void BuildPlayer() {
        BuildTarget buildTarget = EditorUserBuildSettings.activeBuildTarget;
        if (buildTarget == BuildTarget.StandaloneOSXIntel) {
          Debug.LogError("Builder.BuildPlayer: Standalone build not supported, aborting");
          return;
        }

        string outputFolder = GetOutputFolder(buildTarget);
        BuildOptions options = GetBuildOptions(buildTarget, Debug.isDebugBuild, EditorUserBuildSettings.allowDebugging, EditorUserBuildSettings.connectProfiler);
        string result = BuildPlayerInternal(outputFolder, buildTarget, options);
        if (string.IsNullOrEmpty(result)) {
          result = "Builder.BuildPlayer: Successfully built for " + buildTarget + " at " + outputFolder;
        }
        Debug.Log(result);
      }
	  
      // Build the project from a set of arguments. This method is used for automated builds
      public static string BuildWithArgs(string[] argv) {

        string outputFolder = null;
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

      public static void BuildAssetBundlesInternal(BuildTarget buildTarget) {
        // Rebuild sprite atlases
        UnityEditor.Sprites.Packer.RebuildAtlasCacheIfNeeded(buildTarget, true, UnityEditor.Sprites.Packer.Execution.ForceRegroup);

        string outputPath = Path.Combine(Assets.AssetBundleManager.kAssetBundlesFolder, Assets.AssetBundleManager.GetPlatformName(buildTarget));

        if (!Directory.Exists(outputPath)) {
          Directory.CreateDirectory(outputPath);
        }

        BuildPipeline.BuildAssetBundles(outputPath, BuildAssetBundleOptions.None, buildTarget);

        // Copy the asset bundles to the target folder
        if (CopyAssetBundlesTo(Path.Combine(Application.streamingAssetsPath, Assets.AssetBundleManager.kAssetBundlesFolder), buildTarget)) {
          AssetDatabase.Refresh();
        }
      }

      private static string BuildPlayerInternal(string outputFolder, BuildTarget buildTarget, BuildOptions buildOptions) {
        if (outputFolder == null)
          return "No output folder specified for the build";
        
        if (!Directory.Exists(outputFolder)) {
          Directory.CreateDirectory(outputFolder);
        }

        // Build and copy asset bundles
        BuildAssetBundlesInternal(buildTarget);

        // Copy engine assets
        CopyEngineAssetsToStreamingAssets();

        // Generate the manifest
        GenerateResourcesManifest();

        // Refresh the asset DB so Unity picks up the new files
        AssetDatabase.Refresh();

        string[] scenes = GetScenesFromBuildSettings();
        string outputPath = outputFolder + "/" + GetOutputName(buildTarget);

        string result = BuildPipeline.BuildPlayer(scenes, outputPath, buildTarget, buildOptions);

        return result;
      }

      // Returns build options apropriate for the given build config
      private static BuildOptions GetBuildOptions(BuildTarget target, bool isDebugBuild, bool enableDebugging, bool connectWithProfiler) {
        BuildOptions options = BuildOptions.None;

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
      private static bool CopyAssetBundlesTo(string outputFolder, BuildTarget buildTarget) {
        try {
          var sourceFolder = Path.Combine(Path.Combine(System.Environment.CurrentDirectory, Assets.AssetBundleManager.kAssetBundlesFolder), Assets.AssetBundleManager.GetPlatformName(buildTarget));
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

      // Generate the resources.txt file. This file will be used on Android to extract all the files from the jar file.
      // The file has a line for folder that needs to be created and for every file that needs to be extracted. 
      // The paths in the file need to be relative to Assets/StreamingAssets.
      private static void GenerateResourcesManifest() {
        int substringIndex = "Assets/StreamingAsssets".Length;
        List<string> all = new List<string>();

        string[] directories = Directory.GetDirectories("Assets/StreamingAssets", "*", SearchOption.AllDirectories);
        foreach (string d in directories) {
          all.Add(d.Substring(substringIndex));
        }

        string[] files = Directory.GetFiles("Assets/StreamingAssets", "*.*", SearchOption.AllDirectories);
        foreach (string f in files) {
          // Filter the files we don't need to ship
          if (!f.Contains(".meta") && !f.Contains(".DS_Store")) {
            all.Add(f.Substring(substringIndex));
          }
        }

        File.WriteAllLines("Assets/StreamingAssets/resources.txt", all.ToArray());
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
      private static string GetOutputFolder(BuildTarget buildTarget) {
        switch (buildTarget) {
        case BuildTarget.Android:
        case BuildTarget.iOS:
          {
            string configuration = Debug.isDebugBuild ? "Debug" : "Release";
            string platformName = Assets.AssetBundleManager.GetPlatformName(buildTarget);
            string path = _kBuildOuputFolder + platformName + "/" + "unity-" + platformName + "/" + configuration + "-" + platformName;
            return path;
          }
        default:
          DAS.Error(null, "Target " + buildTarget + " not implemented.");
          return null;
        }
      }

      // Returns the name of the build output artifact
      private static string GetOutputName(BuildTarget buildTarget) {
        switch (buildTarget) {
        case BuildTarget.Android:
          return _kProjectName + ".apk";
        case BuildTarget.iOS:
          return ""; // The folder name is enough in iOS
        default:
          DAS.Error(null, "Target " + buildTarget + " not implemented.");
          return null;
        }
      }
    }
  }
}
