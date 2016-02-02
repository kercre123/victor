using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;

public class CozmoWidget : MonoBehaviour {

  [SerializeField]
  Text _CozmoFriendshipStatusText;

  public void UpdateFriendshipText(string friendshipStatus) {
    _CozmoFriendshipStatusText.text = friendshipStatus;
  }
}
