#if !UNITY_EDITOR && UNITY_ANDROID

using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace UTNotifications
{
    public class ManagerImpl : Manager
    {
    //public
        public override bool Initialize(bool willHandleReceivedNotifications, int startId = 0, bool incrementalId = false)
        {
            m_willHandleReceivedNotifications = willHandleReceivedNotifications;

            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    return manager.CallStatic<bool>("initialize", Settings.Instance.PushNotificationsEnabledFirebase, Settings.Instance.PushNotificationsEnabledAmazon, Settings.Instance.FirebaseSenderID, willHandleReceivedNotifications, startId, incrementalId, (int)Settings.Instance.AndroidShowNotificationsMode, Settings.Instance.AndroidRestoreScheduledNotificationsAfterReboot, (int)Settings.Instance.AndroidNotificationsGrouping, Settings.Instance.AndroidShowLatestNotificationOnly, Settings.Instance.PushPayloadTitleFieldName, Settings.Instance.PushPayloadTextFieldName, Settings.Instance.PushPayloadUserDataParentFieldName, Settings.Instance.PushPayloadNotificationProfileFieldName, Settings.Instance.PushPayloadIdFieldName, Settings.Instance.PushPayloadBadgeFieldName);
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
                return false;
            }
        }

        public override void PostLocalNotification(string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("postNotification", WWW.EscapeURL(title), WWW.EscapeURL(text), id, PackUserData(userData), notificationProfile, badgeNumber);
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }
        }

        public override void ScheduleNotification(int triggerInSeconds, string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("scheduleNotification", triggerInSeconds, WWW.EscapeURL(title), WWW.EscapeURL(text), id, PackUserData(userData), notificationProfile, badgeNumber);
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }
        }

        public override void ScheduleNotificationRepeating(int firstTriggerInSeconds, int intervalSeconds, string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("scheduleNotificationRepeating", firstTriggerInSeconds, intervalSeconds, WWW.EscapeURL(title), WWW.EscapeURL(text), id, PackUserData(userData), notificationProfile, badgeNumber);
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }
        }

        public override bool NotificationsEnabled()
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    return manager.CallStatic<bool>("notificationsEnabled");
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
                return false;
            }
        }

        public override bool NotificationsAllowed()
        {
            //Always allowed on Android
            return true;
        }

        public override void SetNotificationsEnabled(bool enabled)
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("setNotificationsEnabled", enabled);
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }
        }

        public override bool PushNotificationsEnabled()
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    return manager.CallStatic<bool>("pushNotificationsEnabled") &&
                            ((Settings.Instance.PushNotificationsEnabledFirebase && manager.CallStatic<bool>("fcmProviderAvailable")) ||
                            (Settings.Instance.PushNotificationsEnabledAmazon && manager.CallStatic<bool>("admProviderAvailable")));
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
                return false;
            }
        }

        public override bool SetPushNotificationsEnabled(bool enabled)
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("setPushNotificationsEnabled", enabled);
                    return PushNotificationsEnabled();
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
                return false;
            }
        }

        public override void CancelNotification(int id)
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("cancelNotification", id);
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }

            HideNotification(id);
        }

        public override void HideNotification(int id)
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("hideNotification", id);
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }
        }

        public override void HideAllNotifications()
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("hideAllNotifications");
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }
        }

        public override void CancelAllNotifications()
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("cancelAllNotifications");
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }
        }

        public override int GetBadge()
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    return manager.CallStatic<int>("getBadge");
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
                return 0;
            }
        }
        
        public override void SetBadge(int bandgeNumber)
        {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("setBadge", bandgeNumber);
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }
        }

        public void _OnAndroidIdReceived(string providerAndId)
        {
            JSONNode json = JSON.Parse(providerAndId);
            
            if (OnSendRegistrationIdHasSubscribers())
            {
                _OnSendRegistrationId(json[0], json[1]);
            }
        }

    //protected
        protected void LateUpdate()
        {
            m_timeToCheckForIncomingNotifications -= Time.unscaledDeltaTime;
            if (m_timeToCheckForIncomingNotifications > 0)
            {
                return;
            }

            m_timeToCheckForIncomingNotifications = m_timeBetweenCheckingForIncomingNotifications;

            if (OnNotificationClickedHasSubscribers())
            {
                try
                {
                    using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                    {
                        HandleClickedNotification(manager.CallStatic<string>("getClickedNotificationPacked"));
                    }
                }
                catch (AndroidJavaException e)
                {
                    Debug.LogException(e);
                }
            }

            if (m_willHandleReceivedNotifications && OnNotificationsReceivedHasSubscribers())
            {
                try
                {
                    using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                    {
                        HandleReceivedNotifications(manager.CallStatic<string>("getReceivedNotificationsPacked"));
                    }
                }
                catch (AndroidJavaException e)
                {
                    Debug.LogException(e);
                }
            }
        }

        protected void OnApplicationPause(bool paused) {
            try
            {
                using (AndroidJavaClass manager = new AndroidJavaClass("universal.tools.notifications.Manager"))
                {
                    manager.CallStatic("setBackgroundMode", paused);
                }
            }
            catch (AndroidJavaException e)
            {
                Debug.LogException(e);
            }
        }

    //private
        private static string[] PackUserData(IDictionary<string, string> userData)
        {
            if (userData == null || userData.Count == 0)
            {
                return null;
            }

            string[] result = new string[userData.Count * 2];
            int i = 0;
            foreach (KeyValuePair<string, string> it in userData)
            {
                result[i++] = it.Key;
                result[i++] = it.Value;
            }

            return result;
        }

        private void HandleClickedNotification(string receivedNotificationPacked)
        {
            if (!string.IsNullOrEmpty(receivedNotificationPacked))
            {
                _OnNotificationClicked(ParseReceivedNotification(JSON.Parse(receivedNotificationPacked)));
            }
        }

        private void HandleReceivedNotifications(string receivedNotificationsPacked)
        {
            if (string.IsNullOrEmpty(receivedNotificationsPacked) || receivedNotificationsPacked == "[]")
            {
                return;
            }

            List<ReceivedNotification> receivedNotifications = new List<ReceivedNotification>();

            JSONNode notificationsList = JSON.Parse(receivedNotificationsPacked);
            for (int i = 0; i < notificationsList.Count; ++i)
            {
                JSONNode json = notificationsList[i];

                ReceivedNotification receivedNotification = ParseReceivedNotification(json);

                //Update out-of-date notifications
                bool updated = false;
                for (int j = 0; j < receivedNotifications.Count; ++j)
                {
                    if (receivedNotifications[j].id == receivedNotification.id)
                    {
                        receivedNotifications[j] = receivedNotification;
                        updated = true;
                        break;
                    }
                }
                if (!updated)
                {
                    receivedNotifications.Add(receivedNotification);
                }
            }

            _OnNotificationsReceived(receivedNotifications);
        }

        ReceivedNotification ParseReceivedNotification(JSONNode json)
        {
            string title = WWW.UnEscapeURL(json["title"].Value);
            string text = WWW.UnEscapeURL(json["text"].Value);
            int id = json["id"].AsInt;
            string notificationProfile = json["notification_profile"].Value;
            int badgeNumber = json["badgeNumber"].AsInt;
            
            JSONNode userDataJson = json["user_data"];
            
            Dictionary<string, string> userData;
            if (userDataJson != null && userDataJson.Count > 0)
            {
                userData = new Dictionary<string, string>();
                foreach (KeyValuePair<string, JSONNode> it in (JSONClass)userDataJson)
                {
                    userData.Add(WWW.UnEscapeURL(it.Key), WWW.UnEscapeURL(it.Value.Value));
                }
            }
            else
            {
                userData = null;
            }
            
            return new ReceivedNotification(title, text, id, userData, notificationProfile, badgeNumber);
        }

        private bool m_willHandleReceivedNotifications;
        private const float m_timeBetweenCheckingForIncomingNotifications = 0.5f;
        private float m_timeToCheckForIncomingNotifications = 0;
    }
}
#endif //UNITY_ANDROID