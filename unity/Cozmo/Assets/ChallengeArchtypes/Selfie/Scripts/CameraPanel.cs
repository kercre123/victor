using System;
using UnityEngine.UI;
using UnityEngine;
using System.Linq;
using System.IO;

public class CameraPanel : MonoBehaviour {
  public RawImage CamDisplay;

  public int Width = 1920;
  public int Height = 1080;

  public string SaveFolder = "Selfies";

  private WebCamTexture _CamTexture;

  void Awake() {
    WebCamDevice device = WebCamTexture.devices[0];
    for (int i = 0; i < WebCamTexture.devices.Length; i++) {
      if (WebCamTexture.devices[i].isFrontFacing) {
        device = WebCamTexture.devices[i];
        break;
      }
    }

    _CamTexture = new WebCamTexture(device.name, Width, Height);
    CamDisplay.texture = _CamTexture;
    _CamTexture.Play();
  }

  void Update() {
    var size = CamDisplay.rectTransform.rect.size;

    float imageRatio = _CamTexture.width / (float)_CamTexture.height;
    float sizeRatio = size.x / size.y;

    Rect uvRect = new Rect();
    if (imageRatio > sizeRatio) {
      uvRect.height = 1;
      uvRect.width = sizeRatio / imageRatio;

      uvRect.x = (1 - uvRect.width) / 2;
    }
    else {
      uvRect.width = 1;
      uvRect.height = imageRatio / sizeRatio;

      uvRect.y = (1 - uvRect.height) / 2;
    }

    CamDisplay.uvRect = uvRect;
  }

  public void PrepareForPhoto() {
    CamDisplay.texture = Texture2D.whiteTexture;
  }

  public void TakePhoto() {
    CamDisplay.texture = _CamTexture;

    Texture2D resultTexture = new Texture2D(_CamTexture.width, _CamTexture.height, TextureFormat.ARGB32, false);

    resultTexture.SetPixels32(_CamTexture.GetPixels32());

    var jpg = resultTexture.EncodeToJPG();

    string folder = Path.Combine(Application.persistentDataPath, SaveFolder);

    Directory.CreateDirectory(folder);

    string fileName = DateTime.Now.ToString().Replace('/', '-').Replace(' ', '_') + ".jpg";
    File.WriteAllBytes(Path.Combine(folder, fileName), jpg);

    Texture.Destroy(resultTexture);
    _CamTexture.Stop();
  }

  void OnDestroy() {
    Destroy(_CamTexture);
  }
}

