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

    private string[] _AcknowledgementTexts;

    public void Initialize(string titleText, string filePath) {
      _TitleText.text = titleText;

      Object[] acknowledgementsTexts = Resources.LoadAll(filePath, typeof(TextAsset));

      // Slides must actually init the prefabs and attach them, we'll populate them later.
      GameObject[] slides = new GameObject[acknowledgementsTexts.Length];
      _AcknowledgementTexts = new string[acknowledgementsTexts.Length];
      for (int i = 0; i < slides.Length; ++i) {
        slides[i] = _AcknowledgementSlidePrefab.gameObject;
        _AcknowledgementTexts[i] = (acknowledgementsTexts[i] as TextAsset).text;
      }
      _SwipeSlides.OnSlideCreated += HandleSlideCreated;
      _SwipeSlides.Initialize(slides);
    }

    private void HandleSlideCreated(GameObject slide, int slideIndex) {
      AnkiInfiniteScrollView scrollInst = slide.GetComponent<AnkiInfiniteScrollView>();
      scrollInst.SetString(_AcknowledgementTexts[slideIndex]);
    }

  }
}
