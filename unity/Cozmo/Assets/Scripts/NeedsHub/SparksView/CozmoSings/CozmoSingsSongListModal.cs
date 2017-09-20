using System;
using System.Collections.Generic;
using Cozmo.Songs;
using Cozmo.UI;
using Anki.Cozmo;
using UnityEngine;
using G2U = Anki.Cozmo.ExternalInterface;

namespace Cozmo.Needs.Sparks.UI.CozmoSings {

  public class CozmoSingsSongListModal : BaseModal {

    [SerializeField]
    private CozmoSingsSongListCell _SongListCellPrefab;

    [SerializeField]
    private RectTransform _ListContainerRect;

    public new void Initialize() {
      RobotEngineManager.Instance.AddCallback<G2U.SongsList>(HandleSongsListResponse);

      RobotEngineManager.Instance.Message.GetSongsList = new G2U.GetSongsList();
      RobotEngineManager.Instance.SendMessage();
    }

    void HandleSongsListResponse(G2U.SongsList message) {
      List<G2U.SongUnlockStatus> songList = new List<G2U.SongUnlockStatus>(message.songUnlockStatuses);
      if (songList == null) {
        DAS.Error("CozmoSingsSongListModal.HandleSongsListResponse.SongListNull", "Could not get song list from robot!");
        return;
      }

      // Keep track of how many locked songs there are - since we won't ever unlock a song
      // while the modal is open, we don't have to add the "actual" locked song
      int lockedCount = 0;
      for (int i = 0; i < songList.Count; i++) {
        if (songList[i].unlocked) {
          try {
            UnlockId id = (UnlockId)Enum.Parse(typeof(UnlockId), songList[i].songUnlockId);
            CozmoSingsSongListCell cell = Instantiate(_SongListCellPrefab.gameObject).GetComponent<CozmoSingsSongListCell>();
            cell.transform.SetParent(_ListContainerRect, false);
            cell.SetSongStatus(false, SongLocMap.GetTitleForSong(id));
          }
          catch (ArgumentException) {
            DAS.Error("CozmoSingsSongListModal.HandleSongsListResponse.SongUnlockStatusInvalid",
                      "Tried to parse a SongUnlockStatus that wasn't a valid UnlockId!");
          }
        }
        else {
          lockedCount++;
        }
      }

      // Add the locked songs
      for (int i = 0; i < lockedCount; i++) {
        CozmoSingsSongListCell cell = Instantiate(_SongListCellPrefab.gameObject).GetComponent<CozmoSingsSongListCell>();
        cell.transform.SetParent(_ListContainerRect, false);
        cell.SetSongStatus(true);
      }
    }

    void OnDestroy() {
      RobotEngineManager.Instance.RemoveCallback<G2U.SongsList>(HandleSongsListResponse);
    }
  }
}
