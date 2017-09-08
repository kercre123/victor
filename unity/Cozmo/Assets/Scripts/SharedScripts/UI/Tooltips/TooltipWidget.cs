using System.Collections;
using UnityEngine;

/**

This tooltip widget has a content size fitter so prefab should resize according to content.
*/
namespace Cozmo.UI {
  public class TooltipWidget : MonoBehaviour {

    [SerializeField]
    private CozmoText _HeaderText;

    [SerializeField]
    private CozmoText _BodyText;

    [SerializeField]
    private GameObject[] _Caret;

    [SerializeField]
    private RectTransform _VertOffsetTransform;

    public bool IsInited { get; set; }

    public enum CaretPosition {
      Up,
      Down,
      Left,
      Right
    }

    private IEnumerator InitInternal(string header, string body, Vector2 localPos, CaretPosition preferredDir) {
      _HeaderText.text = header;
      _BodyText.text = body;

      // Wait for end of frame so layout has time to resize and we get the right offset from _VertOffsetTransform
      yield return new WaitForEndOfFrame();

      for (int i = 0; i < _Caret.Length; ++i) {
        _Caret[i].SetActive(false);
      }
      int index = (int)preferredDir;
      _Caret[index].SetActive(true);

      // Move around the tooltip so we always show right at the tip.0
      RectTransform rectTransform = GetComponent<RectTransform>();
      Vector2 offset = new Vector2();
      switch (preferredDir) {
      case CaretPosition.Up: {
          rectTransform.anchorMin = new Vector2(0.5f, 1.0f);
          rectTransform.anchorMax = new Vector2(0.5f, 1.0f);
          rectTransform.pivot = new Vector2(0.5f, 1.0f);
          offset = new Vector2(0, -_Caret[index].GetComponent<RectTransform>().rect.height - _VertOffsetTransform.rect.height / 2);
          _Caret[index].transform.localPosition = -offset;
        }
        break;
      case CaretPosition.Down: {
          rectTransform.anchorMin = new Vector2(0.5f, 0.0f);
          rectTransform.anchorMax = new Vector2(0.5f, 0.0f);
          rectTransform.pivot = new Vector2(0.5f, 0.0f);
          offset = new Vector2(0, _Caret[index].GetComponent<RectTransform>().rect.height + _VertOffsetTransform.rect.height / 2);
          _Caret[index].transform.localPosition = -offset;
        }
        break;
      case CaretPosition.Left: {
          rectTransform.anchorMin = new Vector2(0.0f, 0.5f);
          rectTransform.anchorMax = new Vector2(0.0f, 0.5f);
          rectTransform.pivot = new Vector2(0.0f, 0.5f);
          offset = new Vector2(_Caret[index].GetComponent<RectTransform>().rect.width, 0);
        }
        break;
      case CaretPosition.Right: {
          rectTransform.anchorMin = new Vector2(1.0f, 0.5f);
          rectTransform.anchorMax = new Vector2(1.0f, 0.5f);
          rectTransform.pivot = new Vector2(1.0f, 0.5f);
          offset = new Vector2(-_Caret[index].GetComponent<RectTransform>().rect.width, 0);
        }
        break;
      }

      rectTransform.localPosition = localPos + offset;
      IsInited = true;
      yield return null;
    }

    public void Init(string header, string body, Vector2 localPos, CaretPosition preferredDir) {
      IsInited = false;
      StartCoroutine(InitInternal(header, body, localPos, preferredDir));
    }


  }

}