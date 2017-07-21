using Cozmo.Challenge;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using DataPersistence;

namespace Cozmo.Needs.Activities.UI {
  public class FreeplayCard : MonoBehaviour {

    [SerializeField]
    private CozmoButton _SlideLeft;

    [SerializeField]
    private CozmoButton _SlideRight;

    private ActivitiesView _ActivityView;

    public void Initialize(ActivitiesView activityView) {
      _ActivityView = activityView;
      _SlideLeft.Initialize(HandleSlideLeftClicked, "freeplay_slide_button", "play_freeplay_card");
      _SlideRight.Initialize(HandleSlideRightClicked, "freeplay_slide_button", "play_freeplay_card");
    }

    // used by activity view to toggle arrow as it scrolls, more meaningful when both arrows were in the same place 
    internal void SlideArrowFaceLeft(bool left) {
      _SlideLeft.gameObject.SetActive(left);
      _SlideRight.gameObject.SetActive(!left);
    }

    private void HandleSlideLeftClicked() {
      DataPersistenceManager.Instance.Data.DefaultProfile.HideFreeplayCard = false;
      _ActivityView.ShowFreeplayCard();
    }

    private void HandleSlideRightClicked() {
      DataPersistenceManager.Instance.Data.DefaultProfile.HideFreeplayCard = true;
      _ActivityView.HideFreeplayCard();
    }

  }
}