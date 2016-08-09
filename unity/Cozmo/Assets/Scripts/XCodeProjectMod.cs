#if UNITY_EDITOR && UNITY_IOS
using UnityEngine;
using UnityEditor;
using UnityEditor.Callbacks;
using UnityEditor.iOS.Xcode;
using System.Collections;
using System.IO;

public class XCodeProjectMod
{
//  [PostProcessBuild]
//  public static void ChangeXcodePlist(BuildTarget buildTarget, string pathToBuiltProject)
//  {
//    if ( buildTarget == BuildTarget.iOS )
//    {
//      // Get plist
//      string plistPath = pathToBuiltProject + "/Info.plist";
//      PlistDocument plist = new PlistDocument();
//      plist.ReadFromString(File.ReadAllText(plistPath));
//      
//      // Get root
//      PlistElementDict rootDict = plist.root;
//      
//      // background modes
//      PlistElementArray bgModes = rootDict.CreateArray("UIBackgroundModes");
//      bgModes.AddString("fetch");
//      bgModes.AddString("remote-notification");
//      
//      // requires wifi
//      rootDict.SetBoolean("UIRequiresPersistentWiFi", true);
//      
//      // Write to file
//      File.WriteAllText(plistPath, plist.WriteToString());
//    }
//  }
}
#endif
