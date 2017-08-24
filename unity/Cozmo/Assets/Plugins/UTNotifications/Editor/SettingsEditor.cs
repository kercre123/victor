#if !UNITY_WEBPLAYER && !UNITY_SAMSUNGTV

using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using UnityEditor;

namespace UTNotifications
{
    [CustomEditor(typeof(Settings))]
    public class SettingsEditor : Editor
    {
    //public
        public override void OnInspectorGUI()
        {
            float previousLabelWidth = EditorGUIUtility.labelWidth;
            EditorGUIUtility.labelWidth = 245.0f;
            try
            {
                DrawVersion();
                DrawHelp();
                DrawNotificationProfilesGUI();
                DrawIOSSettingsGUI();
                DrawAndroidSettingsGUI();
                DrawWindowsSettingsGUI();
                DrawAdvancedSettingsGUI();
            }
            finally
            {
                EditorGUIUtility.labelWidth = previousLabelWidth;
            }
        }

    //private
        private void OnEnable()
        {
            m_amazonDebugAPIKey = Settings.GetAmazonAPIKey();
        }

        private void DrawVersion()
        {
            EditorGUILayout.BeginHorizontal();
            GUILayout.FlexibleSpace();
            GUILayout.Label("Version: " + Settings.Version);
            EditorGUILayout.EndHorizontal();
        }

        private void DrawHelp()
        {
            if (m_showHelp = EditorGUILayout.Foldout(m_showHelp, "Help"))
            {
                EditorGUILayout.BeginHorizontal();

                if (GUILayout.Button("Manual"))
                {
                    Application.OpenURL("http://universal-tools.github.io/UTNotifications/Manual_1.6.1.pdf");
                }
                
                if (GUILayout.Button("API Reference"))
                {
                    Application.OpenURL("http://universal-tools.github.io/UTNotifications/html_1.6.1/annotated.html");
                }
                
                EditorGUILayout.EndHorizontal();

                EditorGUILayout.BeginHorizontal();
                
                if (GUILayout.Button("Forum"))
                {
                    Application.OpenURL("http://forum.unity3d.com/threads/released-utnotifications-professional-cross-platform-push-notifications-and-more.333045/");
                }
                
                if (GUILayout.Button("Report Issue"))
                {
                    Application.OpenURL("https://github.com/universal-tools/UTNotificationsFeedback/issues");
                }
                
                EditorGUILayout.EndHorizontal();

                EditorGUILayout.BeginHorizontal();
                
                if (GUILayout.Button("Feedback"))
                {
                    Application.OpenURL("https://www.assetstore.unity3d.com/#!/content/37767");
                }

                if (GUILayout.Button("Support Email"))
                {
                    Application.OpenURL("mailto:universal.tools.contact@gmail.com");
                }
                
                EditorGUILayout.EndHorizontal();
            }
            
            EditorGUILayout.Space();
        }

        private void DrawNotificationProfilesGUI()
        {
            if (m_showNotificationProfilesSettings = EditorGUILayout.Foldout(m_showNotificationProfilesSettings, "Notification Profiles (Sounds & Icons)"))
            {
                List<Settings.NotificationProfile> profiles = Settings.Instance.NotificationProfiles;
                string profileToRemove = null;
                foreach (var profile in profiles)
                {
                    EditorGUILayout.BeginHorizontal();
                    if (GUILayout.Button(profile.profileName))
                    {
                        NotificationProfileDialog.ShowDialog(profile);
                    }
                    if (profile.profileName != Settings.DEFAULT_PROFILE_NAME && GUILayout.Button("-", GUILayout.Width(18)))
                    {
                        profileToRemove = profile.profileName;
                    }
                    EditorGUILayout.EndHorizontal();
                }

                if (profileToRemove != null)
                {
                    NotificationProfileDialog.RemoveProfile(profileToRemove);
                }

                if (GUILayout.Button("+"))
                {
                    NotificationProfileDialog.ShowDialog(new Settings.NotificationProfile());
                }
            }

            EditorGUILayout.Space();
        }

        private string GetSoundName(string sound)
        {
            return string.IsNullOrEmpty(sound) ? "[default]" : sound;
        }

        private string GetIconName(string icon)
        {
            return string.IsNullOrEmpty(icon) ? "[default]" : icon;
        }

        private void DrawAndroidSettingsGUI()
        {
            if (m_showAndroidSettings = EditorGUILayout.Foldout(m_showAndroidSettings, "Android"))
            {
                EditorGUILayout.LabelField(m_androidShowNotificationsLabel);
                Settings.Instance.AndroidShowNotificationsMode = (Settings.ShowNotifications)EditorGUILayout.EnumPopup(Settings.Instance.AndroidShowNotificationsMode);
                Settings.Instance.AndroidRestoreScheduledNotificationsAfterReboot = EditorGUILayout.Toggle(m_androidRestoreScheduledNotificationsAfterRebootLabel, Settings.Instance.AndroidRestoreScheduledNotificationsAfterReboot);
                
				EditorGUILayout.BeginHorizontal();
                EditorGUILayout.LabelField(m_androidNotificationsGroupingLabel, GUILayout.Width(175));
                if (GUILayout.Button("More info..."))
                {
                    Application.OpenURL("https://developer.android.com/training/wearables/notifications/stacks.html");
                }
                EditorGUILayout.EndHorizontal();
                Settings.Instance.AndroidNotificationsGrouping = (Settings.NotificationsGroupingMode)EditorGUILayout.EnumPopup(Settings.Instance.AndroidNotificationsGrouping);
                if (Settings.Instance.AndroidNotificationsGrouping == Settings.NotificationsGroupingMode.FROM_USER_DATA)
                {
                    EditorGUILayout.HelpBox(m_androidNotificationsGroupingFromUserDataInfo, MessageType.Info);
                }
                if (Settings.Instance.AndroidNotificationsGrouping != Settings.NotificationsGroupingMode.NONE)
                {
                    EditorGUILayout.HelpBox(m_androidNotificationsGroupingInfo, MessageType.Info);
                }

				Settings.Instance.AndroidShowLatestNotificationOnly = EditorGUILayout.Toggle(m_androidShowLatestNotificationOnlyLabel, Settings.Instance.AndroidShowLatestNotificationOnly);

                EditorGUILayout.Space();

                DrawFirebaseSettingsGUI();
                DrawAmazonSettingsGUI();
            }
        }

        private void DrawIOSSettingsGUI()
        {
            if (m_showIOSSettings = EditorGUILayout.Foldout(m_showIOSSettings, "iOS"))
            {
                if (Settings.Instance.PushNotificationsEnabledIOS = EditorGUILayout.Toggle(m_applePushNotificationsEnabledLabel, Settings.Instance.PushNotificationsEnabledIOS))
                {
                    EditorGUILayout.LabelField(m_packageNameLabel);
#if UNITY_5_6_OR_NEWER
                    EditorGUILayout.TextArea(PlayerSettings.applicationIdentifier);
#else
                    EditorGUILayout.TextArea(PlayerSettings.bundleIdentifier);
#endif

                    EditorGUILayout.LabelField(m_registrationIdTokenEncodingLabel);
                    Settings.Instance.IOSTokenEncoding = (Settings.TokenEncoding)EditorGUILayout.EnumPopup(Settings.Instance.IOSTokenEncoding);
                }

                EditorGUILayout.Space();
            }
        }

        private void DrawFirebaseSettingsGUI()
        {
            if (m_showFirebaseSettings = EditorGUILayout.Foldout(m_showFirebaseSettings, "Firebase Cloud Messaging"))
            {
                if (Settings.Instance.PushNotificationsEnabledFirebase = EditorGUILayout.Toggle(m_firebasePushNotificationsEnabledLabel, Settings.Instance.PushNotificationsEnabledFirebase))
                {
                    EditorGUILayout.LabelField(m_packageNameLabel);
#if UNITY_5_6_OR_NEWER
                    EditorGUILayout.TextArea(PlayerSettings.applicationIdentifier);
#else
                    EditorGUILayout.TextArea(PlayerSettings.bundleIdentifier);
#endif
                    
                    if (GUILayout.Button("Load google-services.json"))
                    {
                        LoadGoogleServicesJson();
                    }

                    EditorGUILayout.BeginHorizontal();
                    EditorGUILayout.LabelField(m_senderIDLabel, GUILayout.Width(135), GUILayout.Height(16));
                    Settings.Instance.FirebaseSenderID = EditorGUILayout.TextField(Settings.Instance.FirebaseSenderID, GUILayout.Width(120));
                    EditorGUILayout.EndHorizontal();
                }
            }
            EditorGUILayout.Space();
        }

        private void DrawAmazonSettingsGUI()
        {
            if (m_showAmazonSettings = EditorGUILayout.Foldout(m_showAmazonSettings, "Amazon Device Messaging"))
            {
                if (Settings.Instance.PushNotificationsEnabledAmazon = EditorGUILayout.Toggle(m_amazonPushNotificationsEnabledLabel, Settings.Instance.PushNotificationsEnabledAmazon))
                {
                    const int width = 260;

                    EditorGUILayout.LabelField(m_packageNameLabel);
#if UNITY_5_6_OR_NEWER
                    EditorGUILayout.TextArea(PlayerSettings.applicationIdentifier, GUILayout.Width(width));
#else
                    EditorGUILayout.TextArea(PlayerSettings.bundleIdentifier, GUILayout.Width(width));
#endif
                    EditorGUILayout.LabelField(m_androidDebugSignatureMD5Label);
                    EditorGUILayout.TextArea(m_androidDebugSignatureMD5, GUILayout.Width(width));

                    EditorGUILayout.LabelField(m_amazonDebugAPIKeyLabel);
                    string apiKey = EditorGUILayout.TextArea(m_amazonDebugAPIKey, GUILayout.Width(width));
                    if (apiKey != m_amazonDebugAPIKey)
                    {
                        m_amazonDebugAPIKey = apiKey;
                        Settings.SetAmazonAPIKey(apiKey);
                    }
                }
            }
            EditorGUILayout.Space();
        }

        private void DrawWindowsSettingsGUI()
        {
            if (m_showWindowsSettings = EditorGUILayout.Foldout(m_showWindowsSettings, "Windows Store"))
            {
                if (Settings.Instance.PushNotificationsEnabledWindows = EditorGUILayout.Toggle(m_windowsPushNotificationsEnabledLabel, Settings.Instance.PushNotificationsEnabledWindows))
                {
                    string certificatePublisher = Settings.Instance.WindowsCertificatePublisher;
                    EditorGUILayout.BeginHorizontal();
                    EditorGUILayout.LabelField(m_windowsCertificatePublisherLabel, GUILayout.Width(135), GUILayout.Height(16));
                    EditorGUILayout.LabelField(certificatePublisher);
                    EditorGUILayout.EndHorizontal();

                    if (!Settings.Instance.WindowsCertificateIsCorrect(certificatePublisher))
                    {
                        EditorGUILayout.HelpBox(Settings.WRONG_CERTIFICATE_MESSAGE, MessageType.Warning);
                    }

                    EditorGUILayout.BeginHorizontal();
                    EditorGUILayout.LabelField(m_windowsIdentityNameLabel, GUILayout.Width(135), GUILayout.Height(16));
                    Settings.Instance.WindowsIdentityName = EditorGUILayout.TextField(Settings.Instance.WindowsIdentityName);
                    EditorGUILayout.EndHorizontal();
                }

                Settings.Instance.WindowsDontShowWhenRunning = EditorGUILayout.Toggle(m_windowsDontShowWhenRunningLabel, Settings.Instance.WindowsDontShowWhenRunning);

                EditorGUILayout.Space();
            }
        }

        private void DrawAdvancedSettingsGUI()
        {
            if (m_showAdvancedSettings = EditorGUILayout.Foldout(m_showAdvancedSettings, "Advanced"))
            {
                DrawPushPayloadFormatGUI();
            }
        }

        private void DrawPushPayloadFormatGUI()
        {
            if (m_showPushPayloadFormatSettings = EditorGUILayout.Foldout(m_showPushPayloadFormatSettings, "Push Payload Format (FCM, ADM, WNS)"))
            {
                Settings.Instance.PushPayloadTitleFieldName = PushFormatField("Title", Settings.Instance.PushPayloadTitleFieldName);
                Settings.Instance.PushPayloadTextFieldName = PushFormatField("Text", Settings.Instance.PushPayloadTextFieldName);
                Settings.Instance.PushPayloadUserDataParentFieldName = PushFormatField("User data parent (opt)", Settings.Instance.PushPayloadUserDataParentFieldName);
                Settings.Instance.PushPayloadNotificationProfileFieldName = PushFormatField("Notification profile (opt)", Settings.Instance.PushPayloadNotificationProfileFieldName);
                Settings.Instance.PushPayloadIdFieldName = PushFormatField("Id (opt)", Settings.Instance.PushPayloadIdFieldName);
                Settings.Instance.PushPayloadBadgeFieldName = PushFormatField("Badge (opt)", Settings.Instance.PushPayloadBadgeFieldName);

                EditorGUILayout.Space();
            }
        }

        private void LoadGoogleServicesJson()
        {
            string filePath = EditorUtility.OpenFilePanel("Open google-services.json", "", "json");
            if (!string.IsNullOrEmpty(filePath))
            {
                JSONNode json = JSON.Parse(File.ReadAllText(filePath));

                try
                {
                    var clientNode = json["client"];

                    bool found = false;
                    for (int i = 0; i < clientNode.Count; ++i)
                    {
                        string packageName = clientNode[i]["client_info"]["android_client_info"]["package_name"];
#if UNITY_5_6_OR_NEWER
                        if (packageName == PlayerSettings.applicationIdentifier)
#else
                        if (packageName == PlayerSettings.bundleIdentifier)
#endif
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        Debug.LogError(string.Format("package_name mismatch! {0}'s package_name = \"{1}\", though Unity app's bundle identifier is \"{2}\"", Path.GetFileName(filePath), clientNode[0]["client_info"]["android_client_info"]["package_name"],
#if UNITY_5_6_OR_NEWER
                            PlayerSettings.applicationIdentifier
#else
                            PlayerSettings.bundleIdentifier
#endif
                        ));
                        return;
                    }

                    Settings.Instance.FirebaseSenderID = json["project_info"]["project_number"].Value;
                }
                catch (System.Exception e)
                {
                    Debug.LogError("goog-services.json malformed!\n" + e.Message);
                }
            }
        }

        private string PushFormatField(string label, string value)
        {
            const string valuePrefix = "data/";
            const int valuePrefixLength = 5;

            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField(label, GUILayout.Width(135), GUILayout.Height(16));
            value = EditorGUILayout.TextField(valuePrefix + value);
            EditorGUILayout.EndHorizontal();

            // Yes, it's very slow: but no need to optimize as it's called only in the unity editor when an appropriate editor is shown.
            if (value.StartsWith(valuePrefix))
            {
                value = value.Remove(0, valuePrefixLength);
            }

            return value;
        }

        private GUIContent m_applePushNotificationsEnabledLabel = new GUIContent("Push Notifications [?]:", "Turn on to use the APNS.\nMore details in the section \"Configuring the Apple Push Notification Service (APNS)\" of the manual");
        private GUIContent m_firebasePushNotificationsEnabledLabel = new GUIContent("Push Notifications [?]:", "Turn on to use the FCM.\nMore details in the section \"Configuring the Firebase Cloud Messaging (FCM)\" of the manual");
        private GUIContent m_amazonPushNotificationsEnabledLabel = new GUIContent("Push Notifications [?]:", "Turn on to use the the ADM.\nMore details in the section \"Configuring the Amazon Device Messaging (ADM)\" of the manual");
        private GUIContent m_androidShowNotificationsLabel = new GUIContent("Show Notifications:");
        private GUIContent m_androidRestoreScheduledNotificationsAfterRebootLabel = new GUIContent("Restore Notifications On Reboot [?]:", "Restores scheduled notifications after a device reboot.\nIMPORTANT: enabling the option requests permission RECEIVE_BOOT_COMPLETED.");
        private GUIContent m_androidNotificationsGroupingLabel = new GUIContent("Grouping Mode [?]:", "Grouped notifications may display in a cluster or stack on devices which support such rendering (wearable devices).");
        private readonly string m_androidNotificationsGroupingFromUserDataInfo = "User data \"notification_group\" key value will be used as a grouping key.";
        private readonly string m_androidNotificationsGroupingInfo = "In order to use a notification as a group summary, provide user data key \"notification_group_summary\" with any value when posting the notification.";
		private GUIContent m_androidShowLatestNotificationOnlyLabel = new GUIContent("Show Only Latest Notification [?]", "todo");
		private GUIContent m_senderIDLabel = new GUIContent("SenderID [?]:", "The FCM Sender ID.\nFind more details in the manual.");
        private GUIContent m_packageNameLabel = new GUIContent("Hint: Bundle Identifier (Package Id) [?]:", "The bundle identifier is required to configure the push notifications. You can edit this value in the Unity menu:\nFile -> Build Settings... -> Player Settings...");
        private GUIContent m_registrationIdTokenEncodingLabel = new GUIContent("APNS Registration Id encoding [?]:", "The encoding of the push notifications registration id token provided to Manager.OnSendRegistrationId event.\nThe original value provided by APNS is a binary buffer so the encoding is required to send it to your push notifications server as a string.");
        private GUIContent m_androidDebugSignatureMD5Label = new GUIContent("Hint: Android debug signature MD5 [?]:", "It's required to configure the ADM push notifications.\nMore details in the section \"Configuring the Amazon Device Messaging (ADM)/Getting Your OAuth Credentials and API Key\" of the manual");
        private readonly string m_androidDebugSignatureMD5 = Settings.GetAndroidDebugSignatureMD5();
        private GUIContent m_amazonDebugAPIKeyLabel = new GUIContent("Amazon Debug API Key [?]:", "More details in the section \"Configuring the Amazon Device Messaging (ADM)/Getting Your OAuth Credentials and API Key\" of the manual");
        private string m_amazonDebugAPIKey;
        private bool m_showHelp = true;
        private bool m_showNotificationProfilesSettings = true;
        private bool m_showAdvancedSettings = false;
        private bool m_showPushPayloadFormatSettings = true;
        private bool m_showAndroidSettings = false;
        private bool m_showFirebaseSettings = true;
        private bool m_showAmazonSettings = true;
        private bool m_showIOSSettings = false;
        private bool m_showWindowsSettings = false;
        private GUIContent m_windowsPushNotificationsEnabledLabel = new GUIContent("Push Notifications [?]:", "Turn on to use the WNS.\nMore details in the section \"Configuring the Windows Push Notification Services (WNS)\" of the manual");
        private GUIContent m_windowsCertificatePublisherLabel = new GUIContent("Certificate [?]:", "Certificate publisher name. Set in Unity platform settings for Windows Store. More details in the section \"Configuring the Windows Push Notification Services (WNS)\" of the manual");
        private GUIContent m_windowsIdentityNameLabel = new GUIContent("Identity Name [?]:", "More details in the section \"Configuring the Windows Push Notification Services (WNS)\" of the manual");
        private GUIContent m_windowsDontShowWhenRunningLabel = new GUIContent("Notify only when app is closed or hidden");
    }

    public class NotificationProfileDialog : EditorWindow
    {
    //public
        public static void ShowDialog(Settings.NotificationProfile profile)
        {
            NotificationProfileDialog dialog = (NotificationProfileDialog)EditorWindow.GetWindow(typeof(NotificationProfileDialog));
#if UNITY_4_3 || UNITY_4_4 || UNITY_4_5 || UNITY_4_6 || UNITY_4_7 || UNITY_4_8 || UNITY_4_9 || UNITY_5_0
            dialog.title = m_title.text;
#else
            dialog.titleContent = m_title;
#endif
            dialog.maxSize = dialog.minSize = new Vector2(275.0f, 407.0f);
            dialog.m_profile = dialog.m_originalProfile = profile;

            dialog.m_readOnlyProfileName = (profile.profileName == Settings.DEFAULT_PROFILE_NAME);

            dialog.Show();
        }

        public static void RemoveProfile(string profileName)
        {
            RemoveProfileContents(profileName);

            List<Settings.NotificationProfile> profiles = Settings.Instance.NotificationProfiles;
            for (int i = 0; i < profiles.Count; ++i)
            {
                if (profiles[i].profileName == profileName)
                {
                    profiles.RemoveAt(i);
                    Settings.Instance.Save();
                    break;
                }
            }
        }

    //protected
        protected void OnGUI()
        {
            EditorGUILayout.LabelField(m_profileNameLabel);
            if (!m_readOnlyProfileName)
            {
                m_profile.profileName = EditorGUILayout.TextArea(m_profile.profileName);
                if (!CheckProfileName())
                {
                    EditorGUILayout.HelpBox(m_invalidNameErrorMessage, MessageType.Error);
                }
            }
            else
            {
                EditorGUILayout.TextArea(m_profile.profileName);
            }
            EditorGUILayout.Space();

            DrawIOS();
            DrawAndroid();
            
            EditorGUILayout.Space();

            EditorGUILayout.BeginHorizontal();
            if (GUILayout.Button("OK"))
            {
                OnOK();
            }
            if (GUILayout.Button("Cancel"))
            {
                OnCancel();
            }
            EditorGUILayout.EndHorizontal();
        }

    //private
        private void DrawIOS()
        {
            if (m_showIOS = EditorGUILayout.Foldout(m_showIOS, "iOS"))
            {
                if (m_profile.profileName != Settings.DEFAULT_PROFILE_NAME)
                {
                    DrawFileOption(m_iosSoundLabel, ref m_profile.iosSound, new string[] {"wav", "caf", "aiff", "aif"}, "https://developer.apple.com/library/mac/documentation/NetworkingInternet/Conceptual/RemoteNotificationsPG/Chapters/IPhoneOSClientImp.html#//apple_ref/doc/uid/TP40008194-CH103-SW6");
                }
                else
                {
                    EditorGUILayout.HelpBox("iOS doesn't allow configuring default notifications settings. You may use a specific (non-default) notification profile instead.", MessageType.Info);
                }
            }
            
            EditorGUILayout.Space();
        }

        private void DrawAndroid()
        {
            if (m_showAndroid = EditorGUILayout.Foldout(m_showAndroid, "Android"))
            {
                DrawFileOption(m_androidSoundLabel, ref m_profile.androidSound, new string[] {"wav", "mp3", "ogg", "3gp"}, null);
                string[] imageFormats = {"png"};
                DrawFileOption(m_androidIconLabel, ref m_profile.androidIcon, imageFormats, null);
                DrawFileOption(m_androidIcon5PlusLabel, ref m_profile.androidIcon5Plus, imageFormats, "http://stackoverflow.com/questions/28387602/notification-bar-icon-turns-white-in-android-5-lollipop");
                DrawFileOption(m_androidLargeIconLabel, ref m_profile.androidLargeIcon, imageFormats, null);
            }

            EditorGUILayout.Space();
        }

        private bool CheckProfileName()
        {
            if (m_profile.profileName != m_originalProfile.profileName)
            {
                foreach (Settings.NotificationProfile profile in Settings.Instance.NotificationProfiles)
                {
                    if (m_profile.profileName == profile.profileName)
                    {
                        return false;
                    }
                }
            }

            if (string.IsNullOrEmpty(m_profile.profileName))
            {
                return false;
            }

            return Regex.IsMatch(m_profile.profileName, @"^[a-z_][a-z0-9_]*$");
        }

        private void DrawFileOption(GUIContent label, ref string fileName, string[] extenstions, string helpUrl)
        {
            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField(label, GUILayout.Width(190));
            if (helpUrl != null)
            {
                if (GUILayout.Button("More info...", GUILayout.Width(75)))
                {
                    Application.OpenURL(helpUrl);
                }
            }
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.BeginHorizontal();

            if (GUILayout.Button("Reset", GUILayout.Width(50)))
            {
                fileName = null;
            }

            string extensionsAsString = "";
            string extensionString = "";
            if (extenstions != null && extenstions.Length > 0)
            {
                for (int i = 0; i < extenstions.Length; ++i)
                {
                    extensionsAsString += (i > 0 ? "*." : "") + extenstions[i] + (i < extenstions.Length - 1 ? ";" : "");
                }

                //Unfortunately, MacOSX version of Unity doesn't support multiple extensions in the OpenFilePanel
#if UNITY_EDITOR_WIN
                extensionString = extensionsAsString;
#else
                if (extenstions.Length == 1)
                {
                    extensionString = extenstions[0];
                }
#endif
            }

            if (GUILayout.Button(string.IsNullOrEmpty(fileName) ? "Default (choose: *." + extensionsAsString + ")" : new FileInfo(fileName).Name, GUILayout.Width(215)))
            {
                while (true)
                {
                    string selectedFileName = EditorUtility.OpenFilePanel(label.text.Replace(" [?]:", "") + " (" + extensionsAsString + ")", "", extensionString);

                    if (!string.IsNullOrEmpty(selectedFileName))
                    {
                        bool extensionOK = false;
                        foreach (string extension in extenstions)
                        {
                            if (selectedFileName.EndsWith("." + extension))
                            {
                                extensionOK = true;
                                break;
                            }
                        }

                        if (extensionOK)
                        {
                            fileName = selectedFileName;
                        }
                        else
                        {
                            EditorUtility.DisplayDialog("Wrong file format", "Please choose one of: *." + extensionsAsString, "OK");
                            continue;
                        }
                    }

                    break;
                }
            }

            EditorGUILayout.EndHorizontal();
        }

        private void OnOK()
        {
            if (CheckProfileName())
            {
                List<Settings.NotificationProfile> profiles = Settings.Instance.NotificationProfiles;

                int index = -1;
                if (!string.IsNullOrEmpty(m_originalProfile.profileName))
                {
                    for (int i = 0; i < profiles.Count; ++i)
                    {
                        if (m_originalProfile.profileName == profiles[i].profileName)
                        {
                            index = i;
                            break;
                        }
                    }
                }

                if (index < 0)
                {
                    profiles.Add(m_profile);
                }
                else
                {
                    profiles[index] = m_profile;
                }

                Apply();
                Settings.Instance.Save();

                Close();
            }
            else
            {
                EditorUtility.DisplayDialog("Invalid notification profile name", m_invalidNameErrorMessage, "OK");
            }
        }

        private void Apply()
        {
            PrepareAndroidResource();

            string profileName = m_profile.profileName != Settings.DEFAULT_PROFILE_NAME ? m_profile.profileName : Settings.DEFAULT_PROFILE_NAME_INTERNAL;
            
            if (profileName != m_originalProfile.profileName)
            {
                RenameIOSResource("sounds", m_originalProfile.profileName, profileName);
                RenameAndroidResource("drawable", m_originalProfile.profileName, profileName);
                RenameAndroidResource("drawable", m_originalProfile.profileName + "_android5plus", profileName + "_android5plus");
                RenameAndroidResource("drawable", m_originalProfile.profileName + "_large", profileName + "_large");
                RenameAndroidResource("raw", m_originalProfile.profileName, profileName);
            }

            if (m_profile.iosSound != m_originalProfile.iosSound)
            {
                CopyIOSResource(m_profile.iosSound, profileName);
            }

            if (m_profile.androidIcon != m_originalProfile.androidIcon)
            {
                CopyAndroidResource("drawable", m_profile.androidIcon, profileName);
            }

            if (m_profile.androidIcon5Plus != m_originalProfile.androidIcon5Plus)
            {
                CopyAndroidResource("drawable", m_profile.androidIcon5Plus, profileName + "_android5plus");
            }

            if (m_profile.androidLargeIcon != m_originalProfile.androidLargeIcon)
            {
                CopyAndroidResource("drawable", m_profile.androidLargeIcon, profileName + "_large");
            }

            if (m_profile.androidSound != m_originalProfile.androidSound)
            {
                CopyAndroidResource("raw", m_profile.androidSound, profileName);
            }

            AssetDatabase.Refresh();
        }

        private static void RemoveProfileContents(string profileName)
        {
            CopyIOSResource(null, profileName);
            CopyAndroidResource("drawable", null, profileName);
            CopyAndroidResource("drawable", null, profileName + "_android5plus");
            CopyAndroidResource("drawable", null, profileName + "_large");
            CopyAndroidResource("raw", null, profileName);
        }

        private void RenameIOSResource(string resourceType, string oldName, string newName)
        {
            if (string.IsNullOrEmpty(oldName))
            {
                return;
            }

            string iosResPath = Settings.GetIOSResourceFolder();
            if (File.Exists(Path.Combine(iosResPath, oldName)))
            {
                File.Move(Path.Combine(iosResPath, oldName), Path.Combine(iosResPath, newName));
            }
        }

        private void RenameAndroidResource(string resourceType, string oldName, string newName)
        {
            if (string.IsNullOrEmpty(oldName))
            {
                return;
            }

            string androidResPath = Settings.GetAndroidResourceFolder(resourceType);
            if (Directory.Exists(androidResPath))
            {
                FileInfo[] fileInfos = new DirectoryInfo(androidResPath).GetFiles(oldName + ".*");
                if (fileInfos != null && fileInfos.Length > 0)
                {
                    string newPath = Path.Combine(androidResPath, newName + fileInfos[0].Extension);
                    File.Move(fileInfos[0].FullName, newPath);
                }
            }
        }

        private static void CopyIOSResource(string originalPath, string outName)
        {
            string iosResPath = Settings.GetIOSResourceFolder();
            if (File.Exists(Path.Combine(iosResPath, outName)))
            {
                File.Delete(Path.Combine(iosResPath, outName));
            }

            if (!string.IsNullOrEmpty(originalPath))
            {
                if (!Directory.Exists(iosResPath))
                {
                    Directory.CreateDirectory(iosResPath);
                }
                
                File.Copy(originalPath, Path.Combine(iosResPath, outName));
            }
        }

        private static void PrepareAndroidResource()
        {
            string androidResLibPath = Settings.GetAndroidResourceLibFolder();
            if (!Directory.Exists(androidResLibPath))
            {
                Directory.CreateDirectory(androidResLibPath);
            }

            string resLibTemplatePath = Path.Combine(Settings.FullPath(Settings.AssetsRelatedEditorPath), "Android/Res");
            FileInfo[] fileInfos = new DirectoryInfo(resLibTemplatePath).GetFiles();

            foreach (FileInfo fileInfo in fileInfos)
            {
                if (!fileInfo.Name.StartsWith(".") && !fileInfo.Name.EndsWith(".meta"))
                {
                    string targetFileName = Path.Combine(androidResLibPath, fileInfo.Name);
                    File.Copy(fileInfo.FullName, targetFileName, true);
                }
            }
        }

        private static void CopyAndroidResource(string resourceType, string originalPath, string outName)
        {
            string androidResPath = Settings.GetAndroidResourceFolder(resourceType);

            if (Directory.Exists(androidResPath))
            {
                FileInfo[] fileInfos = new DirectoryInfo(androidResPath).GetFiles(outName + ".*");
                if (fileInfos != null)
                {
                    foreach (FileInfo fileInfo in fileInfos)
                    {
                        File.Delete(fileInfo.FullName);
                    }
                }
            }

            if (!string.IsNullOrEmpty(originalPath))
            {
                if (!Directory.Exists(androidResPath))
                {
                    Directory.CreateDirectory(androidResPath);
                }

                File.Copy(originalPath, Path.Combine(androidResPath, outName + new FileInfo(originalPath).Extension));
            }
        }
        
        private void OnCancel()
        {
            Close();
        }

        private static readonly GUIContent m_title = new GUIContent("Edit Profile");
        private GUIContent m_profileNameLabel = new GUIContent("Profile Name [?]:", "Identifies the sound/icons settings which can be used for a notification.");
        private GUIContent m_iosSoundLabel = new GUIContent("Sound [?]:", "The iOS notification sound (should be shorter than 30 seconds).");
        private GUIContent m_androidIconLabel = new GUIContent("Small Icon [?]:", "The notification bar icon. Recommended sizes are 72×72 or 96×96.");
        private GUIContent m_androidIcon5PlusLabel = new GUIContent("Small Icon (Android 5.0+) [?]:", "The notification bar icon for Android 5.0+.\nRecommended sizes are 72×72 or 96×96.\nIf not specified, Small Icon will be used.");
        private GUIContent m_androidLargeIconLabel = new GUIContent("Large Icon [?]:", "The large notification icon.\nRecommended sizes are 128×128 or 192×192.\nIf not specified, Small Icon will be used.");
        private GUIContent m_androidSoundLabel = new GUIContent("Sound [?]:", "The Android notification sound");
        private string m_invalidNameErrorMessage = "Must contain only lower case English letters, digits or '_' character and can't start with digit.\nShould be also unique, not empty and not \"default\".\nFor testing with UTNotificationsSample use \"demo_notification_profile\"";
        private bool m_showAndroid = true;
        private bool m_showIOS = true;
        private bool m_readOnlyProfileName = false;
        private Settings.NotificationProfile m_originalProfile;
        private Settings.NotificationProfile m_profile;
    }
}

#endif
