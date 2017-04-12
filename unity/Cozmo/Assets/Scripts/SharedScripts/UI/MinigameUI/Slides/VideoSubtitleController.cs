using UnityEngine;
using System.Collections.Generic;

[System.Serializable]
public class VideoLocalizationPair {
  public string _LocalizationKey;
  public float _DurationSeconds;
}

public class VideoSubtitleController : MonoBehaviour {

  [SerializeField]
  private MediaPlayerCtrl _MediaPlayerCtrl;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _SubtitlesText;

  [SerializeField]
  private UnityEngine.UI.Image _SubtitlesBackground;

  private int _CurrentIndex = 0;
  private float _IndexTime = 0.0f;

  private List<VideoLocalizationPair> _VideoLocalizationKeyPairs = new List<VideoLocalizationPair>();

  public void PlaySubtitles(List<VideoLocalizationPair> subtitles) {
    _CurrentIndex = 0;
    _IndexTime = 0.0f;
    _VideoLocalizationKeyPairs = subtitles;

    if (subtitles == null || subtitles.Count == 0) {
      return;
    }

    SetSubtitlesWithText(Localization.Get(_VideoLocalizationKeyPairs[0]._LocalizationKey));

  }

  private void Update() {
    if (_CurrentIndex < _VideoLocalizationKeyPairs.Count) {
      float playTimePositionSeconds = _MediaPlayerCtrl.GetSeekPosition() / 1000.0f;
      if (_IndexTime + _VideoLocalizationKeyPairs[_CurrentIndex]._DurationSeconds < playTimePositionSeconds) {
        // playtime is past the current index's duration, let's move to the next one.
        _IndexTime += _VideoLocalizationKeyPairs[_CurrentIndex]._DurationSeconds;
        _CurrentIndex++;
        if (_CurrentIndex == _VideoLocalizationKeyPairs.Count) {
          // we've reached the end! hide the text.
          SetSubtitlesWithText("");
        }
        else {
          // we haven't reached the end so let's show the next text.
          SetSubtitlesWithText(Localization.Get(_VideoLocalizationKeyPairs[_CurrentIndex]._LocalizationKey));
        }
      }
    }
  }

  private void SetSubtitlesWithText(string subtitleText) {
    if (string.IsNullOrEmpty(subtitleText)) {
      _SubtitlesText.text = "";
      _SubtitlesBackground.gameObject.SetActive(false);
    }
    else {
      _SubtitlesText.text = subtitleText;
      _SubtitlesBackground.gameObject.SetActive(true);
    }
  }
}
