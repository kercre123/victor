#if !UNITY_EDITOR && UNITY_IOS

//Detect the version of Untiy
#define UNITY_3_PLUS
#define UNITY_4_PLUS
#define UNITY_4_6_PLUS
#define UNITY_5_PLUS
#if UNITY_2_6
    #define UNITY_2_X
    #undef UNITY_3_PLUS
    #undef UNITY_4_PLUS
    #undef UNITY_4_6_PLUS
    #undef UNITY_5_PLUS
#elif UNITY_3_0 || UNITY_3_1 || UNITY_3_2 || UNITY_3_3 || UNITY_3_4 || UNITY_3_5
    #define UNITY_3_X
    #undef UNITY_4_6_PLUS
    #undef UNITY_4_PLUS
    #undef UNITY_5_PLUS
#elif UNITY_4_0 || UNITY_4_1 || UNITY_4_2 || UNITY_4_3 || UNITY_4_4 || UNITY_4_5 || UNITY_4_6 || UNITY_4_7 || UNITY_4_8 || UNITY_4_9
    #define UNITY_4_X
    #undef UNITY_5_PLUS
    #if UNITY_4_0 || UNITY_4_1 || UNITY_4_2 || UNITY_4_3 || UNITY_4_4 || UNITY_4_5
        #undef UNITY_4_6_PLUS
    #endif
#elif UNITY_5_0 || UNITY_5_1 || UNITY_5_2 || UNITY_5_3 || UNITY_5_4 || UNITY_5_5 || UNITY_5_6 || UNITY_5_7 || UNITY_5_8 || UNITY_5_9
    #define UNITY_5_X
#endif

#if UNITY_5_PLUS
using UnityEngine.iOS;
#else
using UnityEngine;
#endif
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System;
using System.IO;

namespace UTNotifications
{
    public class ManagerImpl : Manager
    {
    //public
        public override bool Initialize(bool willHandleReceivedNotifications, int startId = 0, bool incrementalId = false)
        {
            m_initialized = true;
            
            LoadScheduledNotificationsWhenDisabled();

            m_pushEnabled = UnityEngine.PlayerPrefs.GetInt(m_pushEnabledOptionName, 1) != 0;
            SetNotificationsEnabled(UnityEngine.PlayerPrefs.GetInt(m_enabledOptionName, 1) != 0);

            m_willHandleReceivedNotifications = willHandleReceivedNotifications;
            m_nextPushNotificationId = startId;
            m_incrementalId = incrementalId;

            return true;
        }

        public override void PostLocalNotification(string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            if (!CheckInitialized())
            {
                return;
            }
            
            if (m_enabled)
            {
                NotificationServices.PresentLocalNotificationNow(CreateLocalNotification(title, text, id, userData, notificationProfile, badgeNumber));
            }
        }

        public override void ScheduleNotification(int triggerInSeconds, string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            if (!CheckInitialized())
            {
                return;
            }
            
            LocalNotification notification = CreateLocalNotification(title, text, id, userData, notificationProfile, badgeNumber);
            notification.fireDate = System.DateTime.Now.AddSeconds(triggerInSeconds);
            if (m_enabled)
            {
                NotificationServices.ScheduleLocalNotification(notification);
            }
            else
            {
                if (m_scheduledNotificationsWhenDisabled == null)
                {
                    m_scheduledNotificationsWhenDisabled = new List<ScheduledNotification>();
                }
                m_scheduledNotificationsWhenDisabled.Add(ToScheduledNotification(notification, DateTime.Now));
                SaveScheduledNotificationsWhenDisabled();
            }
        }

        //Please note that the actual interval may be different.
        //On iOS there are only fixed options like every minute, every day, every week and so on. So the provided <c>intervalSeconds</c> value will be approximated by one of the available options.
        public override void ScheduleNotificationRepeating(int firstTriggerInSeconds, int intervalSeconds, string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            if (!CheckInitialized())
            {
                return;
            }
            
            LocalNotification notification = CreateLocalNotification(title, text, id, userData, notificationProfile, badgeNumber);
            notification.fireDate = System.DateTime.Now.AddSeconds(firstTriggerInSeconds);
            if (intervalSeconds < 3)
            {
                //Approximate it to every second if the desired intervalSeconds is less then 3 seconds
                notification.repeatInterval = CalendarUnit.Second;
            }
            else if (intervalSeconds < 3 * 60)
            {
                //Approximate it to every minute if the desired intervalSeconds is less then 3 minutes
                notification.repeatInterval = CalendarUnit.Minute;
            }
            else if (intervalSeconds < 3 * 60 * 60)
            {
                //Approximate it to every hour if the desired intervalSeconds is less then 3 hours
                notification.repeatInterval = CalendarUnit.Hour;
            }
            else if (intervalSeconds < 3 * 24 * 60 * 60)
            {
                //Approximate it to every day if the desired intervalSeconds is less then 3 days
                notification.repeatInterval = CalendarUnit.Day;
            }
            else if (intervalSeconds < 3 * 7 * 24 * 60 * 60)
            {
                //Approximate it to every week if the desired intervalSeconds is less then 3 weeks
                notification.repeatInterval = CalendarUnit.Week;
            }
            else if (intervalSeconds < 2 * 31 * 24 * 60 * 60)
            {
                //Approximate it to every month if the desired intervalSeconds is less then 2 months
                notification.repeatInterval = CalendarUnit.Month;
            }
            else if (intervalSeconds < 183 * 24 * 60 * 60)
            {
                //Approximate it to every quarter if the desired intervalSeconds is less then half a year
                notification.repeatInterval = CalendarUnit.Quarter;
            }
            else if (intervalSeconds < 3 * 365 * 24 * 60 * 60)
            {
                //Approximate it to every year if the desired intervalSeconds is less then 3 years
                notification.repeatInterval = CalendarUnit.Year;
            }
            else
            {
                UnityEngine.Debug.LogWarning("Suspicious intervalSeconds value provided: " + intervalSeconds);
                notification.repeatInterval = CalendarUnit.Era;
            }

            if (m_enabled)
            {
                NotificationServices.ScheduleLocalNotification(notification);
            }
            else
            {
                if (m_scheduledNotificationsWhenDisabled == null)
                {
                    m_scheduledNotificationsWhenDisabled = new List<ScheduledNotification>();
                }
                m_scheduledNotificationsWhenDisabled.Add(ToScheduledNotification(notification, DateTime.Now));
                SaveScheduledNotificationsWhenDisabled();
            }
        }

        public override bool NotificationsEnabled()
        {
            if (m_initialized)
            {
                return m_enabled;
            }
            else
            {
                return UnityEngine.PlayerPrefs.GetInt(m_enabledOptionName, 1) != 0;
            }
        }

        public override bool NotificationsAllowed()
        {
            return _UT_NotificationsAllowed();
        }

        public override void SetNotificationsEnabled(bool enabled)
        {
            UnityEngine.PlayerPrefs.SetInt(m_enabledOptionName, enabled ? 1 : 0);

            if (!m_initialized)
            {
                if (!enabled)
                {
                    NotificationServices.UnregisterForRemoteNotifications();
                }

                return;
            }

            if (m_enabled == enabled)
            {
                return;
            }

            m_enabled = enabled;

            if (enabled)
            {
                RegisterForNotifications();

                if (m_scheduledNotificationsWhenDisabled != null)
                {
                    DateTime now = DateTime.Now.ToUniversalTime();

                    foreach (var scheduledNotification in m_scheduledNotificationsWhenDisabled)
                    {
                        LocalNotification notification = new LocalNotification();
                        if (scheduledNotification.badgeNumber >= 0)
                        {
                            notification.applicationIconBadgeNumber = scheduledNotification.badgeNumber;
                        }
                        notification.alertAction = scheduledNotification.alertAction;
                        notification.alertBody = scheduledNotification.alertBody;
                        notification.hasAction = true;
                        notification.soundName = scheduledNotification.soundName;
                        notification.userInfo = (scheduledNotification.userData != null) ? scheduledNotification.userData : new Dictionary<string, string>();
                        if (scheduledNotification.fireDate > now)
                        {
                            notification.fireDate = scheduledNotification.fireDate.ToLocalTime();
                        }
                        else if (scheduledNotification.fireDate != DateTime.MinValue)
                        {
                            notification.fireDate = DateTime.Now.AddSeconds(2);
                        }
                        else
                        {
                            notification.fireDate = RepeatIntervalToDateTime(scheduledNotification.repeatInterval);
                        }
                        notification.repeatInterval = scheduledNotification.repeatInterval;

                        NotificationServices.ScheduleLocalNotification(notification);
                    }

                    m_scheduledNotificationsWhenDisabled = null;
                    SaveScheduledNotificationsWhenDisabled();
                }

                SetBadge(GetBadge());
            }
            else
            {
                List<ScheduledNotification> scheduledNotificationsWhenDisabled;
                if (NotificationServices.scheduledLocalNotifications != null && NotificationServices.scheduledLocalNotifications.Length != 0)
                {
                    scheduledNotificationsWhenDisabled = new List<ScheduledNotification>(NotificationServices.scheduledLocalNotifications.Length);

                    DateTime now = DateTime.Now;

                    foreach (var notification in NotificationServices.scheduledLocalNotifications)
                    {
                        scheduledNotificationsWhenDisabled.Add(ToScheduledNotification(notification, now));
                    }
                }
                else
                {
                    scheduledNotificationsWhenDisabled = null;
                }

                CancelAllNotifications();
                NotificationServices.UnregisterForRemoteNotifications();

                if (scheduledNotificationsWhenDisabled != null)
                {
                    m_scheduledNotificationsWhenDisabled = scheduledNotificationsWhenDisabled;
                    SaveScheduledNotificationsWhenDisabled();
                }

                _UT_SetIconBadgeNumber(0);
            }
        }

        public override bool PushNotificationsEnabled()
        {
            bool enabled;
            if (m_initialized)
            {
                enabled = m_pushEnabled;
            }
            else
            {
                enabled = UnityEngine.PlayerPrefs.GetInt(m_pushEnabledOptionName, 1) != 0;
            }

            return enabled && Settings.Instance.PushNotificationsEnabledIOS && NotificationsEnabled();
        }

        public override bool SetPushNotificationsEnabled(bool enabled)
        {
            // Should be done before (enable == m_pushEnabled) check in order to disable push notifications before Manager initialization
            UnityEngine.PlayerPrefs.SetInt(m_pushEnabledOptionName, enabled ? 1 : 0);

            if (enabled && !Settings.Instance.PushNotificationsEnabledIOS)
            {
                UnityEngine.Debug.LogWarning("Can't enable push notifications: iOS -> Push Notifications are disabled in the UTNotifications Settings");
            }
            
            if (!m_initialized || enabled == m_pushEnabled)
            {
                return PushNotificationsEnabled();
            }
            
            m_pushEnabled = enabled;

            if (m_enabled && Settings.Instance.PushNotificationsEnabledIOS)
            {
                if (m_pushEnabled)
                {
                    RegisterForNotifications();
                }
                else
                {
                    NotificationServices.UnregisterForRemoteNotifications();
                }
            }

            return PushNotificationsEnabled();
        }

        public override void CancelNotification(int id)
        {
            if (!CheckInitialized())
            {
                return;
            }

            string idAsString = id.ToString();

            if (NotificationServices.localNotifications != null)
            {
                foreach (var notification in NotificationServices.localNotifications)
                {
                    if (notification.userInfo.Contains(m_idKeyName) && (string)notification.userInfo[m_idKeyName] == idAsString)
                    {
                        NotificationServices.CancelLocalNotification(notification);
                        return;
                    }
                }
            }

            if (NotificationServices.scheduledLocalNotifications != null)
            {
                foreach (var notification in NotificationServices.scheduledLocalNotifications)
                {
                    if (notification.userInfo.Contains(m_idKeyName) && (string)notification.userInfo[m_idKeyName] == idAsString)
                    {
                        NotificationServices.CancelLocalNotification(notification);
                        return;
                    }
                }
            }

            if (m_scheduledNotificationsWhenDisabled != null)
            {
                bool found = false;
                for (int i = m_scheduledNotificationsWhenDisabled.Count - 1; i >= 0; --i)
                {
                    var notification = m_scheduledNotificationsWhenDisabled[i];
                    if (notification.userData.ContainsKey(m_idKeyName) && notification.userData[m_idKeyName] == idAsString)
                    {
                        found = true;
                        m_scheduledNotificationsWhenDisabled.RemoveAt(i);
                    }
                }

                if (found)
                {
                    SaveScheduledNotificationsWhenDisabled();
                }
            }
        }

        public override void HideNotification(int id)
        {
            if (!CheckInitialized())
            {
                return;
            }
            
            if (NotificationServices.localNotifications == null)
            {
                return;
            }

            string idAsString = id.ToString();

            foreach (var notification in NotificationServices.localNotifications)
            {
                if (notification.userInfo.Contains(m_idKeyName) && (string)notification.userInfo[m_idKeyName] == idAsString)
                {
                    HideNotification(notification);
                    return;
                }
            }
        }

        public override void HideAllNotifications()
        {
            if (!CheckInitialized())
            {
                return;
            }
            
            if (NotificationServices.localNotifications != null)
            {
                foreach (var notification in NotificationServices.localNotifications)
                {
                    HideNotification(notification);
                }
                NotificationServices.ClearLocalNotifications();
            }

            if (NotificationServices.remoteNotificationCount > 0)
            {
                NotificationServices.ClearRemoteNotifications();
            }

            _UT_HideAllNotifications();
        }

        public override void CancelAllNotifications()
        {
            if (!CheckInitialized())
            {
                return;
            }
            
            HideAllNotifications();
            NotificationServices.CancelAllLocalNotifications();
            m_scheduledNotificationsWhenDisabled = null;
            SaveScheduledNotificationsWhenDisabled();
        }

        public override int GetBadge()
        {
            if (!CheckInitialized())
            {
                return 0;
            }
            
            return Math.Max(UnityEngine.PlayerPrefs.GetInt(m_badgeOptionName, 0), _UT_GetIconBadgeNumber());
        }

        public override void SetBadge(int bandgeNumber)
        {
            if (!CheckInitialized())
            {
                return;
            }
            
            if (m_enabled)
            {
                _UT_SetIconBadgeNumber(bandgeNumber);
            }

            UnityEngine.PlayerPrefs.SetInt(m_badgeOptionName, bandgeNumber);
        }

    //protected
        protected void LateUpdate()
        {
            if (m_initRemoteNotifications)
            {
                if (!string.IsNullOrEmpty(NotificationServices.registrationError))
                {
                    m_initRemoteNotifications = false;
                    UnityEngine.Debug.LogError("iOS Push Notifications initialization error: " + NotificationServices.registrationError);
                }
                else if (NotificationServices.deviceToken != null && NotificationServices.deviceToken.Length > 0 && OnSendRegistrationIdHasSubscribers())
                {
                    m_initRemoteNotifications = false;
                    _OnSendRegistrationId(m_providerName, EncodeRegistrationId(NotificationServices.deviceToken));
                }
            }

            //Handle received/clicked notifications
            int localCount = (NotificationServices.localNotifications != null) ? NotificationServices.localNotifications.Length : 0;
            int remoteCount = (NotificationServices.remoteNotifications != null) ? NotificationServices.remoteNotifications.Length : 0;

            List<ReceivedNotification> list;
            if ((localCount > 0 || remoteCount > 0) && m_willHandleReceivedNotifications && OnNotificationsReceivedHasSubscribers())
            {
                list = new List<ReceivedNotification>(localCount + remoteCount);
            }
            else
            {
                list = null;
            }

            if (list == null && !OnNotificationClickedHasSubscribers())
            {
                return;
            }

            if (localCount > 0)
            {
                foreach (var notification in NotificationServices.localNotifications)
                {
                    ReceivedNotification receivedNotification = null;

                    //Check if the notification was clicked
                    if (OnNotificationClickedHasSubscribers())
                    {
                        int id = ExtractId(notification.userInfo);

                        if (_UT_LocalNotificationWasClicked(id))
                        {
                            receivedNotification = ToReceivedNotification(notification);
                            _OnNotificationClicked(receivedNotification);

                            if (list == null)
                            {
                                break;
                            }
                        }
                    }
                    
                    if (list != null)
                    {
                        if (receivedNotification == null)
                        {
                            receivedNotification = ToReceivedNotification(notification);
                        }

                        list.Add(receivedNotification);
                    }
                }
            }

            if (remoteCount > 0)
            {
                foreach (var notification in NotificationServices.remoteNotifications)
                {
                    ReceivedNotification receivedNotification = null;

                    //Check if the notification was clicked
                    if (OnNotificationClickedHasSubscribers() && _UT_PushNotificationWasClicked(notification.alertBody))
                    {
                        receivedNotification = ToReceivedNotification(notification);
                        _OnNotificationClicked(receivedNotification);

                        if (list == null)
                        {
                            break;
                        }
                    }

                    if (list != null)
                    {
                        if (receivedNotification == null)
                        {
                            receivedNotification = ToReceivedNotification(notification);
                        }

                        list.Add(receivedNotification);
                    }
                }
            }

            if (list != null && list.Count > 0)
            {
                HideAllNotifications();
                _OnNotificationsReceived(list);
            }
        }

    //private
        private ReceivedNotification ToReceivedNotification(LocalNotification notification)
        {
            Dictionary<string, string> userData = UserInfoToDictionaryOfStrings(notification.userInfo);

            if (userData != null && userData.ContainsKey(m_idKeyName))
            {
                userData.Remove(m_idKeyName);
            }

            string title, text;
            ExtractTitleAndText(notification.alertBody, out title, out text);

            return new ReceivedNotification(title, text, ExtractId(notification.userInfo), userData, ExtractNotificationProfile(notification.soundName), notification.applicationIconBadgeNumber);
        }

        private ReceivedNotification ToReceivedNotification(RemoteNotification notification)
        {
            Dictionary<string, string> userData = UserInfoToDictionaryOfStrings(notification.userInfo);

            if (userData != null && userData.ContainsKey(m_idKeyName))
            {
                userData.Remove(m_idKeyName);
            }

            int id = ExtractId(notification.userInfo);

            string title, text;
            ExtractTitleAndText(notification.alertBody, out title, out text);

            ReceivedNotification receivedNotification = new ReceivedNotification(title, text, id >= 0 ? id : m_nextPushNotificationId, userData, ExtractNotificationProfile(notification.soundName), notification.applicationIconBadgeNumber);

            if (id < 0 && m_incrementalId)
            {
                ++m_nextPushNotificationId;
            }

            return receivedNotification;
        }

        private LocalNotification CreateLocalNotification(string title, string text, int id, IDictionary<string, string> userData, string notificationProfile, int badgeNumber)
        {
            CancelNotification(id);

            LocalNotification notification = new LocalNotification();
            if (badgeNumber >= 0)
            {
                notification.applicationIconBadgeNumber = badgeNumber;
            }
            notification.alertAction = title;
            notification.alertBody = string.IsNullOrEmpty(title) ? text : title + "\n" + text;
            notification.hasAction = true;
            if (!string.IsNullOrEmpty(notificationProfile))
            {
                notification.soundName = m_soundNamePrefix + notificationProfile;
            }
            else
            {
                notification.soundName = LocalNotification.defaultSoundName;
            }
            var userInfo = (userData != null) ? new Dictionary<string, string>(userData) : new Dictionary<string, string>();
            userInfo.Add(m_idKeyName, id.ToString());
            notification.userInfo = userInfo;

            return notification;
        }

        private void HideNotification(LocalNotification notification)
        {
            if (notification.repeatInterval > CalendarUnit.Era)
            {
                LocalNotification replacementNotification = new LocalNotification();
                replacementNotification.applicationIconBadgeNumber = notification.applicationIconBadgeNumber;
                replacementNotification.alertAction = notification.alertAction;
                replacementNotification.alertBody = notification.alertBody;
                replacementNotification.repeatInterval = notification.repeatInterval;
                replacementNotification.hasAction = notification.hasAction;
                replacementNotification.soundName = notification.soundName;
                replacementNotification.userInfo = notification.userInfo;
                replacementNotification.fireDate = RepeatIntervalToDateTime(notification.repeatInterval);
                
                NotificationServices.ScheduleLocalNotification(replacementNotification);
            }
            
            NotificationServices.CancelLocalNotification(notification);
        }

        private class ScheduledNotification
        {
            public void Read(BinaryReader stream)
            {
                alertAction = stream.ReadString();
                alertBody = stream.ReadString();
                long fd = stream.ReadInt64();
                fireDate = (fd != 0) ? DateTime.FromBinary(fd) : DateTime.MinValue;
                repeatInterval = (CalendarUnit)stream.ReadInt32();
                soundName = stream.ReadString();
                int dictSize = stream.ReadInt32();
                userData = new Dictionary<string, string>(dictSize);
                for (int i = 0; i< dictSize; ++i)
                {
                    string key = stream.ReadString();
                    string value = stream.ReadString();
                    userData.Add(key, value);
                }
                try
                {
                    badgeNumber = stream.ReadInt32();
                }
                catch (EndOfStreamException)
                {
                    //Backward compatibility
                    badgeNumber = -1;
                }
            }

            public void Write(BinaryWriter stream)
            {
                stream.Write(alertAction);
                stream.Write(alertBody);
                stream.Write((fireDate == DateTime.MinValue) ? 0 : fireDate.ToBinary());
                stream.Write((int)repeatInterval);
                stream.Write(soundName);
                stream.Write(userData.Count);
                foreach (var entry in userData)
                {
                    stream.Write(entry.Key);
                    stream.Write(entry.Value);
                }
                stream.Write(badgeNumber);
            }

            public string alertAction;
            public string alertBody;
            public DateTime fireDate;
            public CalendarUnit repeatInterval;
            public string soundName;
            public Dictionary<string, string> userData;
            public int badgeNumber;
        }

        private Dictionary<string, string> UserInfoToDictionaryOfStrings(IDictionary userInfo)
        {
            Dictionary<string, string> userData = new Dictionary<string, string>();
            foreach (var key in userInfo.Keys)
            {
                if (key is string)
                {
                    object value = userInfo[key];
                    if (value is string)
                    {
                        userData.Add((string)key, (string)value);
                    }
                }
            }

            return userData;
        }

        private ScheduledNotification ToScheduledNotification(LocalNotification notification, DateTime now)
        {
            ScheduledNotification scheduledNotification = new ScheduledNotification();
            scheduledNotification.badgeNumber = notification.applicationIconBadgeNumber;
            scheduledNotification.alertAction = notification.alertAction;
            scheduledNotification.alertBody = notification.alertBody;
            if (notification.fireDate > now.ToUniversalTime())
            {
                scheduledNotification.fireDate = notification.fireDate;
            }
            else
            {
                scheduledNotification.fireDate = DateTime.MinValue;
            }
            scheduledNotification.repeatInterval = notification.repeatInterval;
            scheduledNotification.userData = UserInfoToDictionaryOfStrings(notification.userInfo);
            scheduledNotification.soundName = notification.soundName;
        
            return scheduledNotification;
        }

        private void SaveScheduledNotificationsWhenDisabled()
        {
            string fileName = GetSerializationFileName();
            if (m_scheduledNotificationsWhenDisabled != null && m_scheduledNotificationsWhenDisabled.Count > 0)
            {
                try
                {
                    FileStream file = File.Create(fileName);
                    using (BinaryWriter stream = new BinaryWriter(file))
                    {
                        stream.Write(m_scheduledNotificationsWhenDisabled.Count);
                        for (int i = 0; i < m_scheduledNotificationsWhenDisabled.Count; ++i)
                        {
                            m_scheduledNotificationsWhenDisabled[i].Write(stream);
                        }
                    }
                    file.Close();
                }
                catch (Exception e)
                {
                    UnityEngine.Debug.LogException(e);
                }
            }
            else
            {
                File.Delete(fileName);
            }
        }

        private void LoadScheduledNotificationsWhenDisabled()
        {
            string fileName = GetSerializationFileName();
            if (File.Exists(fileName))
            {
                try
                {
                    FileStream file = File.OpenRead(fileName);
                    using (BinaryReader stream = new BinaryReader(file))
                    {
                        int count = stream.ReadInt32();
                        m_scheduledNotificationsWhenDisabled = new List<ScheduledNotification>(count);
                        for (int i = 0; i < count; ++i)
                        {
                            ScheduledNotification scheduledNotification = new ScheduledNotification();
                            scheduledNotification.Read(stream);
                            m_scheduledNotificationsWhenDisabled.Add(scheduledNotification);
                        }
                    }
                    file.Close();
                }
                catch (Exception e)
                {
                    UnityEngine.Debug.LogException(e);
                }
            }
            else
            {
                m_scheduledNotificationsWhenDisabled = null;
            }
        }

        private string GetSerializationFileName()
        {
            return UnityEngine.Application.persistentDataPath + "/_ut_notifications_snwd.bin";
        }

        private DateTime RepeatIntervalToDateTime(CalendarUnit repeatInterval)
        {
            switch (repeatInterval)
            {
            case CalendarUnit.Year:
                return System.DateTime.Now.AddYears(1);
                
            case CalendarUnit.Quarter:
                return System.DateTime.Now.AddMonths(3);
                
            case CalendarUnit.Month:
                return System.DateTime.Now.AddMonths(1);
                
            case CalendarUnit.Week:
                return System.DateTime.Now.AddDays(7);
                
            case CalendarUnit.Day:
                return System.DateTime.Now.AddDays(1);
                
            case CalendarUnit.Hour:
                return System.DateTime.Now.AddHours(1);
                
            case CalendarUnit.Minute:
                return System.DateTime.Now.AddMinutes(1);
                
            case CalendarUnit.Second:
                return System.DateTime.Now.AddSeconds(1);
                
            default:
                UnityEngine.Debug.LogWarning("Unexpected repeatInterval: " + repeatInterval);
                return System.DateTime.MaxValue;
            }
        }

        private void RegisterForNotifications()
        {
            m_initRemoteNotifications = Settings.Instance.PushNotificationsEnabledIOS && m_pushEnabled;

            //Workaround for a bug in some versions of Unity: http://forum.unity3d.com/threads/local-notification-not-working-in-ios-8-unity4-5-4-xcode6-0-1.271487/
            if (!_UT_RegisterForIOS8(m_initRemoteNotifications))
            {
#if UNITY_5_PLUS
                NotificationServices.RegisterForNotifications(NotificationType.Badge | NotificationType.Alert | NotificationType.Sound, m_initRemoteNotifications);
#else
    #if UNITY_4_6_PLUS
                NotificationServices.RegisterForLocalNotificationTypes(LocalNotificationType.Badge | LocalNotificationType.Alert | LocalNotificationType.Sound);
    #endif
                if (m_initRemoteNotifications)
                {
                    NotificationServices.RegisterForRemoteNotificationTypes(RemoteNotificationType.Badge | RemoteNotificationType.Alert | RemoteNotificationType.Sound);
                }
#endif
            }
        }

        private static int ExtractId(IDictionary userInfo)
        {
            return userInfo.Contains(m_idKeyName) ? int.Parse((string)userInfo[m_idKeyName]) : -1;
        }

        private static void ExtractTitleAndText(string alertBody, out string title, out string text)
        {
            if (alertBody == null)
            {
                title = "";
                text = "";
                return;
            }

            int endlinePos = alertBody.IndexOf('\n');
            if (endlinePos < 0)
            {
                title = "";
                text = alertBody;
            }
            else
            {
                title = alertBody.Substring(0, endlinePos);
                text = alertBody.Substring(endlinePos + 1);
            }
        }

        private static string ExtractNotificationProfile(string soundName)
        {
            if (soundName == LocalNotification.defaultSoundName)
            {
                return "";
            }
            else
            {
                return string.IsNullOrEmpty(soundName) ? "" : soundName.Replace(m_soundNamePrefix, "");
            }
        }

        private static string EncodeRegistrationId(byte[] token)
        {
            switch (Settings.Instance.IOSTokenEncoding)
            {
            case Settings.TokenEncoding.Base64:
                return System.Convert.ToBase64String(token);

            case Settings.TokenEncoding.HEX:
                return ByteArrayToHexString(token);

            default:
                throw new Exception("Unexpected value of Settings.Instance.IOSTokenEncoding: " + Settings.Instance.IOSTokenEncoding);
            }
        }

        private static string ByteArrayToHexString(byte[] byteArray)
        {
            string hex = BitConverter.ToString(byteArray);
            return hex.Replace("-", "").ToLower();
        }

        [DllImport("__Internal")]
        private static extern bool _UT_RegisterForIOS8(bool remote);

        [DllImport("__Internal")]
        private static extern int _UT_GetIconBadgeNumber();

        [DllImport("__Internal")]
        private static extern void _UT_SetIconBadgeNumber(int value);

        [DllImport("__Internal")]
        private static extern void _UT_HideAllNotifications();

        [DllImport("__Internal")]
        private static extern bool _UT_LocalNotificationWasClicked(int id);

        [DllImport("__Internal")]
        private static extern bool _UT_PushNotificationWasClicked(string body);

        [DllImport("__Internal")]
        private static extern bool _UT_NotificationsAllowed();

        private const string m_idKeyName = "id";
        private const string m_providerName = "APNS";
        private const string m_enabledOptionName = "_UT_NOTIFICATIONS_ENABLED";
        private const string m_pushEnabledOptionName = "_UT_NOTIFICATIONS_PUSH_ENABLED";
        private const string m_badgeOptionName = "_UT_NOTIFICATIONS_BADGE_VALUE";
        private const string m_soundNamePrefix = "Data/Raw/";
        private bool m_willHandleReceivedNotifications;
        private bool m_initRemoteNotifications;
        private int m_nextPushNotificationId;
        private bool m_incrementalId;
        private bool m_enabled = false;
        private bool m_pushEnabled = false;

        private List<ScheduledNotification> m_scheduledNotificationsWhenDisabled;
    }
}
#endif