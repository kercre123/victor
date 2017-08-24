#if UNITY_EDITOR && UNITY_IOS

using System.IO;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using UnityEngine;
using UnityEditor;
#if !UNITY_4_3 && !UNITY_4_4 && !UNITY_4_5 && !UNITY_4_6 && !UNITY_4_7 && !UNITY_4_8 && !UNITY_4_9
using UnityEditor.iOS.Xcode;
#endif

namespace UTNotifications
{
    class IOSBuildPostprocessor
    {
        [UnityEditor.Callbacks.PostProcessBuildAttribute(0)]
        public static void OnPostprocessBuild(BuildTarget target, string pathToBuiltProject)
        {
#if UNITY_4_3 || UNITY_4_4 || UNITY_4_5 || UNITY_4_6 || UNITY_4_7 || UNITY_4_8 || UNITY_4_9
            if (target == BuildTarget.iPhone)
#else
            if (target == BuildTarget.iOS)
#endif
            {
                PatchAppController(pathToBuiltProject);
                CopyRaw(pathToBuiltProject);
            }
        }

        private static void PatchAppController(string pathToBuiltProject)
        {
            string fileName = Path.Combine(pathToBuiltProject, "Classes/UnityAppController.mm");
            List<string> appControllerLines = new List<string>(File.ReadAllLines(fileName));

            PatchIncludes(appControllerLines);
            PatchDidReceiveLocalNotification(appControllerLines);
            PatchDidReceiveRemoteNotification(appControllerLines);
            PatchDidFinishLaunchingWithOptions(appControllerLines);
            UpdatePushNotificationsCapability(pathToBuiltProject);

            File.WriteAllLines(fileName, appControllerLines.ToArray());
        }

        private static void PatchIncludes(List<string> appControllerLines)
        {
#if UNITY_4_3 || UNITY_4_4 || UNITY_4_5 || UNITY_4_6 || UNITY_4_7 || UNITY_4_8 || UNITY_4_9
			const string include = "#include \"Libraries/UTNotificationsTools.h\"";
#else
			const string include = "#include \"UTNotificationsTools.h\"";
#endif
            
            foreach (string line in appControllerLines)
            {
                if (line.Contains(include))
                {
                    return;
                }
            }
            
            int position;
            for (position = appControllerLines.Count - 1; position >= 0; --position)
            {
                if (appControllerLines[position].Trim().StartsWith("#include"))
                {
                    break;
                }
            }

            appControllerLines.Insert(position + 1, include);
        }

        private static void PatchDidReceiveLocalNotification(List<string> appControllerLines)
        {
            string[] addition =
            {
                "    // UTNotifications: handle clicks on local notifications",
                "    if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive)",
                "    {",
                "        _UT_SetLocalNotificationWasClicked(notification.userInfo);",
                "    }"
            };

            PatchMethod("- (void)application:(UIApplication*)application didReceiveLocalNotification:(UILocalNotification*)notification", appControllerLines, addition);
        }

        private static void PatchDidReceiveRemoteNotification(List<string> appControllerLines)
        {
            string[] addition =
            {
                "    // UTNotifications: handle clicks on push notifications",
                "    if ([UIApplication sharedApplication].applicationState != UIApplicationStateActive)",
                "    {",
                "        _UT_SetPushNotificationWasClicked(userInfo);",
                "    }"
            };

            PatchMethod("- (void)application:(UIApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo", appControllerLines, addition);
			PatchMethod("- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo fetchCompletionHandler:(void (^)(UIBackgroundFetchResult result))handler", appControllerLines, addition);
        }

        private static void PatchDidFinishLaunchingWithOptions(List<string> appControllerLines)
        {
            string[] addition =
            {
                "#if !UNITY_TVOS",
                "    // UTNotifications: handle clicks on local notifications",
                "    if (UILocalNotification* notification = [launchOptions objectForKey:UIApplicationLaunchOptionsLocalNotificationKey])",
                "    {",
                "        _UT_SetLocalNotificationWasClicked(notification.userInfo);",
                "    }",
                "",
                "    // UTNotifications: handle clicks on push notifications",
                "    if (NSDictionary* notification = [launchOptions objectForKey:UIApplicationLaunchOptionsRemoteNotificationKey])",
                "    {",
                "        _UT_SetPushNotificationWasClicked(notification);",
                "    }",
                "#endif"
            };

            PatchMethod("- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions", appControllerLines, addition);
        }

        private static void PatchMethod(string method, List<string> appControllerLines, string[] addition)
        {
            method = method.Trim();

            int methodHeader;
            for (methodHeader = 0; methodHeader < appControllerLines.Count; ++methodHeader)
            {
                if (appControllerLines[methodHeader].Trim() == method)
                {
                    break;
                }
            }

            string autoGeneratedPrefix = "//--> This block is auto-generated. Please don't change it!";
            string autoGeneratedPostfix = "//<-- End of auto-generated block";

			if (methodHeader >= appControllerLines.Count)
			{
				Debug.LogWarning("Can't patch Objective-C code (it may be alright). Method not found: " + method);
				return;
			}

            int braceCount = 0;
            int prefixLine = -1;
            int insertionLine = -1;
            for (int i = methodHeader; i < appControllerLines.Count; ++i)
            {
                string line = appControllerLines[i].Trim();
                if (line == autoGeneratedPrefix)
                {
					prefixLine = i;
                }
				else if (line == autoGeneratedPostfix)
                {
					appControllerLines.RemoveRange(prefixLine, i - prefixLine + 1);
					if (appControllerLines.Count > prefixLine && appControllerLines[prefixLine].Trim().Length == 0)
					{
						appControllerLines.RemoveAt(prefixLine);
					}

					break;
                }

                braceCount += CountOf(line, '{');

                int closingBracesCount = CountOf(line, '}');
                if (closingBracesCount > 0)
                {
                    braceCount -= closingBracesCount;

                    if (braceCount == 0)
                    {
                        //Method doesn't contain the patch yet.
                        break;
                    }
                }

                if (braceCount == 1 && insertionLine < 0)
                {
                    insertionLine = i + 1;
                }
            }

            //Let's patch.
			if (insertionLine < 0)
			{
				Debug.LogError("Can't patch Objective-C code. Unable to find a method start: " + method);
			}

            appControllerLines.Insert(insertionLine, autoGeneratedPrefix);
            appControllerLines.Insert(insertionLine + 1, autoGeneratedPostfix);
            appControllerLines.Insert(insertionLine + 2, "");
            appControllerLines.InsertRange(insertionLine + 1, addition);
        }

        private static int CountOf(string str, char ch)
        {
            int result = 0;
            foreach (char c in str)
            {
                if (c == ch)
                {
                    ++result;
                }
            }

            return result;
        }

        private static void CopyRaw(string pathToBuiltProject)
        {
            string inDir = Path.Combine(Settings.PluginsFullPath, "iOS/Raw");
            if (Directory.Exists(inDir))
            {
                string outDir = Path.Combine(pathToBuiltProject, "Data/Raw");
                if (!Directory.Exists(outDir))
                {
                    Directory.CreateDirectory(outDir);
                }

                FileInfo[] fileInfos = new DirectoryInfo(inDir).GetFiles();
                if (fileInfos != null)
                {
                    foreach (FileInfo fileInfo in fileInfos)
                    {
                        if (!fileInfo.Name.EndsWith(".meta"))
                        {
                            File.Copy(fileInfo.FullName, Path.Combine(outDir, fileInfo.Name), true);
                        }
                    }
                }
            }
        }

        private static void UpdatePushNotificationsCapability(string pathToBuiltProject)
        {
#if !UNITY_4_3 && !UNITY_4_4 && !UNITY_4_5 && !UNITY_4_6 && !UNITY_4_7 && !UNITY_4_8 && !UNITY_4_9
            string unityTargetName = PBXProject.GetUnityTargetName();
            string xcodeProjectPath = pathToBuiltProject + "/" + unityTargetName + ".xcodeproj/project.pbxproj";
            string initialXcodeProjectText = File.ReadAllText(xcodeProjectPath);

            try
            {
                string xcodeProjectText = initialXcodeProjectText;

                // Update com.apple.Push
                Regex enabledRegex = new Regex(@"com\.apple\.Push\s*=\s*\{\s*enabled\s*=\s*1");
                if (enabledRegex.IsMatch(xcodeProjectText) != Settings.Instance.PushNotificationsEnabledIOS)
                {
                    // Needs to be enabled
                    if (Settings.Instance.PushNotificationsEnabledIOS)
                    {
                        Regex disabledRegex = new Regex(@"com\.apple\.Push\s*=\s*\{\s*enabled\s*=\s*0");
                        if (disabledRegex.IsMatch(xcodeProjectText))
                        {
                            xcodeProjectText = disabledRegex.Replace(xcodeProjectText, "com.apple.Push = {\nenabled = 1");
                        }
                        else
                        {
                            string targetGuid;
                            {
                                PBXProject pbxProject = new PBXProject();
                                pbxProject.ReadFromString(xcodeProjectText);

                                targetGuid = pbxProject.TargetGuidByName(unityTargetName);
                            }

                            Regex systemCapabilitiesRegex = new Regex(@"SystemCapabilities\s*=\s*\{");
                            if (!systemCapabilitiesRegex.IsMatch(xcodeProjectText))
                            {
                                xcodeProjectText = new Regex(@"TargetAttributes\s*=\s*\{").Replace(xcodeProjectText, string.Format(
                                    @"TargetAttributes = {{
                                    {0} = {{
                                        SystemCapabilities = {{
                                            com.apple.Push = {{
                                                enabled = 1;
                                            }};
                                        }};
                                    }};", targetGuid));
                            }
                            else
                            {
                                xcodeProjectText = new Regex(@"SystemCapabilities\s*=\s*\{").Replace(xcodeProjectText,
                                    @"SystemCapabilities = {
                                        com.apple.Push = {
                                            enabled = 1;
                                        };");
                            }
                        }
                    }
                    else // Needs to be disabled
                    {
                        xcodeProjectText = enabledRegex.Replace(xcodeProjectText, "com.apple.Push = {\nenabled = 0");
                    }

                    File.WriteAllText(xcodeProjectPath, xcodeProjectText);
                }

                // Update .entitlements
                if (Settings.Instance.PushNotificationsEnabledIOS)
                {
                    Regex codeSignEntitlements = new Regex(@"CODE_SIGN_ENTITLEMENTS\s*=\s*\""(.+)\""");
                    Match match = codeSignEntitlements.Match(xcodeProjectText);
                    string entitlementsFileName;
                    bool updatePBXProject = false;
                    if (match.Success)
                    {
                        entitlementsFileName = match.Groups[1].Value;
                    }
                    else
                    {
                        string productName = Regex.Match(xcodeProjectText, @"PRODUCT_NAME\s*=\s*(.+);").Groups[1].Value;
                        entitlementsFileName = string.Format("{0}/{1}.entitlements", unityTargetName, productName);
                        updatePBXProject = true;
                    }

                    string entitlementsFullFileName = pathToBuiltProject + "/" + entitlementsFileName;
                    if (!File.Exists(entitlementsFullFileName))
                    {
                        const string entitlementsFileTemplate =
@"<?xml version=""1.0"" encoding=""UTF-8""?>
<!DOCTYPE plist PUBLIC ""-//Apple//DTD PLIST 1.0//EN"" ""http://www.apple.com/DTDs/PropertyList-1.0.dtd"">
<plist version=""1.0"">
<dict>
</dict>
</plist>
";
                        File.WriteAllText(entitlementsFullFileName, entitlementsFileTemplate);
                    }

                    string entitlementsFileText = File.ReadAllText(entitlementsFullFileName);
                    if (entitlementsFileName.IndexOf("aps-environment") < 0)
                    {
                        string newDictEntry = "<dict>\n    <key>aps-environment</key>\n    <string>development</string>";
                        Regex emptyDictRegex = new Regex(@"\<\s*dict\s*/\s*\>");
                        if (emptyDictRegex.IsMatch(entitlementsFileText))
                        {
                            entitlementsFileText = emptyDictRegex.Replace(entitlementsFileText, newDictEntry + "\n</dict>");
                        }
                        else
                        {
                            entitlementsFileText = new Regex(@"\<\s*dict\s*\>").Replace(entitlementsFileText, newDictEntry);
                        }
                    }
                    else
                    {
                        entitlementsFileText = new Regex(@"aps-environment\s*\<\s*/\s*key\s*\>\s*\<\s*string\s*\>.*\<").Replace(entitlementsFileText, "aps-environment</key>\n    <string>development<");
                    }

                    File.WriteAllText(entitlementsFullFileName, entitlementsFileText);

                    if (updatePBXProject)
                    {
                        PBXProject pbxProject = new PBXProject();
                        pbxProject.ReadFromString(xcodeProjectText);
                        string targetGuid = pbxProject.TargetGuidByName(unityTargetName);

                        pbxProject.AddBuildProperty(targetGuid, "CODE_SIGN_ENTITLEMENTS", entitlementsFileName);

                        pbxProject.AddFile(entitlementsFileName, entitlementsFileName, PBXSourceTree.Source);

                        pbxProject.WriteToFile(xcodeProjectPath);
                    }
                }
            }
            catch (System.Exception e)
            {
                Debug.LogError("UTNotifications: Unable to automatically configure Xcode project's Push Notifications settings. Usually it is caused by using an out-of-date version of Xcode. Push Notifications, if enabled, may work or not, depending on a specific Xcode version.\n" + e.StackTrace);
                File.WriteAllText(xcodeProjectPath, initialXcodeProjectText);
            }
#endif  // Unity 5.0+
        }
    }
}

#endif
