using System;
using UnityEngine;

namespace Cozmo.Needs.Sparks.UI.CozmoSings {

  public class CozmoSingsSongListCell : MonoBehaviour {

    [SerializeField]
    private CozmoText _SongTitle;

    [SerializeField]
    private CozmoImage _LockStatusIcon;

    public CozmoText SongTitle {
      get {
        return _SongTitle;
      }

      set {
        _SongTitle = value;
      }
    }

    public void SetSongStatus(bool isLocked, string songTitle = null) {
      // Determine themes
      string titleThemeKey = isLocked ? ThemeKeys.Cozmo.TextMeshPro.kCozmoSingsSongTitleLocked
                             : ThemeKeys.Cozmo.TextMeshPro.kCozmoSingsSongTitleUnlocked;
      string iconThemeKey = isLocked ? ThemeKeys.Cozmo.Image.kCozmoSingsSongIconLocked
                            : ThemeKeys.Cozmo.Image.kCozmoSingsSongIconUnlocked;
      // Update componentId's
      _SongTitle.LinkedComponentId = titleThemeKey;
      _LockStatusIcon.LinkedComponentId = iconThemeKey;

      if (!isLocked) {
        if (!string.IsNullOrEmpty(songTitle)) {
          SongTitle.text = songTitle;
        }
        else {
          DAS.Warn("CozmoSingsSongListCell", "Tried to unlock a SongListCell without a song title!");
        }
      }

      // Trigger skin update
      _SongTitle.UpdateSkinnableElements(CozmoThemeSystemUtils.sInstance.GetCurrentThemeId(),
                         CozmoThemeSystemUtils.sInstance.GetCurrentSkinId());
      _LockStatusIcon.UpdateSkinnableElements();
    }
  }
}
