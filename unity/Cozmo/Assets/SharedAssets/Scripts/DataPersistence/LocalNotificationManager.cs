using UnityEngine;
using System.Collections;
using System;
using Cozmo.Util;
using System.Collections.Generic;


#if UNITY_IOS
using UnityEngine.iOS;
using LocalNotification = UnityEngine.iOS.LocalNotification;
using NotificationServices = UnityEngine.iOS.NotificationServices;
#endif

// TODO: Android!!!

// Our best bet is probably to buy this: 
// https://www.assetstore.unity3d.com/en/#!/content/45507


public class LocalNotificationManager : MonoBehaviour {

  [Serializable]
  public class NotificatationScheduler {
    public int DelayDays;
    public float EarliestHour;
    public float LatestHour;

    public List<string> LocalizationKeyOptions = new List<string>();
  }

  [HideInInspector]
  public List<NotificatationScheduler> NotificationsToSchedule = new List<NotificatationScheduler>();


  public static void CancelScheduledNotifications() {
#if UNITY_IOS
    NotificationServices.CancelAllLocalNotifications();
#endif
  }

  public static void ScheduleNotification(DateTime time, string message) {
#if UNITY_IOS
    LocalNotification notification = new LocalNotification() {
      fireDate = time,
      alertBody = message,
    };

    NotificationServices.ScheduleLocalNotification(notification);
#endif
  }

  // StartupManager initializes localization in Start
  public void Start() {
#if UNITY_IOS
    NotificationServices.RegisterForNotifications(NotificationType.Alert | NotificationType.Badge | NotificationType.Sound);
#endif

    CancelScheduledNotifications();

    if (NotificationsToSchedule != null) {      
      for (int i = 0; i < NotificationsToSchedule.Count; i++) {
        var notif = NotificationsToSchedule[i];

        if (notif != null && notif.LocalizationKeyOptions != null && notif.LocalizationKeyOptions.Count > 0) {
          
          float hour = UnityEngine.Random.Range(notif.EarliestHour, notif.LatestHour);

          DateTime time = DateTime.Now.Date.AddDays(notif.DelayDays).AddHours(hour);

          var message = Localization.Get(notif.LocalizationKeyOptions[UnityEngine.Random.Range(0, notif.LocalizationKeyOptions.Count)]);

          ScheduleNotification(time, message);
        }
      }
    }
  }
}
