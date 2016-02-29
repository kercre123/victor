using UnityEngine;
using System.Collections;
using UnityEngine.UI;

namespace Cozmo.HubWorld {
  public class HubWorldButton : MonoBehaviour {
    public delegate void ButtonClickedHandler(string challengeClickedId,Transform buttonTransform);

    public event ButtonClickedHandler OnButtonClicked;

    [SerializeField]
    private Anki.UI.AnkiButton _ButtonScript;

    [SerializeField]
    private Image _IconImage;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _ChallengeTitle;

    private string _ChallengeId;

    public virtual void Initialize(ChallengeData challengeData, string dasParentViewName) {
      if (challengeData != null) {
        _ChallengeId = challengeData.ChallengeID;
        _IconImage.overrideSprite = challengeData.ChallengeIcon;
        _ChallengeTitle.text = Localization.Get(challengeData.ChallengeTitleLocKey);
      }
      _ButtonScript.onClick.AddListener(HandleButtonClicked);
      _ButtonScript.DASEventButtonName = string.Format("{0}_button", _ChallengeId);
      _ButtonScript.DASEventViewController = dasParentViewName;
    }

    private void Update() {
      OnUpdate();
    }

    protected virtual void OnUpdate() {
    }

    private void HandleButtonClicked() {
      RaiseButtonClicked(_ChallengeId);
    }

    private void RaiseButtonClicked(string challenge) {
      if (OnButtonClicked != null) { 
        OnButtonClicked(challenge, this.transform);
      }
    }

  }
}