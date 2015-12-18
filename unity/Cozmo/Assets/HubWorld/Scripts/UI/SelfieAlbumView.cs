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

  private const float kEntryWidth = 500;
  private const int kNumEntries = 5;
  private const float kCenterX = -500;

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

  private string[] _Images;

  private int _CurrentIndex;

  private List<SelfieAlbumEntry> _SpawnedEntries = new List<SelfieAlbumEntry>();

  private void Awake() {
    _SpawnPool = new SimpleObjectPool<SelfieAlbumEntry>(CreateAlbumEntry, 0);

    string folder = Path.Combine(Application.persistentDataPath, _Folder);
    if (Directory.Exists(folder)) {
      _Images = Directory.GetFiles(folder);
    }
    else {
      _Images = new string[0];
    }

    for(int i = 0; i < Mathf.Min(_Images.Length, kNumEntries); i++) {
      var entry = _SpawnPool.GetObjectFromPool();
      entry.SetFilePath(_Images[i]);
      _SpawnedEntries.Add(entry);
    }

    _ScrollViewContent.anchoredPosition += Vector2.right * kCenterX;

    _Preview.SetActive(false);

    _PreviewCloseButton.onClick.AddListener(HandlePreviewClose);
    _CloseButton.onClick.AddListener(HandleClose);
  }

  private SelfieAlbumEntry CreateAlbumEntry() {
    GameObject go = GameObject.Instantiate<GameObject>(_SelfieAlbumEntryPrefab);
    go.transform.SetParent(_ScrollViewContent, false);
    var entry = go.GetComponent<SelfieAlbumEntry>();
    entry.OnSelected += HandleImageSelected;
    return entry;
  }

  private void LateUpdate() {
    if (_SpawnedEntries.Count < _Images.Length) {      
      // if we have scrolled left enough, and we have more images to spawn
      if (_ScrollViewContent.anchoredPosition.x > kCenterX + kEntryWidth && _CurrentIndex > 0) {
        _CurrentIndex--;
        // despawn the image on the right
        _SpawnPool.ReturnToPool(_SpawnedEntries[_SpawnedEntries.Count - 1]);
        _SpawnedEntries.RemoveAt(_SpawnedEntries.Count - 1);
        // now spawn a new image and put it on the left
        var entry = _SpawnPool.GetObjectFromPool();
        entry.transform.SetAsFirstSibling();
        entry.SetFilePath(_Images[_CurrentIndex]);
        _SpawnedEntries.Insert(0, entry);
        _ScrollViewContent.anchoredPosition += Vector2.left * kEntryWidth;
        ScrollRectStartPosition += Vector2.left * kEntryWidth;
        // sometimes we started to bounce before we load in our image. reverse that
        if (_ScrollRect.velocity.x < 0) {
          _ScrollRect.velocity = -_ScrollRect.velocity;
        }
      }
      // if we scroll right enough and have images to spawn
      else if(_ScrollViewContent.anchoredPosition.x < kCenterX - kEntryWidth && _CurrentIndex + kNumEntries < _Images.Length ) {
        _CurrentIndex++;
        // despawn the image on the left
        _SpawnPool.ReturnToPool(_SpawnedEntries[0]);
        _SpawnedEntries.RemoveAt(0);
        // spawn a new image and put it on the right
        var entry = _SpawnPool.GetObjectFromPool();
        entry.transform.SetAsLastSibling();
        entry.SetFilePath(_Images[_CurrentIndex + kNumEntries - 1]);
        _SpawnedEntries.Add(entry);
        _ScrollViewContent.anchoredPosition += Vector2.right * kEntryWidth;
        ScrollRectStartPosition += Vector2.right * kEntryWidth;
        // sometimes we started to bounce before we load in our image. reverse that
        if (_ScrollRect.velocity.x > 0) {
          _ScrollRect.velocity = -_ScrollRect.velocity;
        }
      }
    }
  }

  private void HandleImageSelected(Texture2D image, string path) {
    _Preview.SetActive(true);
    _PreviewImage.texture = image;
  }

  private void HandlePreviewClose() {
    _Preview.SetActive(false);
  }

  private void HandleClose() {
    GameObject.Destroy(gameObject);
  }
}
