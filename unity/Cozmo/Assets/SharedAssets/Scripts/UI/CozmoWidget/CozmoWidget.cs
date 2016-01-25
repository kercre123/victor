using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;

public class CozmoWidget : BaseView {

  [SerializeField]
  Text _CozmoFriendshipStatusText;

  public void UpdateFriendshipText(string friendshipStatus) {
    _CozmoFriendshipStatusText.text = friendshipStatus;
  }

  protected override void CleanUp() {
    
  }
}
