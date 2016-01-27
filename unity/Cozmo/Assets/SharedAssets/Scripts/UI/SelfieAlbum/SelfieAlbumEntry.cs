using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class SelfieAlbumEntry : MonoBehaviour {

  [SerializeField]
  private RawImage Image;

  [SerializeField]
  private Button Button;

  [SerializeField]
  private RectTransform RectTransform;

  private string _FilePath;

  private Texture2D _Texture;

  public event System.Action<Texture2D, string> OnSelected;

  private void Awake() {
    // these settings will get overwritten when loadings
    _Texture = new Texture2D(1, 1,TextureFormat.RGB24, false);
    Image.texture = _Texture;
    Button.onClick.AddListener(HandleClick);
    Image.gameObject.SetActive(false);
  }

	void Update () {
    // we want the image to grow as it crosses the center of the screen
    float x = Mathf.Abs(RectTransform.position.x - Screen.width / 2f);

    float t = Mathf.InverseLerp(0f, Screen.width / 2f, x);

    float scale = Mathf.Lerp(1.2f, 0.4f, t);

    RectTransform.localScale = Vector3.one * scale;
	}

  public void SetFilePath(string path) {
    Image.gameObject.SetActive(true);
    Image.color = Color.clear;
    _FilePath = path;
    StartCoroutine(LoadImage());
  }

  private IEnumerator LoadImage() {
    var file = _FilePath;
    WWW www = new WWW("file://" + _FilePath);

    yield return www;

    if (file == _FilePath) {
      www.LoadImageIntoTexture(_Texture);
      Image.color = Color.white;
    }
    www.Dispose();
  }

  private void OnDestroy() {
    Texture.Destroy(_Texture);
  }

  private void HandleClick() {
    if (OnSelected != null) {
      OnSelected(_Texture, _FilePath);
    }
  }
}
