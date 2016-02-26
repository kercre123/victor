using System;
using UnityEngine;
using UnityEngine.UI;

namespace Anki.Cozmo.Viz {
  public class VizLabel : MonoBehaviour {

    [SerializeField]
    private Text _Label;

    public string Text {
      get { return _Label.text; }
      set { _Label.text = value; }
    }

    public Color Color {
      get { return _Label.color; }
      set { _Label.color = value; }
    }

    private Camera _VizCamera;
    private Transform _VizScene;

    private Vector3 _vizPosition;

    private void LateUpdate() {
      var position = _VizCamera.WorldToViewportPoint(_VizScene.TransformPoint(_vizPosition));

      _RectTransform.anchorMin = _RectTransform.anchorMax = position;

    }

    public void Initialize(string name, Camera vizCam, Transform vizScene) {
      gameObject.name = name;
      _VizCamera = vizCam;
      _VizScene = vizScene;
    }

    private RectTransform _RectTransform;

    private void Awake() {
      _RectTransform = transform as RectTransform;
    }

    public void SetPosition(Vector3 position) {
      _vizPosition = position;
    }
  }
}