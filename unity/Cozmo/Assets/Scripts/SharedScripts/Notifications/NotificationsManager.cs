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
      public bool Persist { get; private set; }

      public Notification(int secondsInFuture, string textKey, bool persist) {
        SecondsInFuture = secondsInFuture;
        TextKey = textKey;
        Persist = persist;
      }
    }

    private int _NotifId;
    private List<Notification> _NotificationCache;

    private NotificationsManager() {
      _NotifId = 0;
      _NotificationCache = new List<Notification>();

      // Since initalizing will pop up permissions on iOS, don't allow until our preprompt has been shown.
      InitNotificationsIfAllowed();
      CancelAllNotifications();

      RobotEngineManager.Instance.AddCallback<ClearNotificationCache>(HandleClearNotificationCache);
      RobotEngineManager.Instance.AddCallback<CacheNotificationToSchedule>(HandleCacheNotificationToSchedule);
    }

    public void Init() {
      Cozmo.PauseManager.Instance.OnPauseStateChanged += HandlePauseStateChanged;
      RobotEngineManager.Instance.SendNotificationsManagerReady();
    }

    public void InitNotificationsIfAllowed() {
      // on platforms that don't require a prompt ( android ) this will always be true.
      // on platforms that require a prompt ( iOS ) this is set by either our settings menu notification panel, 
      //              or first time onboarding, or the weekly repeat of that same onboarding.
      if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.OSNotificationsPermissionsPromptShown) {
        // causes the actual system prompt and sets up UTNotifications
        UTNotifications.Manager.Instance.Initialize(false);
      }
    }

    private void HandlePauseStateChanged(bool isPaused) {
      if (isPaused) {
        ScheduleAllNotifications();
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
        UTNotifications.Manager.Instance.CancelAllNotifications();
      }
    }

    private void ScheduleAllNotifications() {
      if (!UTNotifications.Manager.Instance.IsInitialized()) {
        return;
      }
      CancelAllNotifications();

      foreach (var notification in _NotificationCache) {
        UTNotifications.Manager.Instance.ScheduleNotification(
          notification.SecondsInFuture,
#if UNITY_ANDROID && !UNITY_EDITOR
          "Cozmo",
#else
          // On iOS, the title does not look so good (COZMO-14564)
          string.Empty,
#endif
          Localization.GetWithArgs(notification.TextKey),
          _NotifId++
        );
      }
    }
  }
}