// C# build script for unity project
using UnityEditor;
using UnityEngine;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System;

public class CommandLineBuild {

  public static void Build() {
    string[] argv = new string[0];
    try {
      argv = System.Environment.GetCommandLineArgs();
      DAS.Debug("LightCube", "args: " + System.Environment.CommandLine);
      for (int k = 0; k < argv.Length; ++k) {
        string arg = argv[k];
        DAS.Debug("LightCube", "arg => " + arg);
      }
    }
    catch (UnityException ex) {
      Debug.LogException(ex);
    }

    ProjectBuilder builder = new ProjectBuilder();
    string result = builder.Build(argv);
    
    if (!String.IsNullOrEmpty(result)) {
      // If builder.Build returned a response, then there was an error.
      // Throw an exception as an attempt to ensure that Unity exits with an error code.yep
      throw new Exception(result);
    }
  }
          
}
