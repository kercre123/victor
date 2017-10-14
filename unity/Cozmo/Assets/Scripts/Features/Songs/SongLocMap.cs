using Anki.Cozmo;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Songs {
  public class SongLocMap : ScriptableObject {
    [System.Serializable]
    public class SongEntry {
      public UnlockableInfo.SerializableUnlockIds SongId;
      public string SongTitleLocKey;
    }

    private static SongLocMap _sInstance;

    public static SongLocMap Instance {
      get {
        return _sInstance;
      }
    }

    public static void SetInstance(SongLocMap instance) {
      _sInstance = instance;
    }

    [SerializeField]
    private Sprite _SongIcon;
    public static Sprite SongIcon { get { return _sInstance._SongIcon; } }

    [SerializeField]
    private SongEntry[] _SongEntries;

    public SongEntry[] SongEntries {
      get {
        return _SongEntries;
      }
    }

    // Opt to search through with every query because it doesn't happen very often.
    public static string GetTitleForSong(UnlockId unlockId) {
      SongEntry entry;
      string locKey = null;
      for (int i = 0; i < _sInstance._SongEntries.Length; i++) {
        entry = _sInstance._SongEntries[i];
        if (entry.SongId.Value == unlockId) {
          locKey = entry.SongTitleLocKey;
          break;
        }
      }

      string songTitle = "";
      if (locKey != null) {
        songTitle = Localization.Get(locKey);
      }

      return songTitle;
    }
  }
}