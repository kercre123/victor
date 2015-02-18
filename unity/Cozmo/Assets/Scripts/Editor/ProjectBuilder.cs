// C# build script for unity project
using UnityEditor;
using UnityEngine;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System;

public class ProjectBuilder {


  /// <summary>
  /// returns the list of all know scenes in the project
  /// </summary>
  private List<string> getScenes() {
    List<string> scenes = new List<string>();
		
    for (int sceneIndex = 0; sceneIndex < EditorBuildSettings.scenes.Length; ++sceneIndex) {
      if (EditorBuildSettings.scenes[sceneIndex].enabled) {
        scenes.Add(EditorBuildSettings.scenes[sceneIndex].path);
      }
    }

    return scenes;
  }


  /// <summary>
  /// parses string and atempts to build the project with the provided arguments
  /// </summary>
  /// <returns>
  /// build output string
  /// </returns>
  public string Build(string args) {
    string[] argv = args.Split(' ');
    return Build(argv);
  }


  /// <summary>
  /// atempts to build the project with the provided arguments
  /// </summary>
  /// <returns>
  /// build output string
  /// </returns>
  public string Build(string[] argv) {

    string buildPath = Application.dataPath + "/../Build";
    string platform = null;
    string config = null;
    string sdk = null;
    bool enableDebugging = false;

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
          buildPath = argv[i++];
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
      default:
        break;
      }
    }
    
    Debug.Log(String.Format("platform: {0} | config: {1} | buildPath: {2} | enabledDebugging: {3}", platform, config, buildPath, enableDebugging));
    iOSSdkVersion saveIOSSDKVersion = PlayerSettings.iOS.sdkVersion;

    BuildTarget buildTarget = BuildTarget.StandaloneOSXIntel64;
    switch (platform) {
    case "android":
      {
        buildTarget = BuildTarget.Android;
        break;
      }
    case "osx":
      {
        buildTarget = BuildTarget.StandaloneOSXIntel64;
        break;
      }
    case "ios":
      {
        buildTarget = BuildTarget.iPhone;
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
		
    List<string> scenes = getScenes();
    BuildOptions buildOptions = GetBuildOptions(buildTarget, config, enableDebugging);
	
    // run build
    string result = BuildPipeline.BuildPlayer(scenes.ToArray(), buildPath, buildTarget, buildOptions);

    // restore player settings
    PlayerSettings.iOS.sdkVersion = saveIOSSDKVersion;

    return result;
  }
          

  /// <summary>
  /// returns build options apropriate for the given build config
  /// </summary>
  public BuildOptions GetBuildOptions(BuildTarget target, string config, bool enableDebugging) {
    BuildOptions options = BuildOptions.AcceptExternalModificationsToPlayer;
    if (target == BuildTarget.iPhone) {
      // Overwrite any existing xcodeproject files for iOS
      options = BuildOptions.None;
    }
    if (config == "debug") {
      options = (options | BuildOptions.AllowDebugging | BuildOptions.Development);
    }
    if (enableDebugging) {
      options = (options | BuildOptions.AllowDebugging);
    }
    return options;
  }


}