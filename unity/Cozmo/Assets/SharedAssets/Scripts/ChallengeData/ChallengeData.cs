using UnityEngine;
using System.Collections.Generic;
using Anki.Assets;

public class ChallengeData : ScriptableObject {

  [System.Serializable]
  private class ChallengePrefabDataLink : AssetBundleAssetLink<ChallengePrefabData> { }

  [SerializeField]
  private ChallengePrefabDataLink _ChallengePrefabData;

  public string PrefabDataAssetBundle {
    get { return _ChallengePrefabData.AssetBundle; }
  }

  public void LoadPrefabData(System.Action<ChallengePrefabData> dataLoadedCallback) {
    _ChallengePrefabData.LoadAssetData(dataLoadedCallback);
  }

  // the key used to find this specific challenge
  [SerializeField]
  private string _ChallengeID;

  public string ChallengeID {
    get { return _ChallengeID; }
  }

  // the key used to get the actual localized string for the
  // challenge title shown to the player.
  [SerializeField]
  private string _ChallengeTitleLocKey;

  public string ChallengeTitleLocKey {
    get { return _ChallengeTitleLocKey; }
  }

  // the key used to get the actual localized string for the
  // challenge description shown to the player.
  [SerializeField]
  private string _ChallengeDescriptionLocKey;

  public string ChallengeDescriptionLocKey {
    get { return _ChallengeDescriptionLocKey; }
  }

  // icon to show to represent this challenge
  [SerializeField]
  private Sprite _ChallengeIcon;

  public Sprite ChallengeIcon {
    get { return _ChallengeIcon; }
  }

  [SerializeField]
  private UnlockableInfo.SerializableUnlockIds _UnlockId;

  public UnlockableInfo.SerializableUnlockIds UnlockId {
    get { return _UnlockId; }
  }

  [SerializeField]
  protected MusicStateWrapper _Music;

  public Anki.Cozmo.Audio.GameState.Music DefaultMusic {
    get { return _Music.Music; }
  }

  // string path to MinigameConfig
  public MinigameConfigBase MinigameConfig;


  [SerializeField]
  public bool IsMinigame;

  [SerializeField]
  public string InstructionVideoPath;

  public List<DifficultySelectOptionData> DifficultyOptions = new List<DifficultySelectOptionData>();
}

[System.Serializable]
public struct MusicStateWrapper {

  public MusicStateWrapper(Anki.Cozmo.Audio.GameState.Music music) {
    _Music = (int)music;
  }

  [SerializeField]
  private int _Music;

  public Anki.Cozmo.Audio.GameState.Music Music {
    get { return (Anki.Cozmo.Audio.GameState.Music)_Music; }
    set { _Music = (int)value; }
  }

  public static implicit operator Anki.Cozmo.Audio.GameState.Music(MusicStateWrapper other) {
    return other.Music;
  }

  public static implicit operator MusicStateWrapper(Anki.Cozmo.Audio.GameState.Music other) {
    return new MusicStateWrapper(other);
  }

}
