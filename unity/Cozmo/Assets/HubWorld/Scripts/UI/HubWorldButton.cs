using UnityEngine;
using System.Collections;
using UnityEngine.UI;

namespace Cozmo.HubWorld {
  public class HubWorldButton : MonoBehaviour {
    public delegate void ButtonClickedHandler(string challengeClickedId,Transform buttonTransform);

    public event ButtonClickedHandler OnButtonClicked;

    [SerializeField]
    private Button _ButtonScript;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _ButtonLabel;

    private string _ChallengeId;

    public virtual void Initialize(ChallengeData challengeData) {
    
      if (challengeData != null) {
        _ChallengeId = challengeData.ChallengeID;
      }
      _ButtonScript.onClick.AddListener(HandleButtonClicked);

      // Allow for buttons that only show an image and no text
      if (_ButtonLabel != null) {
        _ButtonLabel.text = Localization.Get(challengeData.ChallengeTitleLocKey);
      }
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