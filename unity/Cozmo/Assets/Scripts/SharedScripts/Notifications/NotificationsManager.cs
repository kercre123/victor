using System;
using Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using DataPersistence;

namespace Cozmo.Notifications {

  public class Notification {
    public int SecondsInFuture { get; private set; }
    public string TextKey { get; private set; }
    public bool Persist { get; private set; }
    public string OrderId;
    public UInt32 TimeToSend;
    public UInt32 TimeClicked;

    public Notification(int secondsInFuture, string textKey, bool persist) {
      SecondsInFuture = secondsInFuture;
      TextKey = textKey;
      Persist = persist;
      OrderId = "";
      TimeToSend = 0;
      TimeClicked = 0;
    }
  }

  public class NotificationsManager {
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
    private static extern void AnkiNotifications_ChangeAppBoyUserId(string playerId);

    [DllImport("__Internal")]
    private static extern void AnkiNotifications_RequestPermission();

    [DllImport("__Internal")]
    private static extern void AnkiNotifications_SetAppBoyUserDOB(string dob);

    [DllImport("__Internal")]
    private static extern void AnkiNotifications_SetCustomAttribute(string attributeKey, string attributeValue);

    [DllImport("__Internal")]
    private static extern void AnkiNotifications_AddAppBoyAlias(string alias, string label);

    [DllImport("__Internal")]
    private static extern bool AnkiNotifications_IsInitialized();

    [DllImport("__Internal")]
    private static extern void AnkiNotifications_SetRequestProcessingPolicy(bool automatic);
#endif

    private const string _kOrderIdNotificationDataKey = "order_id";
    private const string _kTimeSentNotificationDataKey = "time_sent";
    private const string _kMessageKeyNotificationDataKey = "message_key";
    private const string _kNotificationClickDasEvent = "app.notification.click";
    private const string _kNotificationSendDasEvent = "app.notification.send";
    private const string _kTimeStampDasEventDataKey = "$ts";
    private const string _kDataDasEventDataKey = "$data";
    private const UInt32 _kNotifClickGetsCreditWindowInSeconds = 30 * 60;

    #region Push Notification Consts
    private const string _kBrazePlayerIdAttributeKey = "player_id";
    private const string _kDasEventBase = "ui.push_notifications.";
    private const string _kDasEventReceived = "received";
    private const string _kDasEventReaction = "reaction";
    #endregion

    private static DateTime _sEpochZero = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);

    public static NotificationsManager Instance { get; private set; }

    public static void CreateInstance() {
      Instance = new NotificationsManager();
    }

    private List<Notification> _NotificationCache;

    private NotificationsManager() {
      _NotificationCache = new List<Notification>();

      UTNotifications.Manager.Instance.OnNotificationClicked += HandleNotificationClicked;

      // Since initalizing will pop up permissions on iOS, don't allow until our preprompt has been shown.
      InitNotificationsIfAllowed();

      RobotEngineManager.Instance.AddCallback<ClearNotificationCache>(HandleClearNotificationCache);
      RobotEngineManager.Instance.AddCallback<CacheNotificationToSchedule>(HandleCacheNotificationToSchedule);
    }

    public void Init() {
      Cozmo.PauseManager.Instance.OnPauseStateChanged += HandlePauseStateChanged;
      RobotEngineManager.Instance.SendNotificationsManagerReady();

      LogDasEventsForSentNotifications();
      ClearNotificationsToBeSentRecords();
    }

    public void InitNotificationsIfAllowed() {
      // on platforms that don't require a prompt ( android ) this will always be true.
      // on platforms that require a prompt ( iOS ) this is set by either our settings menu notification panel, 
      //              or first time onboarding, or the weekly repeat of that same onboarding.
      if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.OSNotificationsPermissionsPromptShown) {
#if UNITY_IOS && !UNITY_EDITOR
        // We want the AnkiNotifications lib to handle requesting the notification permission on iOS - 
        // it will update the status with the Braze SDK
        AnkiNotifications_RequestPermission();

        // On iOS we can't clear the notifications immediately and log them for DAS, so wait awhile for a startup
        UTNotifications.Manager.Instance.StartCoroutine(DelayCancelAllNotifications());
#else
        CancelAllNotifications();
#endif
        // Always init UTNotifications w/o having the lib request permissions
        UTNotifications.Manager.Instance.Initialize(false);
      }
    }

    private System.Collections.IEnumerator DelayCancelAllNotifications(float waitTime = 0.1f) {
      yield return new UnityEngine.WaitForSeconds(waitTime);
      CancelAllNotifications();
    }

    private UInt32 DateTimeToEpochTimestamp(DateTime dateTime) {
      return Convert.ToUInt32((dateTime - _sEpochZero).TotalSeconds);
    }

    private void HandlePauseStateChanged(bool isPaused) {
      if (isPaused) {
        ScheduleAllNotifications();
      }
      else {
        LogDasEventsForSentNotifications();
        ClearNotificationsToBeSentRecords();
        ClearNotificationCache(); // notifications not-yet-delivered (or scheduled)
      }
    }

    private void HandleClearNotificationCache(ClearNotificationCache notificationMessage) {
      // Remove all except those marked to persist
      _NotificationCache.RemoveAll(x => !x.Persist);
    }

    public void ClearNotificationCache() {
      _NotificationCache.Clear();
    }

    public void HandleCacheNotificationToSchedule(CacheNotificationToSchedule notificationMessage) {
      _NotificationCache.Add(new Notification(notificationMessage.secondsInFuture, notificationMessage.textKey,
                                              notificationMessage.persist));
    }

    private void CancelAllNotifications() {
      if (UTNotifications.Manager.Instance.IsInitialized()) {
        DAS.Info("NotificationsManager.CancelAllNotifications", "");
        UTNotifications.Manager.Instance.CancelAllNotifications();
      }
    }

    private void ScheduleAllNotifications() {
      if (!UTNotifications.Manager.Instance.IsInitialized()) {
        return;
      }
      CancelAllNotifications();

      _NotificationCache.Sort((a, b) => (a.SecondsInFuture.CompareTo(b.SecondsInFuture)));

      int orderId = 0;
      foreach (var notification in _NotificationCache) {
        DateTime timeToSend = DateTime.UtcNow.AddSeconds(notification.SecondsInFuture);
        notification.TimeToSend = DateTimeToEpochTimestamp(timeToSend);
        notification.OrderId = orderId.ToString();

        Dictionary<string, string> notifData = new Dictionary<string, string>(){
          {_kOrderIdNotificationDataKey, notification.OrderId},
          {_kTimeSentNotificationDataKey, notification.TimeToSend.ToString()},
          {_kMessageKeyNotificationDataKey, notification.TextKey}
        };

        UTNotifications.Manager.Instance.ScheduleNotification(
          notification.SecondsInFuture,
#if UNITY_ANDROID && !UNITY_EDITOR
          "Cozmo",
#else
          // On iOS, the title does not look so good (COZMO-14564)
          string.Empty,
#endif
          Localization.GetWithArgs(notification.TextKey),
          orderId,
          notifData
        );

        orderId++;
      }

      RecordNotificationsToBeSent();
    }

    private void RecordNotificationsToBeSent() {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.NotificationsToBeSent = _NotificationCache;
      DataPersistence.DataPersistenceManager.Instance.Save();
    }

    private void LogDasEventsForSentNotifications() {
      List<Notification> possiblySentNotifications = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.NotificationsToBeSent;

      UInt32 now = DateTimeToEpochTimestamp(DateTime.UtcNow);
      foreach (var notification in possiblySentNotifications) {
        if (notification.TimeToSend < now) {
          var dataDict = DasTracker.GetDataDictionary(
            _kTimeStampDasEventDataKey, notification.TimeToSend.ToString(),
            _kDataDasEventDataKey, notification.TextKey);

          DAS.Event(_kNotificationSendDasEvent, notification.OrderId.ToString(), dataDict);
        }
      }
    }

    private void ClearNotificationsToBeSentRecords() {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.NotificationsToBeSent.Clear();
    }

    private void HandleNotificationClicked(UTNotifications.ReceivedNotification notification) {
      string orderId = notification.userData[_kOrderIdNotificationDataKey];
      string timeSent = notification.userData[_kTimeSentNotificationDataKey];
      string messageKey = notification.userData[_kMessageKeyNotificationDataKey];
      UInt32 timeClicked = DateTimeToEpochTimestamp(DateTime.UtcNow);

      Dictionary<string, string> dasData = new Dictionary<string, string>() {
        { _kTimeStampDasEventDataKey, timeClicked.ToString() },
        { _kDataDasEventDataKey, messageKey + ":" + timeSent }
      };
      DAS.Event(_kNotificationClickDasEvent, orderId, dasData);

      // save clicked notification to give credit on robot connect success
      Notification clickedNotif = new Notification(0, messageKey, false);
      clickedNotif.OrderId = orderId;
      clickedNotif.TimeClicked = timeClicked;
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.MostRecentNotificationClicked = clickedNotif;
      CancelAllNotifications();
    }

    public Notification GetNotificationClickedForSession() {
      Notification notifClicked = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.MostRecentNotificationClicked;
      if (notifClicked == null) {
        return null;
      }

      UInt32 secondsSinceClick = DateTimeToEpochTimestamp(DateTime.UtcNow) - notifClicked.TimeClicked;
      if (secondsSinceClick > _kNotifClickGetsCreditWindowInSeconds) {
        return null;
      }

      return notifClicked;
    }

    public void CreditNotificationForConnect() {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.MostRecentNotificationClicked = null;
    }

    // Should only be called on iOS
    public void OnNotificationPermissionGranted() {
      // On iOS just needing a callback for a click counts as "needing to handle" notifications.
      // This every frame process is required for proper DAS logging
      DAS.Info("NotificationsManager", "OnNotificationPermissionGranted");
      UTNotifications.Manager.Instance.Initialize(true);

      // Make sure we've set the user data
      SetBrazeData();
    }

    public void SetBrazeData() {
#if UNITY_IOS && !UNITY_EDITOR
      // Use Robot ID as a unique identifier
      AnkiNotifications_ChangeAppBoyUserId(DataPersistenceManager.Instance.Data.DefaultProfile.RobotPhysicalId);

      // Set birthdate - guaranteed to be available by the time NeedsHubView loads
      string dob = DataPersistenceManager.Instance.Data.DefaultProfile.Birthdate.ToString("yyyyMMdd");
      AnkiNotifications_SetAppBoyUserDOB(dob);

      // Now using Player ID as a custom attr
      AnkiNotifications_SetCustomAttribute(_kBrazePlayerIdAttributeKey, DataPersistenceManager.Instance.Data.DefaultProfile.PlayerId);
#endif
    }

    public void OnNotificationReceived(JSONObject notificationJson) {
      DAS.Event(_kDasEventBase + _kDasEventReceived, getNotificationId(notificationJson));
    }

    public void OnNotificationOpened(JSONObject notificationJson) {
      DAS.Event(_kDasEventBase + _kDasEventReaction, getNotificationId(notificationJson));
    }

    private string getNotificationId(JSONObject json) {
      string id = "-1";
      if (json.HasField("id")) {
        id = json.GetField("id").str;
      }

      return id;
    }

    public void SetBrazeRequestProcessingPolicy(bool automatic) {
#if UNITY_IOS && !UNITY_EDITOR
      DAS.Info("NotificationManager", "Setting Braze Request Processing Policy to " + (automatic ? "automatic" : "manual"));
      AnkiNotifications_SetRequestProcessingPolicy(automatic);
#endif
    }

    public bool NotificationsEnabled() {
      return DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.OSNotificationsPermissionsPromptShown;
    }
  }
}