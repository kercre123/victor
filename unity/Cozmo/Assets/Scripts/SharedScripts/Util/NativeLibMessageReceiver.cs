using System;
using UnityEngine;
using Cozmo.Notifications;
using System.Collections.Generic;

namespace Cozmo.Util {
  public class NativeLibMessageReceiver : MonoBehaviour {
    // Defined in AnkiNotifications lib - see AnkiNotifications.m
    const string NOTIFICATION_PERMISSION_GRANTED_KEY = "OnAnkiNotificationsPermissionGranted";
    const string NOTIFICATION_RECEIVED_BACKGROUND_KEY = "OnAnkiNotificationReceivedBackground";
    const string NOTIFICATION_RECEIVED_FOREGROUND_KEY = "OnAnkiNotificationReceivedForeground";
    const string NOTIFICATION_OPENED_KEY = "OnAnkiNotificationOpened";

    public static NativeLibMessageReceiver Instance { get; private set; }

    private Queue<string> _UndeliveredMessages;

    private void Awake() {
      DontDestroyOnLoad(gameObject);
      Instance = this;

      _UndeliveredMessages = new Queue<string>();
    }

    private void OnNativeMessageReceived(string message) {
      DAS.Info("NativeLibMessageReceiver", "Message received: " + message);


      if (DeliverOrStoreNotificationMessage(message)) {
        if (message == NOTIFICATION_PERMISSION_GRANTED_KEY) {
          NotificationsManager.Instance.OnNotificationPermissionGranted();
          return;
        }

        // Can only send 1 string from Obj-C layer to Unity layer - 
        // key will be immediately followed by the JSON notification data
        JSONObject notifJson = messageToJSON(message);
        if (message.Contains(NOTIFICATION_RECEIVED_BACKGROUND_KEY)) {
          NotificationsManager.Instance.OnNotificationReceived(notifJson);
        }
        else if (message.Contains(NOTIFICATION_RECEIVED_FOREGROUND_KEY)) {
          NotificationsManager.Instance.OnNotificationReceived(notifJson);
        }
        else if (message.Contains(NOTIFICATION_OPENED_KEY)) {
          NotificationsManager.Instance.OnNotificationOpened(notifJson);
        }
      }
    }

    private JSONObject messageToJSON(string message) {
      string stripped = "";
      if (message.Contains(NOTIFICATION_RECEIVED_FOREGROUND_KEY)) {
        stripped = message.Replace(NOTIFICATION_RECEIVED_FOREGROUND_KEY, "");
      }
      else if (message.Contains(NOTIFICATION_RECEIVED_BACKGROUND_KEY)) {
        stripped = message.Replace(NOTIFICATION_RECEIVED_BACKGROUND_KEY, "");
      }
      else if (message.Contains(NOTIFICATION_OPENED_KEY)) {
        stripped = message.Replace(NOTIFICATION_OPENED_KEY, "");
      }

      if (!String.IsNullOrEmpty(stripped)) {
        return new JSONObject(stripped);
      }

      return new JSONObject();
    }

    // Stores the message if NotificationsManager isn't inited     
    // Returns true if caller should deliver message
    private bool DeliverOrStoreNotificationMessage(string message) {
      if (NotificationsManager.Instance == null) {
        _UndeliveredMessages.Enqueue(message);
        return false;
      }

      return true;
    }

    public void DeliverStoredMessages() {
      while (_UndeliveredMessages.Count > 0 && _UndeliveredMessages.Peek() != null) {
        OnNativeMessageReceived(_UndeliveredMessages.Dequeue());
      }
    }
  }
}
