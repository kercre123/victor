using Anki.UI;
using UnityEngine;

namespace Cozmo.UI {
  public class ChallengeDetailsUnlock : MonoBehaviour {

    [SerializeField]
    private AnkiTextLegacy _DescriptionLabel;

    [SerializeField]
    private RectTransform _UnlockGameContainer;

    [SerializeField]
    private RectTransform _UnlockSkillContainer;

    public string Text {
      get { return _DescriptionLabel.text; }
      set { _DescriptionLabel.text = value; }
    }

    public bool UnlockGameContainerEnabled {
      get { return _UnlockGameContainer.gameObject.activeSelf; }
      set { _UnlockGameContainer.gameObject.SetActive(value); }
    }

    public bool UnlockSkillContainerEnabled {
      get { return _UnlockSkillContainer.gameObject.activeSelf; }
      set { _UnlockSkillContainer.gameObject.SetActive(value); }
    }
  }
}