using UnityEngine;
using System.Collections;
using System;
using System.IO;
using UnityEngine.UI;
using System.Collections.Generic;

public class SelfieAlbumView : MonoBehaviour {

  [SerializeField]
  private GameObject _SelfieAlbumEntryPrefab;

  [SerializeField]
  private RectTransform _ScrollViewContent;

  [SerializeField]
  private ScrollRect _ScrollRect;

  [SerializeField]
  private GameObject _Preview;

  [SerializeField]
  private RawImage _PreviewImage;

  [SerializeField]
  private Button _CloseButton;

  [SerializeField]
  private Button _PreviewCloseButton;

  [SerializeField]
  private Button _PreviewDeleteButton;

  private const float kEntryWidth = 500;
  private const int kNumEntries = 5;
  private const float kCenterX = -1000;

  private RectTransform _LeftBuffer;
  private RectTransform _RightBuffer;

  private int _CurrentSelectedIndex;

  SimpleObjectPool<SelfieAlbumEntry> _SpawnPool;

  // The Drag start position is private, but we need to reset it when we
  // rotate our images, so use reflection to grab it. 
  // Get a static reference to the field info for faster access
  private static System.Reflection.FieldInfo _ScrollRectStartPositionField;

  static SelfieAlbumView() {
    _ScrollRectStartPositionField = typeof(ScrollRect).GetField("m_ContentStartPosition", 
      System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic);
  }

  // A property to make dealing with the reflected field easier
  private Vector2 ScrollRectStartPosition {
    get {
      return (Vector2)_ScrollRectStartPositionField.GetValue(_ScrollRect);
    }
    set {
      _ScrollRectStartPositionField.SetValue(_ScrollRect, value);
    }
  }

  [SerializeField]
  private string _Folder = "Selfies";

  private List<string> _Images;

  private int _CurrentIndex;

  private List<SelfieAlbumEntry> _SpawnedEntries = new List<SelfieAlbumEntry>();

  private void Awake() {
    _SpawnPool = new SimpleObjectPool<SelfieAlbumEntry>(CreateAlbumEntry, ResetAlbumEntry, 0);

    string folder = Path.Combine(Application.persistentDataPath, _Folder);
    if (Directory.Exists(folder)) {
      _Images = new List<string>(Directory.GetFiles(folder));
    }
    else {
      _Images = new List<string>();
    }

    _LeftBuffer = _SpawnPool.GetObjectFromPool().GetComponent<RectTransform>();

    for(int i = 0; i < Mathf.Min(_Images.Count, kNumEntries); i++) {
      var entry = _SpawnPool.GetObjectFromPool();
      entry.SetFilePath(_Images[i]);
      _SpawnedEntries.Add(entry);
    }

    _RightBuffer = _SpawnPool.GetObjectFromPool().GetComponent<RectTransform>();

    // scroll to our first element
    _ScrollViewContent.anchoredPosition += Vector2.left * kEntryWidth;

    _Preview.SetActive(false);

    _PreviewCloseButton.onClick.AddListener(HandlePreviewClose);
    _CloseButton.onClick.AddListener(HandleClose);
    _PreviewDeleteButton.onClick.AddListener(HandleDelete);
  }

  private SelfieAlbumEntry CreateAlbumEntry() {
    GameObject go = GameObject.Instantiate<GameObject>(_SelfieAlbumEntryPrefab);
    go.transform.SetParent(_ScrollViewContent, false);
    var entry = go.GetComponent<SelfieAlbumEntry>();
    entry.OnSelected += HandleImageSelected;
    return entry;
  }

  private void ResetAlbumEntry(SelfieAlbumEntry entry, bool spawned) {
    entry.gameObject.SetActive(spawned);
  }

  private void LateUpdate() {
    if (_SpawnedEntries.Count < _Images.Count) {      
      // if we have scrolled left enough, and we have more images to spawn
      if (_ScrollViewContent.anchoredPosition.x > kCenterX + kEntryWidth && _CurrentIndex > 0) {
        _CurrentIndex--;
        int lastIndex = _SpawnedEntries.Count - 1;
        // despawn the image on the right
        _SpawnPool.ReturnToPool(_SpawnedEntries[lastIndex]);
        _SpawnedEntries.RemoveAt(lastIndex);
        // now spawn a new image and put it on the left
        var entry = _SpawnPool.GetObjectFromPool();
        entry.transform.SetAsFirstSibling();
        _LeftBuffer.SetAsFirstSibling();
        entry.SetFilePath(_Images[_CurrentIndex]);
        _SpawnedEntries.Insert(0, entry);
        _ScrollViewContent.anchoredPosition += Vector2.left * kEntryWidth;
        ScrollRectStartPosition += Vector2.left * kEntryWidth;
      }
      // if we scroll right enough and have images to spawn
      else if(_ScrollViewContent.anchoredPosition.x < kCenterX - kEntryWidth && _CurrentIndex + kNumEntries < _Images.Count ) {
        _CurrentIndex++;
        // despawn the image on the left
        _SpawnPool.ReturnToPool(_SpawnedEntries[0]);
        _SpawnedEntries.RemoveAt(0);
        // spawn a new image and put it on the right
        var entry = _SpawnPool.GetObjectFromPool();
        entry.transform.SetAsLastSibling();
        _RightBuffer.SetAsLastSibling();
        entry.SetFilePath(_Images[_CurrentIndex + kNumEntries - 1]);
        _SpawnedEntries.Add(entry);
        _ScrollViewContent.anchoredPosition += Vector2.right * kEntryWidth;
        ScrollRectStartPosition += Vector2.right * kEntryWidth;
      }
    }
  }

  private void HandleImageSelected(Texture2D image, string path) {
    _CurrentSelectedIndex = _Images.IndexOf(path);
    _Preview.SetActive(true);
    var previewRectTransform = _Preview.GetComponent<RectTransform>();
    var previewImageRectTransform = _PreviewImage.GetComponent<RectTransform>();
    var widthDelta = previewRectTransform.rect.width - previewImageRectTransform.rect.width;

    float imageWidth = (image.width / (float)image.height) * previewImageRectTransform.rect.height;
    // resize the preview to be the correct resolution for the image
    previewRectTransform.SetSizeWithCurrentAnchors(RectTransform.Axis.Horizontal, imageWidth + widthDelta);
    previewImageRectTransform.SetSizeWithCurrentAnchors(RectTransform.Axis.Horizontal, imageWidth);
    _PreviewImage.texture = image;
  }

  private void HandlePreviewClose() {
    _Preview.SetActive(false);
  }

  private void HandleDelete() {

    File.Delete(_Images[_CurrentSelectedIndex]);

    _Images.RemoveAt(_CurrentSelectedIndex);

    // the image is currently on screen.
    if (_CurrentSelectedIndex >= _CurrentIndex && _CurrentSelectedIndex < _CurrentIndex + kNumEntries) {
      int spawnedIndex = _CurrentSelectedIndex - _CurrentIndex;

      var entry = _SpawnedEntries[spawnedIndex];
      _SpawnedEntries.RemoveAt(spawnedIndex);
      _SpawnPool.ReturnToPool(entry);

      if (_CurrentIndex + kNumEntries - 1 < _Images.Count) {
        entry = _SpawnPool.GetObjectFromPool();
        entry.transform.SetAsLastSibling();
        _RightBuffer.SetAsLastSibling();
        entry.SetFilePath(_Images[_CurrentIndex + kNumEntries - 1]);
        _SpawnedEntries.Add(entry);
      }
      else if (_CurrentIndex > 0) {
        _CurrentIndex--;
        entry = _SpawnPool.GetObjectFromPool();
        entry.transform.SetAsFirstSibling();
        _LeftBuffer.SetAsFirstSibling();
        entry.SetFilePath(_Images[_CurrentIndex]);
        _SpawnedEntries.Insert(0, entry);
      }
    }


    HandlePreviewClose();
  }

  private void HandleClose() {
    GameObject.Destroy(gameObject);
  }
}
