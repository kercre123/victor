using UnityEngine;
using System.Collections;

namespace Cozmo.UI {

  public class ChallengeIconProxy : IconProxy {

    [SerializeField]
    private Sprite _ActivitySprite;

    [SerializeField]
    private Sprite _GameSprite;

    public void SetChallengeIconAsGame(bool isGame) {
      if (isGame) {
        _BackgroundImage.sprite = _GameSprite;
      }
      else {
        _BackgroundImage.sprite = _ActivitySprite;
      }

    }
  }

}