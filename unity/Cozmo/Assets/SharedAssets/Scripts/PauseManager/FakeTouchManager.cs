using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;

public class FakeTouchManager : MonoBehaviour {

  private const float kSoakTestTapInterval = 0.25f;

  private static FakeTouchManager _Instance;

  public static FakeTouchManager Instance {
    get {
      if (_Instance == null) {
        DAS.Error("FakeTouchManager.NullInstance", "Do not access FakeTouchManager until start: " + System.Environment.StackTrace);
      }
      return _Instance;
    }
    private set {
      if (_Instance != null) {
        DAS.Error("FakeTouchManager.DuplicateInstance", "FakeTouchManager Instance already exists");
      }
      _Instance = value;
    }
  }

  private List<FakeTouch> _RecordedTouches = new List<FakeTouch>();
  private List<FakeTouch> _PlaybackTouches = new List<FakeTouch>();
  public bool IsPlayingTouches = false;
  public bool IsSoakingTouches = false;
  public bool IsRecordingTouches = false;
  private float _PlaybackStartTimestamp;
  private float _RecordingStartTimestamp;

  void Awake() {
    Instance = this;

  }

#if ENABLE_DEBUG_PANEL
  /// <summary>
  /// Update this instance, listens for fake touches and records input
  /// Only bother with this if using Debug Tools and those flags have
  /// been thrown
  /// </summary>
  void Update() {
    if (IsPlayingTouches || IsSoakingTouches) {
      UpdatePlayback();
    }
    if (IsRecordingTouches) {
      RecordInput();
    }
  }
#endif



  public void SendFakeTouch(FakeTouch touch) {
    if (touch == null) {
      SetRecordingInputs(false);
      Debug.LogError("Attempted to send NULL Faketouch");
      return;
    }

    PointerEventData pointerEvent = new PointerEventData(UIManager.Instance.EventSystemScript);
    pointerEvent.eligibleForClick = true;
    pointerEvent.delta = Vector2.zero;
    pointerEvent.dragging = false;
    pointerEvent.useDragThreshold = true;
    pointerEvent.position = touch.TouchPos;
    pointerEvent.pressPosition = pointerEvent.position;
    List<RaycastResult> raycastResults = new List<RaycastResult>();
    UIManager.Instance.EventSystemScript.RaycastAll(pointerEvent, raycastResults);
    for (int i = 0; i < raycastResults.Count; i++) {
      pointerEvent.pointerCurrentRaycast = raycastResults[i];
      pointerEvent.pointerPressRaycast = pointerEvent.pointerCurrentRaycast;
      ExecuteEvents.ExecuteHierarchy(pointerEvent.pointerPressRaycast.gameObject, pointerEvent, ExecuteEvents.pointerClickHandler);
    }

  }

  public void RecordInput() {
    if (!DebugMenuManager.Instance.IsDialogOpen()) {
      if (Input.GetMouseButtonDown(0)) {
        Debug.Log(string.Format("TouchRecorded : Timestamp : {0} Pos : {1}", Time.time - _RecordingStartTimestamp, Input.mousePosition));
        _RecordedTouches.Add(new FakeTouch(Input.mousePosition, Time.time - _RecordingStartTimestamp));
      }
    }
  }

  public void SaveRecordedInputs(string recordingName) {
    Debug.Log(string.Format("Save Playback of {0} touches", _RecordedTouches.Count));
    Dictionary<string, List<FakeTouch>> savedRecordings = DataPersistenceManager.Instance.Data.DebugPrefs.FakeTouchRecordings;
    if (savedRecordings.ContainsKey(recordingName)) {
      savedRecordings[recordingName].Clear();
      savedRecordings[recordingName].AddRange(_RecordedTouches);
    }
    else {
      savedRecordings.Add(recordingName, _RecordedTouches);
    }
    DataPersistenceManager.Instance.Save();
  }

  public void SetRecordingInputs(bool isRecording) {
    if (isRecording) {
      _RecordedTouches.Clear();
      _RecordingStartTimestamp = Time.time;
      IsPlayingTouches = false;
    }
    IsRecordingTouches = isRecording;
  }

  // Stop recording, then playback the current saved Fake touch recording
  public void PlayBackRecordedInputs(string recordingName) {
    _PlaybackTouches.Clear();
    if (DataPersistenceManager.Instance.Data.DebugPrefs.FakeTouchRecordings.TryGetValue(recordingName, out _PlaybackTouches)) {
      _PlaybackStartTimestamp = Time.time;
      Debug.Log(string.Format("Start Playback of {0} touches", _PlaybackTouches.Count));
      IsPlayingTouches = true;
      IsRecordingTouches = false;
    }
  }

  public void UpdatePlayback() {
    if (DebugMenuManager.Instance == null) {
      return;
    }
    if (DebugMenuManager.Instance.IsDialogOpen() == false) {
      if (IsPlayingTouches) {
        if (_PlaybackTouches.Count <= 0) {
          IsPlayingTouches = false;
          Debug.Log("Finish Touch Playback");
        }
        else if (Time.time - _PlaybackStartTimestamp >= _PlaybackTouches[0].TimeStamp) {
          // If the next touch coming up should happen, send the fake touch
          // and remove from the list.
          SendFakeTouch(_PlaybackTouches[0]);
          _PlaybackTouches.RemoveAt(0);
        }
      }
      else if (IsSoakingTouches) {
        if (Input.GetMouseButtonDown(0)) {
          IsSoakingTouches = false;
          Debug.Log("End Soak Test");
        }
        if (Time.time - _PlaybackStartTimestamp >= kSoakTestTapInterval) {
          _PlaybackStartTimestamp = Time.time;
          SendFakeTouch(new FakeTouch(new Vector3(Random.Range(0, UIManager.Instance.MainCamera.pixelWidth), Random.Range(0, UIManager.Instance.MainCamera.pixelHeight),
            0.0f), Time.time - _PlaybackStartTimestamp));
        }
      }
    }
  }
}
