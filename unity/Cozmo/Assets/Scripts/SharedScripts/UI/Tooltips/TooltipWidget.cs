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

    // Background image
    [SerializeField]
    private RectTransform _VertOffsetTransform;

    public bool IsInited { get; set; }

    public enum CaretPosition {
      Up,
      Down,
      Left,
      Right,
      UpLeft,
      DownRight,
    }

    private IEnumerator InitInternal(string header, string body, Vector2 localPos, CaretPosition preferredDir) {
      _HeaderText.text = header;
      _BodyText.text = body;
      // Wait for end of frame so layout has time to resize before we set at correct position
      yield return new WaitForEndOfFrame();

      RectTransform rectTransform = GetComponent<RectTransform>();
      rectTransform.localPosition = localPos;
      IsInited = true;
      yield return null;
    }

    public void Init(string header, string body, Vector2 localPos, CaretPosition preferredDir) {
      IsInited = false;
      StartCoroutine(InitInternal(header, body, localPos, preferredDir));
    }


  }

}