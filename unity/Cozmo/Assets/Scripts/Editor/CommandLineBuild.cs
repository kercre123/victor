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
      Debug.Log("args: " + System.Environment.CommandLine);
      foreach (string arg in argv) {
        Debug.Log("arg => " + arg);
      }
    }
    catch (UnityException ex) {
      Debug.LogException(ex);
    }

    ProjectBuilder builder = new ProjectBuilder();
    builder.Build(argv);
  }
          
}