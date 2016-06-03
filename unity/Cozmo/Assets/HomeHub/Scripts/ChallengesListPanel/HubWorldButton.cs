using UnityEngine;
using System.Collections;
using UnityEngine.UI;

namespace Cozmo.HubWorld {
  public class HubWorldButton : MonoBehaviour {
    public delegate void ButtonClickedHandler(string challengeClickedId, Transform buttonTransform);

    public event ButtonClickedHandler OnButtonClicked;

    [SerializeField]
    private Cozmo.UI.CozmoButton _ButtonScript;

    [SerializeField]
    private Image _IconImage;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _ChallengeTitle;

    [SerializeField]
    private Image _CircuitImage;

    private string _ChallengeId;

    public virtual void Initialize(ChallengeData challengeData, string dasParentViewName, Sprite circuitSprite, bool isEnd = false) {
      if (challengeData != null) {
        _ChallengeId = challengeData.ChallengeID;
        _IconImage.overrideSprite = challengeData.ChallengeIcon;
        _ChallengeTitle.text = Localization.Get(challengeData.ChallengeTitleLocKey);
      }

      _CircuitImage.sprite = circuitSprite;

      if (isEnd) {
        _CircuitImage.enabled = false;
      }
      _ButtonScript.Initialize(HandleButtonClicked, string.Format("see_{0}_details_button", _ChallengeId), dasParentViewName);
    }

    private void Update() {
      OnUpdate();
    }

    public void SetIsEnd(bool endCap) {
      _CircuitImage.enabled = !endCap;
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