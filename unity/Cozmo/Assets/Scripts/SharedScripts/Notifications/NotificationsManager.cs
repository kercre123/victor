using Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;

namespace Cozmo.Notifications {
  public class NotificationsManager {

    public static NotificationsManager Instance { get; private set; }

    public static void CreateInstance() {
      Instance = new NotificationsManager();
    }

    private struct Notification {
      public int SecondsInFuture { get; private set; }
      public string TextKey { get; private set; }

      public Notification(int secondsInFuture, string textKey) {
        SecondsInFuture = secondsInFuture;
        TextKey = textKey;
      }
    }

    private int _NotifId;
    private List<Notification> _NotificationCache;

    private NotificationsManager() {
      _NotifId = 0;
      _NotificationCache = new List<Notification>();

      UTNotifications.Manager.Instance.Initialize(false);
      CancelAllNotifications();

      RobotEngineManager.Instance.AddCallback<ClearNotificationCache>(HandleClearNotificationCache);
      RobotEngineManager.Instance.AddCallback<CacheNotificationToSchedule>(HandleCacheNotificationToSchedule);
    }

    public void Init() {
      Cozmo.PauseManager.Instance.OnPauseStateChanged += HandlePauseStateChanged;
      RobotEngineManager.Instance.SendNotificationsManagerReady();
    }

    private void HandlePauseStateChanged(bool isPaused) {
      if (isPaused) {
        ScheduleAllNotifications();
      }
    }

    private void HandleClearNotificationCache(ClearNotificationCache notificationMessage) {
      _NotificationCache.Clear();
    }

    private void HandleCacheNotificationToSchedule(CacheNotificationToSchedule notificationMessage) {
      _NotificationCache.Add(new Notification(notificationMessage.secondsInFuture, notificationMessage.textKey));
    }

    private void CancelAllNotifications() {
      UTNotifications.Manager.Instance.CancelAllNotifications();
    }

    private void ScheduleAllNotifications() {
      CancelAllNotifications();

      foreach (var notification in _NotificationCache) {
        UTNotifications.Manager.Instance.ScheduleNotification(
          notification.SecondsInFuture,
          Localization.GetWithArgs(notification.TextKey),
          "",
          _NotifId++
        );
      }
    }
  }
}