using Cozmo.Challenge;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using DataPersistence;

namespace Cozmo.Needs.Sparks.UI {
  public class FreeplayCard : MonoBehaviour {

    [SerializeField]
    private CozmoButton _SlideLeft;

    [SerializeField]
    private CozmoButton _SlideRight;

    private SparksView _SparksView;

    public void Initialize(SparksView sparksView) {
      _SparksView = sparksView;
      _SlideLeft.Initialize(HandleSlideLeftClicked, "freeplay_slide_button", "play_freeplay_card");
      _SlideRight.Initialize(HandleSlideRightClicked, "freeplay_slide_button", "play_freeplay_card");
    }

    internal void ShowSlideLeftArrow() {
      _SlideLeft.gameObject.SetActive(true);
      _SlideRight.gameObject.SetActive(false);
    }

    internal void ShowSlideRightArrow() {
      _SlideLeft.gameObject.SetActive(false);
      _SlideRight.gameObject.SetActive(true);
    }

    private void HandleSlideLeftClicked() {
      DataPersistenceManager.Instance.Data.DefaultProfile.HideFreeplayCard = false;
      _SparksView.ShowFreeplayCard();
    }

    private void HandleSlideRightClicked() {
      DataPersistenceManager.Instance.Data.DefaultProfile.HideFreeplayCard = true;
      _SparksView.HideFreeplayCard();
    }
  }
}