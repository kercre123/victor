using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ObservedObjectBox : MonoBehaviour
{
  [System.NonSerialized] public ObservedObject observedObject;

  private Image[] _images;
  public Image[] images {
    get {
      if(!initialized) Initialize();
      return _images;
    }
    private set {
      _images = value;
    }
  }

  private Text _text;
  public Text text {
    get {
      if(!initialized) Initialize();
      return _text;
    }
    private set {
      _text = value;
    }
  }

  private RectTransform _rectTransform;
  public RectTransform rectTransform {
    get {
      if(!initialized) Initialize();
      return _rectTransform;
    }
    private set {
      _rectTransform = value;
    }
  }

  private Canvas _canvas;
  public Canvas canvas {
    get {
      if(!initialized) Initialize();
      return _canvas;
    }
    private set {
      _canvas = value;
    }
  }

  public Color color { get; private set; }

  protected bool initialized { get; private set; }

  protected virtual void Initialize() {
    if(initialized) return;
    initialized = true;

    images = GetComponentsInChildren<Image>(true);

    //hack because GetComponentInChildren cannot retrieve disabled components
    Text[] texts = GetComponentsInChildren<Text>(true);
    if(texts != null && texts.Length > 0) text = texts[0];

    rectTransform = transform as RectTransform;
    color = Color.white;

    canvas = GetComponentInParent<Canvas>();

    //Debug.Log(gameObject.name + " ObservedObjectBox Awake rectTransform("+rectTransform+") text("+text+")");
  }

  protected void Awake() {
    Initialize();
  }

  public virtual void SetColor(Color color) {
    this.color = color;
    SetTextColor(color);
    SetImageColor(color);
  }

  public void SetTextColor(Color color) {
    if(text != null) text.color = color;
  }

  public void SetImageColor(Color color) {
    for(int i=0;i<images.Length;i++) {
      images[i].color = color;
    }
  }

  public void SetText(string s) {
    if(text != null) text.text = s;
  }
}
