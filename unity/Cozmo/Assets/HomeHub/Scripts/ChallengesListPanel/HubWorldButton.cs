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
    private Cozmo.UI.IconProxy _IconProxy;

    [SerializeField]
    private Transform _IconContainer;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _ChallengeTitle;

    private string _ChallengeId;

    [SerializeField]
    private GameObject _NewUnlockIndicator;

    public virtual void Initialize(ChallengeData challengeData, string dasParentViewName, bool isEnd = false, bool isNew = false) {
      if (challengeData != null) {
        _ChallengeId = challengeData.ChallengeID;
        _IconProxy.SetIcon(challengeData.ChallengeIcon);
        _ChallengeTitle.text = Localization.Get(challengeData.ChallengeTitleLocKey);
      }

      _NewUnlockIndicator.SetActive(isNew);

      _ButtonScript.Initialize(HandleButtonClicked, string.Format("see_{0}_details_button", _ChallengeId), dasParentViewName);
      _ButtonScript.onPress.AddListener(HandlePointerDown);
      _ButtonScript.onRelease.AddListener(HandlePointerUp);
    }

    private void HandlePointerDown() {
      _IconContainer.localScale = new Vector3(0.9f, 0.9f, 1.0f);
    }

    private void HandlePointerUp() {
      _IconContainer.localScale = new Vector3(1.0f, 1.0f, 1.0f);
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