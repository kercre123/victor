using UnityEngine;
using UnityEditor;
using UnityEditor.Callbacks;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System;

namespace Anki {
  namespace Build {

    public class Builder {

      public const string _kProjectName = "Cozmo";
      private const string _kBuildOuputFolder = "../../build/";

#if UNITY_EDITOR
      private static string[] _kFileExclusions = {
        ".meta",
        ".DS_Store",
        ".travis.yml",
        ".npmrc",
        ".npmignore",
        ".eslintrc.js",
        ".eslintrc",
        ".eslintignore",
        ".editorconfig",
        ".gitignore"
      };

      private static string[] _kDirectoryExclusions = {
        "AssetBundles",
        "sound",
      };

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

      [MenuItem(Build.Builder._kProjectName + "/Build/Use SD Assets")]
      public static void UseSDAssets() {
        EditorPrefs.SetString(Anki.Build.BuildKeyConstants.kAssetsPrefKey, Anki.Build.BuildKeyConstants.kSDAssetsValueKey);
      }

      [MenuItem(Build.Builder._kProjectName + "/Build/Use HD Assets")]
      public static void UseHDAssets() {
        EditorPrefs.SetString(Anki.Build.BuildKeyConstants.kAssetsPrefKey, Anki.Build.BuildKeyConstants.kHDAssetsValueKey);
      }

      [MenuItem(Build.Builder._kProjectName + "/Build/Use UHD Assets")]
      public static void UseUHDAssets() {
        EditorPrefs.SetString(Anki.Build.BuildKeyConstants.kAssetsPrefKey, Anki.Build.BuildKeyConstants.kUHDAssetsValueKey);
      }

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

      private static List<string> GetDirectoryAndFileNamesRecursive(string currentDir) {
        string[] allFiles = Directory.GetFiles(currentDir, "*", SearchOption.AllDirectories);
        List<string> fileList = new List<string>(Array.FindAll(allFiles, file => {
          return string.IsNullOrEmpty(Array.Find(_kFileExclusions, excl => file.Contains(excl)));
        }));
        fileList.Sort();
        return fileList;
      }

      public static byte[] GetDirectoryMD5Hash(string dirName) {
        using (MD5 md5 = MD5.Create()) {
          List<byte[]> hashes = new List<byte[]>();
          List<string> filenames = GetDirectoryAndFileNamesRecursive(dirName);
          StringBuilder fileString = new StringBuilder();
          int totalHashSize = 0;

          filenames.ForEach(filename => {
            try {
              fileString.Append(filename);
              FileStream fs = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
              fs.Position = 0;
              byte[] hash = md5.ComputeHash(fs);
              totalHashSize += hash.Length;
              hashes.Add(hash);
              fs.Close();
            }
            catch (Exception e) {
              Debug.LogError("error hashing file: " + e.ToString());
            }
          });

          // combine hashes together
          byte[] filenamesBlob = Encoding.UTF8.GetBytes(fileString.ToString());
          byte[] hashBlob = new byte[filenamesBlob.Length + totalHashSize];
          filenamesBlob.CopyTo(hashBlob, 0);
          int currentIndex = filenamesBlob.Length;
          hashes.ForEach(hash => {
            hash.CopyTo(hashBlob, currentIndex);
            currentIndex += hash.Length;
          });

          // hash the big blob
          return md5.ComputeHash(hashBlob);
        }
      }

      [MenuItem(Build.Builder._kProjectName + "/Build/Copy Engine Assets to StreamingAssets")]
      public static string CopyEngineAssetsToStreamingAssets() {
        try {
          CopyEngineAssets("Assets/StreamingAssets/cozmo_resources", EditorUserBuildSettings.activeBuildTarget);
          return null;
        }
        catch (Exception e) {
          Debug.LogException(e);
          return e.ToString();
        }
      }

      public static void CopyEngineAssets(string assetFolder, BuildTarget buildTarget) {
        // Delete and create the directory to make sure we don't leave stale assets
        FileUtil.DeleteFileOrDirectory(assetFolder);
        Directory.CreateDirectory(assetFolder);

        //
        // Copy engine resources
        //
        // The 'animations' and 'CladEnumToStringMaps' directories come from 
        //   ../../EXTERNALS/cozmo-assets (SVN) 
        // while the 'CladEnumToStringMaps' and 'behaviors' directories come from 
        //   '../../resources/assets/' (Git)
        //
        FileUtil.CopyFileOrDirectoryFollowSymlinks("../../EXTERNALS/cozmo-assets", assetFolder + "/assets");
        FileUtil.DeleteFileOrDirectory(assetFolder + "/assets/.svn");
        FileUtil.CopyFileOrDirectoryFollowSymlinks("../../resources/assets/cladToFileMaps",
                                                   assetFolder + "/assets/cladToFileMaps");
        FileUtil.CopyFileOrDirectoryFollowSymlinks("../../resources/assets/RewardedActions",
                                                   assetFolder + "/assets/RewardedActions");
        FileUtil.CopyFileOrDirectoryFollowSymlinks("../../resources/config", assetFolder + "/config");

        // Copy generated platform resources
        string platform = Assets.AssetBundleManager.GetPlatformName(buildTarget);
        string generatedResources = "../../generated/" + platform + "/resources";
        FileUtil.CopyFileOrDirectoryFollowSymlinks(generatedResources + "/tts", assetFolder + "/tts");

        // Delete compressed animation files that we don't need
        string[] tarFiles = Directory.GetFiles(assetFolder + "/assets/animations", "*.tar", SearchOption.AllDirectories);
        foreach (string tf in tarFiles) {
          File.Delete(tf);
        }

        // Delete compressed PNG files that we don't need
        string[] faceTarFiles = Directory.GetFiles(assetFolder + "/assets/faceAnimations", "*.tar", SearchOption.AllDirectories);
        foreach (string ftf in faceTarFiles) {
          File.Delete(ftf);
        }

        // Copy audio banks from a specific folder depending on the platform
        string soundFolder;
        bool includeHashFile = false;
        switch (buildTarget) {
        case BuildTarget.Android:
          soundFolder = "Android";
          includeHashFile = true;
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
        FileUtil.CopyFileOrDirectoryFollowSymlinks("../../EXTERNALS/cozmosoundbanks/GeneratedSoundBanks/" + soundFolder, assetFolder + "/sound");

        if (includeHashFile) {
          Debug.Log("Starting hash...");
          var sw = System.Diagnostics.Stopwatch.StartNew();
          byte[] filesHash = GetDirectoryMD5Hash("Assets/StreamingAssets");
          StringBuilder hashString = new StringBuilder();
          Array.ForEach(filesHash, data => {
            hashString.Append(data.ToString("x2"));
          });
          sw.Stop();
          Debug.Log("Hash value is: " + hashString + ", time: " + sw.Elapsed.TotalSeconds);

          var fs = File.Create("Assets/StreamingAssets/cozmo_resources/allAssetHash.txt");
          var hashBytes = Encoding.UTF8.GetBytes(hashString.ToString());
          fs.Write(hashBytes, 0, hashBytes.Length);
          fs.Close();
        }


        Debug.Log("CopyEngineAssets Engine assets copied to " + assetFolder);
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
        int buildNumber = 1;
        bool enableDebugging = false;
        bool connectWithProfiler = false;
        string assetFolder = null;
        string buildType = null;
        string scriptEngine = null;
        int i = 0;

        while (i < argv.Length) {
          string arg = argv[i++];
          switch (arg.ToLower()) {
          case "--platform": {
              platform = argv[i++];
              break;
            }
          case "--config": {
              config = argv[i++];
              break;
            }
          case "--build-number": {
              if (!Int32.TryParse(argv[i++], out buildNumber)) {
                buildNumber = 1;
              }
              PlayerSettings.Android.bundleVersionCode = buildNumber;
              break;
            }
          case "--build-path": {
              outputFolder = argv[i++];
              break;
            }
          case "--debug": {
              enableDebugging = true;
              break;
            }
          case "--sdk": {
              sdk = argv[i++];
              break;
            }
          case "--profile": {
              connectWithProfiler = true;
              break;
            }
          case "--asset-path": {
              assetFolder = argv[i++];
              break;
            }
          case "--build-type": {
              buildType = argv[i++];
              break;
            }
          case "--script-engine": {
              scriptEngine = argv[i++];
              break;
            }
          default:
            break;
          }
        }

        Debug.Log(
          String.Format("platform: {0} | config: {1} | buildPath: {2} | enabledDebugging: {3} | assetPath: {4} | buildType: {5} |",
                        platform, config, outputFolder, enableDebugging, assetFolder, buildType));

        iOSSdkVersion saveIOSSDKVersion = PlayerSettings.iOS.sdkVersion;
        ScriptCallOptimizationLevel saveIOSScriptLevel = PlayerSettings.iOS.scriptCallOptimization;

        BuildTarget buildTarget = BuildTarget.StandaloneOSXIntel64;
        BuildTargetGroup buildTargetGroup = BuildTargetGroup.Standalone;
        switch (platform.ToLower()) {
        case "android": {
            buildTarget = BuildTarget.Android;
            buildTargetGroup = BuildTargetGroup.Android;
            break;
          }
        case "mac":
        case "osx": {
            buildTarget = BuildTarget.StandaloneOSXIntel64;
            buildTargetGroup = BuildTargetGroup.Standalone;
            break;
          }
        case "ios": {
            buildTarget = BuildTarget.iOS;
            buildTargetGroup = BuildTargetGroup.iOS;
            if (sdk == "iphoneos") {
              PlayerSettings.iOS.sdkVersion = iOSSdkVersion.DeviceSDK;
            }
            else if (sdk == "iphonesimulator") {
              PlayerSettings.iOS.sdkVersion = iOSSdkVersion.SimulatorSDK;
            }
            break;
          }
        }

        if (buildType == null) {
          buildType = "PlayerAndAssets";
        }

        // save script defines
        string savePoundDefines = PlayerSettings.GetScriptingDefineSymbolsForGroup(buildTargetGroup);

        EditorUserBuildSettings.SwitchActiveBuildTarget(buildTargetGroup, buildTarget);

        bool isDebugBuild = config.ToLower() == "debug";
        BuildOptions buildOptions = GetBuildOptions(buildTarget, isDebugBuild, enableDebugging, connectWithProfiler);

        ScriptingImplementation saveScriptingImplementation = PlayerSettings.GetScriptingBackend(buildTargetGroup);
        ConfigurePlayerSettings(buildTargetGroup, config, scriptEngine);

        // Stop playing
        EditorApplication.isPlaying = false;

        // Force-Reimport Assets
        // when build script invokes this Unity freaks out with a Reload Assembly error, which
        // may not be an actual error http://answers.unity3d.com/questions/63184/error-using-importassetoptionsforcesynchronousimpo.html
        AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);

        if (!Directory.Exists(outputFolder)) {
          Directory.CreateDirectory(outputFolder);
        }
        string result;

        // Build and copy asset bundles. If there is an error building bundles, then abort the build
        result = BuildAssetBundlesInternal(buildTarget);
        if (!string.IsNullOrEmpty(result)) {
          PlayerSettings.iOS.sdkVersion = saveIOSSDKVersion;
          PlayerSettings.iOS.scriptCallOptimization = saveIOSScriptLevel;
          PlayerSettings.SetScriptingDefineSymbolsForGroup(buildTargetGroup, savePoundDefines);
          PlayerSettings.SetScriptingBackend(buildTargetGroup, saveScriptingImplementation);
          return result;
        }

        // copy assets
        if (assetFolder != null && buildType.ToLower() != "onlyplayer") {
          CopyEngineAssets(assetFolder, buildTarget);
          GenerateResourcesManifest();
        }

        // run build
        if (buildType.ToLower() != "onlyassets") {
          result = BuildPlayerInternal(outputFolder, buildTarget, buildOptions);
        }
        else {
          // empty string means success
          result = "";
        }

        // restore player settings
        PlayerSettings.iOS.sdkVersion = saveIOSSDKVersion;
        PlayerSettings.iOS.scriptCallOptimization = saveIOSScriptLevel;
        PlayerSettings.SetScriptingDefineSymbolsForGroup(buildTargetGroup, savePoundDefines);
        PlayerSettings.SetScriptingBackend(buildTargetGroup, saveScriptingImplementation);

        return result;
      }

      public static string BuildAssetBundlesInternal(BuildTarget buildTarget) {
        // Rebuild sprite atlases
        UnityEditor.Sprites.Packer.RebuildAtlasCacheIfNeeded(buildTarget, true, UnityEditor.Sprites.Packer.Execution.ForceRegroup);

        string outputPath = Path.Combine(Assets.AssetBundleManager.kAssetBundlesFolder, Assets.AssetBundleManager.GetPlatformName(buildTarget));

        if (!Directory.Exists(outputPath)) {
          Directory.CreateDirectory(outputPath);
        }

        AssetBundleManifest manifest = BuildPipeline.BuildAssetBundles(outputPath, BuildAssetBundleOptions.ChunkBasedCompression, buildTarget);
        if (manifest == null) {
          return "Error building asset bundles. See the Unity log for more information";
        }

        // Copy the asset bundles to the target folder
        if (CopyAssetBundlesTo(Path.Combine(Application.streamingAssetsPath, Assets.AssetBundleManager.kAssetBundlesFolder), buildTarget)) {
          AssetDatabase.Refresh();
        }

        return null;
      }

      private static string BuildPlayerInternal(string outputFolder, BuildTarget buildTarget, BuildOptions buildOptions) {
        if (outputFolder == null)
          return "No output folder specified for the build";

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
      private static void ConfigurePlayerSettings(BuildTargetGroup buildTargetGroup, string config, string scriptEngine) {

        // unity keeps this list from last execution. so we will not build on it. we will instead start fresh each time
        //string poundDefines = PlayerSettings.GetScriptingDefineSymbolsForGroup(buildTargetGroup);
        string poundDefines = "";
        // This needs to be true for all configurations
        PlayerSettings.iOS.scriptCallOptimization = ScriptCallOptimizationLevel.SlowAndSafe;

        switch (config.ToLower()) {
        case "debug": {
            poundDefines += "ENABLE_DEBUG_PANEL;ANKI_DEVELOPER_CODE;ANKI_DEV_CHEATS;DEBUG";
          }
          break;
        case "shipping": {
            poundDefines += "SHIPPING;NDEBUG";
          }
          break;
        case "googleplay": {
            PlayerSettings.Android.useAPKExpansionFiles = true;
            poundDefines += "SHIPPING;NDEBUG";
          }
          break;
        case "profile": {
            poundDefines += "PROFILE;NDEBUG";
          }
          break;
        case "release": {
            // TODO: BRC - Remove me after Founder Demo
            // Disable FastNoExceptions mode until we know what is causing the exception
            // in the DOTween library.
            poundDefines += "ENABLE_DEBUG_PANEL;ANKI_DEVELOPER_CODE;ANKI_DEV_CHEATS;RELEASE;NDEBUG";
          }
          break;
        }

        PlayerSettings.SetScriptingDefineSymbolsForGroup(buildTargetGroup, poundDefines);

        if (scriptEngine != null) {
          ScriptingImplementation si = (scriptEngine == "mono2x" ? ScriptingImplementation.Mono2x : ScriptingImplementation.IL2CPP);
          PlayerSettings.SetScriptingBackend(buildTargetGroup, si);
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
      [MenuItem(Build.Builder._kProjectName + "/Build/Generate Resources Manifest")]
      private static void GenerateResourcesManifest() {
        string resourcesDirectory = "Assets/StreamingAssets";
        int substringIndex = resourcesDirectory.Length + 1;
        List<string> all = new List<string>();

        AddFilesToResourcesManifestList(resourcesDirectory, substringIndex, all);

        string[] directories = Directory.GetDirectories(resourcesDirectory, "*", SearchOption.AllDirectories);
        foreach (string d in directories) {
          // Only add the folder if it is not in the list of exclusions
          if (string.IsNullOrEmpty(Array.Find(_kDirectoryExclusions, (string exclusion) => {
            return d.Contains(exclusion);
          }))) {
            // Add a line with just the folder first to create it at runtime
            all.Add(d.Substring(substringIndex));

            // Now add a line for every valid file in the folder
            AddFilesToResourcesManifestList(d, substringIndex, all);
          }
        }

        File.WriteAllLines(resourcesDirectory + "/resources.txt", all.ToArray());
      }

      private static void AddFilesToResourcesManifestList(string directory, int substringIndex, List<string> fileList) {
        string[] files = Directory.GetFiles(directory, "*", SearchOption.TopDirectoryOnly);
        foreach (string f in files) {
          // Only add the file if it is not in the list of exclusions
          if (string.IsNullOrEmpty(Array.Find(_kFileExclusions, (string exclusion) => {
            return f.Contains(exclusion);
          }))) {
            fileList.Add(f.Substring(substringIndex));
          }
        }
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
        case BuildTarget.iOS: {
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
