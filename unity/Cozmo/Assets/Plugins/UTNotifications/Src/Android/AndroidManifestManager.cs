#if UNITY_EDITOR && !UNITY_WEBPLAYER && !UNITY_SAMSUNGTV

using UnityEngine;
using UnityEditor;
using System.IO;
using System.Xml;
using System;

namespace UTNotifications
{
    public static class AndroidManifestManager
    {
    //public
        public static void Update()
        {
            string manifestFile = Settings.MainAndroidManifestFullPath;

            //If AndroidManifest.xml doesn't exist in project copy default one
            if (!File.Exists(manifestFile))
            {
                //prior Unity 5.2
                string defaultManifest = Path.Combine(EditorApplication.applicationContentsPath, "PlaybackEngines/androidplayer/AndroidManifest.xml");

                if (!File.Exists(defaultManifest))
                {
                    //Unity 5.2
                    defaultManifest = Path.Combine(EditorApplication.applicationContentsPath, "PlaybackEngines/AndroidPlayer/Apk/AndroidManifest.xml");
                }

                if (!File.Exists(defaultManifest))
                {
                    //Unity 5.3+
                    defaultManifest = Path.Combine(Settings.FullPath(Settings.AssetsRelatedEditorPath), "Android/DefaultAndroidManifest.xml");
                }

                File.Copy(defaultManifest, manifestFile);
				AssetDatabase.Refresh();
            }

            XmlDocument xmlDocument = new XmlDocument();
            try
            {
                xmlDocument.Load(manifestFile);
            }
            catch (Exception e)
            {
                Debug.LogException(e);
                return;
            }

            XmlElement manifestNode = XmlUtils.FindChildNode(xmlDocument, "manifest") as XmlElement;
            XmlNode applicationNode = XmlUtils.FindChildNode(manifestNode, "application");
            
            if (applicationNode == null)
            {
                Debug.LogError("Error parsing " + manifestFile);
                return;
            }

            string ns = applicationNode.GetNamespaceOfPrefix("android");

            UpdateUTNotificationsCommon(xmlDocument, manifestNode, applicationNode, ns);
            UpdateFirebaseCloudMessaging(xmlDocument, manifestNode, applicationNode, ns);
            UpdateAmazonDeviceMessaging(xmlDocument, manifestNode, applicationNode, ns);

            xmlDocument.Save(manifestFile);
        }

    //private
        private static void UpdateUTNotificationsCommon(XmlDocument xmlDocument, XmlNode manifestNode, XmlNode applicationNode, string ns)
        {
            //<application>
            //    <!-- UTNotifications common -->
            //    <receiver android:name="universal.tools.notifications.AlarmBroadcastReceiver" />
            //    <service android:name="universal.tools.notifications.NotificationIntentService" />
            //</application>
            XmlUtils.UpdateOrCreateElement(xmlDocument, applicationNode, "receiver", "name", ns, "universal.tools.notifications.AlarmBroadcastReceiver", "UTNotifications common");
            var serviceElement = XmlUtils.UpdateOrCreateElement(xmlDocument, applicationNode, "service", "name", ns, "universal.tools.notifications.NotificationIntentService");
            serviceElement.SetAttribute("exported", ns, "true");

            if (Settings.Instance.PushNotificationsEnabledFirebase || Settings.Instance.PushNotificationsEnabledAmazon)
            {
                //<uses-permission android:name="android.permission.INTERNET"/>
                XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "uses-permission", "name", ns, "android.permission.INTERNET");
            }
            //<uses-permission android:name="android.permission.VIBRATE"/>
            XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "uses-permission", "name", ns, "android.permission.VIBRATE");
            //<uses-permission android:name="android.permission.WAKE_LOCK" />
            XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "uses-permission", "name", ns, "android.permission.WAKE_LOCK");

            UpdateRestoreScheduledOnReboot(xmlDocument, manifestNode, applicationNode, ns);
            UpdateAndroid4CompatibilityMode(xmlDocument, manifestNode, applicationNode, ns);
        }

        // Not supported anymore
        private static void UpdateAndroid4CompatibilityMode(XmlDocument xmlDocument, XmlNode manifestNode, XmlNode applicationNode, string ns)
        {
            string comment = "Android 4.4 Compatibility Mode";
            XmlUtils.RemoveElement(xmlDocument, manifestNode, "uses-sdk", "targetSdkVersion", ns, "20", comment);
        }

        private static void UpdateFirebaseCloudMessaging(XmlDocument xmlDocument, XmlNode manifestNode, XmlNode applicationNode, string ns)
        {
            string comment = "Firebase Cloud Messaging";

            //<application>
            //    <!-- Firebase Cloud Messaging -->
            //    <receiver android:name="com.google.android.gms.gcm.GcmReceiver" android:exported="true" android:permission="com.google.android.c2dm.permission.SEND">
            //        <intent-filter>
            //            <action android:name="com.google.android.c2dm.intent.REGISTRATION" />
            //            <action android:name="com.google.android.c2dm.intent.RECEIVE" />
            //            <category android:name="<BUNDLE ID>" />
            //        </intent-filter>
            //    </receiver>
            //    <service android:name="universal.tools.notifications.GcmIntentService" android:exported="false">
            //        <intent-filter>
            //            <action android:name="com.google.android.c2dm.intent.RECEIVE" />
            //        </intent-filter>
            //    </service>
            //    <service android:name="universal.tools.notifications.GcmInstanceIDListenerService" android:exported="false">
            //        <intent-filter>
            //            <action android:name="com.google.android.gms.iid.InstanceID"/>
            //        </intent-filter>
            //    </service>
            //</application>

            //Remove outdated entry
            XmlUtils.RemoveElement(xmlDocument, applicationNode, "receiver", "name", ns, "universal.tools.notifications.GcmBroadcastReceiver", comment);

            if (Settings.Instance.PushNotificationsEnabledFirebase)
            {
                //receiver and comment
                {
                    XmlElement receiverElement = XmlUtils.UpdateOrCreateElement(xmlDocument, applicationNode, "receiver", "name", ns, "com.google.android.gms.gcm.GcmReceiver", comment);
                    receiverElement.SetAttribute("permission", ns, "com.google.android.c2dm.permission.SEND");
                    receiverElement.SetAttribute("exported", ns, "true");

                    //intent-filter
                    XmlElement intentFilter = XmlUtils.UpdateOrCreateElement(xmlDocument, receiverElement, "intent-filter");

                    //actions
                    XmlUtils.UpdateOrCreateElement(xmlDocument, intentFilter, "action", "name", ns, "com.google.android.c2dm.intent.REGISTRATION");
                    XmlUtils.UpdateOrCreateElement(xmlDocument, intentFilter, "action", "name", ns, "com.google.android.c2dm.intent.RECEIVE");

                    //category
                    XmlUtils.UpdateOrCreateElement(xmlDocument, intentFilter, "category", "name", ns, ApplicationIdentifier());
                }

                //service GcmIntentService
                {
                    XmlElement serviceElement = XmlUtils.UpdateOrCreateElement(xmlDocument, applicationNode, "service", "name", ns, "universal.tools.notifications.GcmIntentService");
                    serviceElement.SetAttribute("exported", ns, "false");

                    //intent-filter
                    XmlElement intentFilter = XmlUtils.UpdateOrCreateElement(xmlDocument, serviceElement, "intent-filter");

                    //action
                    XmlUtils.UpdateOrCreateElement(xmlDocument, intentFilter, "action", "name", ns, "com.google.android.c2dm.intent.RECEIVE");
                }

                //service GcmInstanceIDListenerService
                {
                    XmlElement serviceElement = XmlUtils.UpdateOrCreateElement(xmlDocument, applicationNode, "service", "name", ns, "universal.tools.notifications.GcmInstanceIDListenerService");
                    serviceElement.SetAttribute("exported", ns, "false");

                    //intent-filter
                    XmlElement intentFilter = XmlUtils.UpdateOrCreateElement(xmlDocument, serviceElement, "intent-filter");

                    //action
                    XmlUtils.UpdateOrCreateElement(xmlDocument, intentFilter, "action", "name", ns, "com.google.android.gms.iid.InstanceID");
                }
            }
            else
            {
                XmlUtils.RemoveElement(xmlDocument, applicationNode, "receiver", "name", ns, "com.google.android.gms.gcm.GcmReceiver", comment);
                XmlUtils.RemoveElement(xmlDocument, applicationNode, "service", "name", ns, "universal.tools.notifications.GcmIntentService");
                XmlUtils.RemoveElement(xmlDocument, applicationNode, "service", "name", ns, "universal.tools.notifications.GcmInstanceIDListenerService");
            }

            //<!-- Firebase Cloud Messaging -->
            //<permission android:name="<BUNDLE ID>.permission.C2D_MESSAGE" android:protectionLevel="signature" />
            //<uses-permission android:name="<BUNDLE ID>.permission.C2D_MESSAGE" />
            //<uses-permission android:name="com.google.android.c2dm.permission.RECEIVE" />

            if (Settings.Instance.PushNotificationsEnabledFirebase)
            {
                XmlElement permission = XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "permission", "name", ns, ApplicationIdentifier() + ".permission.C2D_MESSAGE", comment);
                permission.SetAttribute("protectionLevel", ns, "signature");
                XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "uses-permission", "name", ns, ApplicationIdentifier() + ".permission.C2D_MESSAGE");
                XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "uses-permission", "name", ns, "com.google.android.c2dm.permission.RECEIVE");
            }
            else
            {
                XmlUtils.RemoveElement(xmlDocument, manifestNode, "permission", "name", ns, ApplicationIdentifier() + ".permission.C2D_MESSAGE", comment);
                XmlUtils.RemoveElement(xmlDocument, manifestNode, "uses-permission", "name", ns, ApplicationIdentifier() + ".permission.C2D_MESSAGE");
                XmlUtils.RemoveElement(xmlDocument, manifestNode, "uses-permission", "name", ns, "com.google.android.c2dm.permission.RECEIVE");
            }
        }

        private static void UpdateAmazonDeviceMessaging(XmlDocument xmlDocument, XmlElement manifestNode, XmlNode applicationNode, string ns)
        {
            string comment = "Amazon Device Messaging";

            //<manifest xmlns:amazon="http://schemas.amazon.com/apk/res/android" />
            string xmlns = manifestNode.GetNamespaceOfPrefix("xmlns");
            string amazonNs = "http://schemas.amazon.com/apk/res/android";
            manifestNode.SetAttribute("amazon", xmlns, amazonNs);
            
            //<application>
            //    <!-- Amazon Device Messaging -->
            //    <amazon:enable-feature android:name="com.amazon.device.messaging" android:required="false" />
            //    <receiver android:name="universal.tools.notifications.AdmBroadcastReceiver" android:permission="com.amazon.device.messaging.permission.SEND" >
            //        <intent-filter>
            //            <action android:name="com.amazon.device.messaging.intent.REGISTRATION" />
            //            <action android:name="com.amazon.device.messaging.intent.RECEIVE" />
            //            <category android:name="<BUNDLE ID>" />
            //        </intent-filter>
            //    </receiver>
            //    <service android:name="universal.tools.notifications.AdmIntentService" android:exported="false" />
            //</application>

            if (Settings.Instance.PushNotificationsEnabledAmazon)
            {
                //receiveramazon:enable-feature and comment
                XmlElement enableFeature = XmlUtils.UpdateOrCreateElement(xmlDocument, applicationNode, "amazon:enable-feature", "name", ns, "com.amazon.device.messaging", comment, amazonNs);
                enableFeature.SetAttribute("required", ns, "false");

                //receiver
                XmlElement receiverElement = XmlUtils.UpdateOrCreateElement(xmlDocument, applicationNode, "receiver", "name", ns, "universal.tools.notifications.AdmBroadcastReceiver");
                receiverElement.SetAttribute("permission", ns, "com.amazon.device.messaging.permission.SEND");
                
                //intent-filter
                XmlElement intentFilter = XmlUtils.UpdateOrCreateElement(xmlDocument, receiverElement, "intent-filter");

                //actions
                XmlUtils.UpdateOrCreateElement(xmlDocument, intentFilter, "action", "name", ns, "com.amazon.device.messaging.intent.REGISTRATION");
                XmlUtils.UpdateOrCreateElement(xmlDocument, intentFilter, "action", "name", ns, "com.amazon.device.messaging.intent.RECEIVE");

                //category
                XmlUtils.UpdateOrCreateElement(xmlDocument, intentFilter, "category", "name", ns, ApplicationIdentifier());
                
                //service
                XmlElement service = XmlUtils.UpdateOrCreateElement(xmlDocument, applicationNode, "service", "name", ns, "universal.tools.notifications.AdmIntentService");
                service.SetAttribute("exported", ns, "false");
            }
            else
            {
                XmlUtils.RemoveElement(xmlDocument, applicationNode, "amazon:enable-feature", "name", ns, "com.amazon.device.messaging", comment, amazonNs);
                XmlUtils.RemoveElement(xmlDocument, applicationNode, "receiver", "name", ns, "universal.tools.notifications.AdmBroadcastReceiver");
                XmlUtils.RemoveElement(xmlDocument, applicationNode, "service", "name", ns, "universal.tools.notifications.AdmIntentService");
            }
            
            //<!-- Amazon Device Messaging -->
            //<permission android:name="<BUNDLE ID>.permission.RECEIVE_ADM_MESSAGE" android:protectionLevel="signature" />
            //<uses-permission android:name="<BUNDLE ID>.permission.RECEIVE_ADM_MESSAGE" />
            //<uses-permission android:name="com.amazon.device.messaging.permission.RECEIVE" />

            if (Settings.Instance.PushNotificationsEnabledAmazon)
            {
                XmlElement permission = XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "permission", "name", ns, ApplicationIdentifier() + ".permission.RECEIVE_ADM_MESSAGE", comment);
                permission.SetAttribute("protectionLevel", ns, "signature");
                XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "uses-permission", "name", ns, ApplicationIdentifier() + ".permission.RECEIVE_ADM_MESSAGE");
                XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "uses-permission", "name", ns, "com.amazon.device.messaging.permission.RECEIVE");
            }
            else
            {
                XmlUtils.RemoveElement(xmlDocument, manifestNode, "permission", "name", ns, ApplicationIdentifier() + ".permission.RECEIVE_ADM_MESSAGE", comment);
                XmlUtils.RemoveElement(xmlDocument, manifestNode, "uses-permission", "name", ns, ApplicationIdentifier() + ".permission.RECEIVE_ADM_MESSAGE");
                XmlUtils.RemoveElement(xmlDocument, manifestNode, "uses-permission", "name", ns, "com.amazon.device.messaging.permission.RECEIVE");
            }
        }

        private static void UpdateRestoreScheduledOnReboot(XmlDocument xmlDocument, XmlNode manifestNode, XmlNode applicationNode, string ns)
        {
            string comment = "Restore Scheduled Notifications On Reboot";

            //<application>
            //    <!-- Restore Scheduled Notifications On Reboot -->
            //    <receiver android:name="universal.tools.notifications.ScheduledNotificationsRestorer">
            //        <intent-filter>
            //            <action android:name="android.intent.action.BOOT_COMPLETED" />
            //        </intent-filter>
            //    </receiver>
            //</application>

            if (Settings.Instance.AndroidRestoreScheduledNotificationsAfterReboot)
            {
                //receiver and comment
                XmlElement receiverElement = XmlUtils.UpdateOrCreateElement(xmlDocument, applicationNode, "receiver", "name", ns, "universal.tools.notifications.ScheduledNotificationsRestorer", comment);
                
                //intent-filter
                XmlElement intentFilter = XmlUtils.UpdateOrCreateElement(xmlDocument, receiverElement, "intent-filter");

                //action
                XmlUtils.UpdateOrCreateElement(xmlDocument, intentFilter, "action", "name", ns, "android.intent.action.BOOT_COMPLETED");
            }
            else
            {
                XmlUtils.RemoveElement(xmlDocument, applicationNode, "receiver", "name", ns, "universal.tools.notifications.ScheduledNotificationsRestorer", comment);
            }

            //<!-- Restore Scheduled Notifications On Reboot -->
            //<uses-permission android:name="android.permission.RECEIVE_BOOT_COMPLETED" />
            if (Settings.Instance.AndroidRestoreScheduledNotificationsAfterReboot)
            {
                XmlUtils.UpdateOrCreateElement(xmlDocument, manifestNode, "uses-permission", "name", ns, "android.permission.RECEIVE_BOOT_COMPLETED", comment);
            }
            else
            {
                XmlUtils.RemoveElement(xmlDocument, manifestNode, "uses-permission", "name", ns, "android.permission.RECEIVE_BOOT_COMPLETED", comment);
            }
        }

        private static string ApplicationIdentifier()
        {
#if UNITY_5_6_OR_NEWER
            return PlayerSettings.applicationIdentifier;
#else
            return PlayerSettings.bundleIdentifier;
#endif
        }
    }
}

#endif //UNITY_EDITOR