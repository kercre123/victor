using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.UI;

public class ListViewDefaultCell : ListViewCell {




  public new static ListViewDefaultCell CreateInstance(string name)
  {
    ListViewDefaultCell instance = new GameObject( (name == null) ? "ListViewDefaultCell" : name ).AddComponent<ListViewDefaultCell>();

    LayoutElement layoutElement = instance.gameObject.AddComponent<LayoutElement>();
    layoutElement.preferredHeight = 160.0f;
    layoutElement.flexibleWidth = 1.0f;
      
    instance.LayoutCell();
      
    return instance;
  }

  [SerializeField]
  private Button _CellButton;

  private Color _BGColor;
  private Color _SelectedBGColor;

  [SerializeField]
  private Image _ThumbnailImage;

  [SerializeField]
  private Text _TitleText;

  public string Title
  {
    set {
      _TitleText.text = value;
    }
  }

  public Color BackgroundColor
  {
    set { 
      Image bgImage = gameObject.GetComponent<Image>();
      _BGColor = value;
      bgImage.color = _BGColor;
    }
  }

  public Color SelectedBackgroundColor
  {
    set { 
      _SelectedBGColor = value;
    }
  }

  public Sprite ThumbnailImageSprite
  {
    set {
      _ThumbnailImage.gameObject.SetActive((value != null));
      _ThumbnailImage.overrideSprite = value;
      LayoutCell();
    }
  }

  public Button CellButton
  {
    get { return _CellButton; }
  }


	protected override void OnLoad()
  {
    base.OnLoad();

    _ThumbnailImage.gameObject.SetActive(false);

    LayoutCell();
  }

  public override void SetIsSelected(bool selected)
  {
    base.SetIsSelected(selected);

    if (selected) {
      Image bgImage = gameObject.GetComponent<Image>();
      bgImage.color = _SelectedBGColor;
    }
    else { 
      Image bgImage = gameObject.GetComponent<Image>();
      bgImage.color = _BGColor;
    }
  }

  private void LayoutCell()
  {
    Debug.Log("LayoutCell");
    RectTransform textTransform = _TitleText.GetComponent<Transform>() as RectTransform;
    RectTransform imageTransform = _ThumbnailImage.GetComponent<Transform>() as RectTransform;
    if (textTransform != null && imageTransform != null) {
      Debug.Log("LayoutCell != null");
      bool showImage = _ThumbnailImage.gameObject.activeSelf;
      if (showImage) {
        textTransform.anchorMin = new Vector2(0.22f, 0.05f);
        textTransform.anchorMax = new Vector2(0.95f, 0.95f);
        imageTransform.anchorMin = new Vector2(0.01f, 0.01f);
        imageTransform.anchorMax = new Vector2(0.20f, 0.99f);
      }
      else {
        textTransform.anchorMin = new Vector2(0.05f, 0.05f);
        textTransform.anchorMax = new Vector2(0.95f, 0.95f);
      }
    }
  }

}
