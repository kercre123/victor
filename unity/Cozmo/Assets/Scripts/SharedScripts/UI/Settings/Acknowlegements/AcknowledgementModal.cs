using Anki.Cozmo.ExternalInterface;
using Anki.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Settings {
  public class AcknowledgementModal : BaseModal {
    [SerializeField]
    private SwipeSlides _SwipeSlides;

    [SerializeField]
    private AnkiInfiniteScrollView _AcknowledgementSlidePrefab;

    [SerializeField]
    private CozmoText _TitleText;

    public void Initialize(string titleText, string filePath) {
      _TitleText.text = titleText;

      Object[] acknowledgementsTexts = Resources.LoadAll(filePath, typeof(TextAsset));

      // Slides must actually init the prefabs and attach them, we'll populate them later.
      GameObject[] slides = new GameObject[acknowledgementsTexts.Length];
      for (int i = 0; i < slides.Length; ++i) {
        slides[i] = _AcknowledgementSlidePrefab.gameObject;
      }
      _SwipeSlides.Initialize(slides);
      for (int i = 0; i < slides.Length; ++i) {
        GameObject go = _SwipeSlides.GetSlideInstanceAt(i);
        AnkiInfiniteScrollView scrollInst = go.GetComponent<AnkiInfiniteScrollView>();
        TextAsset textAsset = acknowledgementsTexts[i] as TextAsset;
        if (textAsset != null) {
          scrollInst.SetString(textAsset.text);
        }
      }
    }
  }
}
