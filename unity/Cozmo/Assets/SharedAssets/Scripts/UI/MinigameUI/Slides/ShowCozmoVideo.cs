using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ShowCozmoVideo : MonoBehaviour {

  public System.Action OnContinueButton;

  [SerializeField]
  private MediaPlayerCtrl _MediaPlayerCtrl;

  [SerializeField]
  private RawImage _RawImage;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ReplayButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ContinueButton;

  private string _Filename;

#if UNITY_EDITOR
  // The plugin only works in iOS and Android. In the editor we use Unity's video player functionality.
  private Coroutine _PlayCoroutine;
#endif

  private void Awake() {
    _ContinueButton.Initialize(HandleContinueButton, "continue_button", "show_cozmo_video");
    _ReplayButton.Initialize(HandleReplayButton, "replay_button", "show_cozmo_video");
#if !UNITY_EDITOR
    // Disable the background image until the video has been loaded
    _RawImage.enabled = false;

    // Disable buttons until the video has finished
    _ReplayButton.gameObject.SetActive(false);
    _ContinueButton.gameObject.SetActive(false);

    // Register a function to play the video once it has been loaded
    _MediaPlayerCtrl.OnReady += () => {
      _RawImage.enabled = true;
      _MediaPlayerCtrl.Play();
    };

    // Register a function to enable the buttons once the video is finished
    _MediaPlayerCtrl.OnEnd += () => {
      HandleVideoFinished();
    };
#endif
  }

  private void OnDestroy() {
#if UNITY_EDITOR
    if (_PlayCoroutine != null) {
      StopCoroutine(_PlayCoroutine);
      _PlayCoroutine = null;
    }
#else
    _MediaPlayerCtrl.UnLoad();
#endif
  }

  // filename is a path relative to the StreamingAssets folder
  public void PlayVideo(string filename) {
    _Filename = filename;
#if UNITY_EDITOR
    _PlayCoroutine = StartCoroutine(LoadAndPlayCoroutine(filename));
#else
    _MediaPlayerCtrl.Load(filename);
#endif
  }

  private void HandleVideoFinished() {
    _ReplayButton.gameObject.SetActive(true);
    _ContinueButton.gameObject.SetActive(true);
  }

  public void ShowContinueButton(bool show) {
    _ContinueButton.gameObject.SetActive(show);
  }

  private void HandleContinueButton() {
    if (OnContinueButton != null) {
      OnContinueButton();
    }
  }

  private void HandleReplayButton() {
    if (string.IsNullOrEmpty(_Filename)) {
      DAS.Error("ShowCozmoVideo.HandleReplayButton", "Attempting to replay empty video string");
    }
    else {
      PlayVideo(_Filename);
    }
  }

#if UNITY_EDITOR
  private IEnumerator LoadAndPlayCoroutine(string filename) {
    WWW fileWWW = new WWW("file://" + Application.streamingAssetsPath + "/" + filename);

    // Wait until the video has been loaded
    while (!fileWWW.isDone) {
      yield return null;
    }

    if (fileWWW.error != null) {
      Debug.LogError(fileWWW.error);
      _PlayCoroutine = null;
      HandleVideoFinished();
      yield break;
    }

    // When there is a problem with the codec, Unity doesn't gives an error back but just
    // prints error messages in the console. This is the best way we have to know if the 
    // Unity could load the video file correctly
    if (fileWWW.movie.duration <= 0) {
      Debug.LogError("Couldn't load the video file " + filename);
      HandleVideoFinished();
      yield break;
    }

    while (!fileWWW.movie.isReadyToPlay) {
      yield return null;
    }

    // Set the texture to the RawImage's texture so the video is shown on screen and play it
    _RawImage.texture = fileWWW.movie;
    fileWWW.movie.Play();

    _PlayCoroutine = null;
  }
#endif
}
