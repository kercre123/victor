#if UNITY_EDITOR || (!UNITY_IOS && !UNITY_ANDROID && !UNITY_WSA && !UNITY_METRO) || ((UNITY_WSA || UNITY_METRO) && ENABLE_IL2CPP)

using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace UTNotifications
{
    public class ManagerImpl : Manager
    {
    //public
        public override bool Initialize(bool willHandleReceivedNotifications, int startId, bool incrementalId)
        {
            NotSupported();
            m_initialized = true;
            return false;
        }

        public override void PostLocalNotification(string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            CheckInitialized();
            NotSupported();
        }

        public override void ScheduleNotification(int triggerInSeconds, string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            CheckInitialized();
            NotSupported();
        }

        public override void ScheduleNotificationRepeating(int firstTriggerInSeconds, int intervalSeconds, string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            CheckInitialized();
            NotSupported();
        }

        public override bool NotificationsEnabled()
        {
            NotSupported();
            return false;
        }

        public override bool NotificationsAllowed()
        {
            NotSupported();
            return false;
        }

        public override void SetNotificationsEnabled(bool enabled)
        {
            NotSupported();
        }

        public override bool PushNotificationsEnabled()
        {
            NotSupported();
            return false;
        }

        public override bool SetPushNotificationsEnabled(bool enable)
        {
            NotSupported();
            return false;
        }

        public override void CancelNotification(int id)
        {
            CheckInitialized();
            NotSupported();
        }

        public override void HideNotification(int id)
        {
            CheckInitialized();
            NotSupported();
        }

        public override void HideAllNotifications()
        {
            CheckInitialized();
            NotSupported();
        }

        public override void CancelAllNotifications()
        {
            CheckInitialized();
            NotSupported();
        }

        public override int GetBadge()
        {
            CheckInitialized();
            NotSupported();
            return 0;
        }

        public override void SetBadge(int bandgeNumber)
        {
            CheckInitialized();
            NotSupported();
        }
    }
}
#endif