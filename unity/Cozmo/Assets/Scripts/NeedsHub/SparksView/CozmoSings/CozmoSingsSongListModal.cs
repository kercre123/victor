using System;
using Cozmo.Songs;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Needs.Sparks.UI.CozmoSings {

  public class CozmoSingsSongListModal : BaseModal {

    [SerializeField]
    private CozmoSingsSongListCell _SongListCellPrefab;

    [SerializeField]
    private RectTransform _ListContainerRect;

    public new void Initialize() {
      SongLocMap.SongEntry[] allSongs = SongLocMap.Instance.SongEntries;

      // Keep track of how many locked songs there are - since we won't ever unlock a song
      // while the modal is open, we don't have to add the "actual" locked song
      int lockedCount = 0;
      foreach (SongLocMap.SongEntry song in allSongs) {
        if (UnlockablesManager.Instance.IsUnlocked(song.SongId.Value)) {
          CozmoSingsSongListCell cell =
            Instantiate(_SongListCellPrefab.gameObject).GetComponent<CozmoSingsSongListCell>();
          cell.transform.SetParent(_ListContainerRect, false);
          cell.SetSongStatus(false, song);
        }
        else {
          lockedCount++;
        }
      }

      // Add the locked songs
      for (int i = 0; i < lockedCount; i++) {
        CozmoSingsSongListCell cell =
          Instantiate(_SongListCellPrefab.gameObject).GetComponent<CozmoSingsSongListCell>();
        cell.transform.SetParent(_ListContainerRect, false);
        cell.SetSongStatus(true, null);
      }
    }
  }
}
