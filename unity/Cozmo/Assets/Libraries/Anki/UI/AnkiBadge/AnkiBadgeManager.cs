using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections.Generic;

namespace Anki {
  namespace UI {
    
    public static class AnkiBadgeManager {

      public delegate int BadgeDataSource();

      public static void RegisterBadgeSource(string type, BadgeDataSource source) {
        _badgeSources[type] = source;
        _badgeListeners[type] = null;
      }

      public static void UnregisterBadgeSource(string type) {
        _badgeSources[type] = null;
      }

      private static Dictionary<string, BadgeDataSource> _badgeSources = new Dictionary<string, BadgeDataSource>();
      private static Dictionary<string, Action<int>> _badgeListeners = new Dictionary<string, Action<int>>();

      public static int GetBadgeNumber(string badgeType) {
        BadgeDataSource source;
        if (_badgeSources.TryGetValue(badgeType, out source)) {
          return source();
        }
        else {
          DAS.Warn("AnkiBadgeManager.GetBadgeNumber", "can't find data source for badge type " + badgeType);
          return 0;
        }
      }

      public static void HandleUpdate(string badgeType, int count) {
        Action<int> action;
        if (_badgeListeners.TryGetValue(badgeType, out action)) {
          if (action != null) {
            action(count);
          }
        }
      }

      public static void AddListener(string badgeType, Action<int> callback) {
        if (_badgeListeners.ContainsKey(badgeType)) {
          _badgeListeners[badgeType] += callback;
        }
        else {
          DAS.Warn("AnkiBadgeManager.AddListener", "can't find listener for badge type " + badgeType);
        }
      }

      public static void RemoveListener(string badgeType, Action<int> callback) {
        if (_badgeListeners.ContainsKey(badgeType)) {
          _badgeListeners[badgeType] -= callback;
        }
        else {
          DAS.Warn("AnkiBadgeManager.RemoveListener", "can't find listener for badge type " + badgeType);
        }
      }
    }
  }
}
