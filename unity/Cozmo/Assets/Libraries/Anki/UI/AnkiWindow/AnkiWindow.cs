using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Anki {
  namespace UI {

    [RequireComponent(typeof(CanvasGroup))]
    public class AnkiWindow : MonoBehaviour {

      [SerializeField]
      private GameObject
        _ContentPanel;
      
      [SerializeField]
      private Mask
        _MaskComponent;
      
      [SerializeField]
      private Image
        _MaskImage;
      
      [SerializeField]
      private Image
        _BackgroundImage;

      public Image SetBGRef {
        set { _BackgroundImage = value;}
      }

      public Mask SetMaskRef {
        set { _MaskComponent = value;}
      }

      public Image SetMaskBgRef {
        set { _MaskImage = value;}
      }

      public GameObject SetContentRef {
        set{ _ContentPanel = value;}
      }

      private const int DEFAULT_CONTENT_INDEX = 2 ;

      public bool ShowMaskImage {
        get { return _MaskComponent.showMaskGraphic;}
        set { _MaskComponent.showMaskGraphic = value;}
      }

      public Sprite MaskImage {
        get { return _MaskImage.sprite;}
        set { _MaskImage.overrideSprite = value;}
      }

      public Sprite BackgroundSprite {
        get { return _BackgroundImage.sprite;}
        set { _BackgroundImage.overrideSprite = value;}
      }

      public Image BackgroundImage {
        get { return _BackgroundImage;}
        set { _BackgroundImage = value;}
      }

      public float Alpha {
        get { return GetComponent<CanvasGroup>().alpha;}
        set { GetComponent<CanvasGroup>().alpha = value;}
      }

      public GameObject ContentPanel {
        get { return _ContentPanel;}
      }
      
      public void Hide() {
        gameObject.SetActive(false);
      }

      public void Show() {
        gameObject.SetActive(true);
      }

      public void MoveContentToTop() {
        _ContentPanel.transform.SetAsFirstSibling();
      }

      public void MoveContentToMiddle() {
        _ContentPanel.transform.SetSiblingIndex(DEFAULT_CONTENT_INDEX);
      }

    }
  }
}
