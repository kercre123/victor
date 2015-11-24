using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class HubWorldButton : MonoBehaviour {
  public delegate void ButtonClickedHandler(string challengeClickedId);

  public event ButtonClickedHandler OnButtonClicked;

  [SerializeField]
  private Button _ButtonScript;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _ButtonLabel;

  private string _ChallengeId;

  public virtual void Initialize(ChallengeData challengeData) {
    
    _ChallengeId = challengeData.ChallengeID;
    _ButtonScript.onClick.AddListener(HandleButtonClicked);

    // Allow for buttons that only show an image and no text
    if (_ButtonLabel != null) {
      _ButtonLabel.text = Localization.Get(challengeData.ChallengeTitleKey);
    }
  }

  private void HandleButtonClicked() {
    RaiseButtonClicked(_ChallengeId);
  }

  private void RaiseButtonClicked(string challenge) {
    if (OnButtonClicked != null) { 
      OnButtonClicked(challenge);
    }
  }

}
