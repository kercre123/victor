using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class HubWorldButton : MonoBehaviour {
  public delegate void ButtonClickedHandler(ChallengeData challengeClicked);

  public event ButtonClickedHandler OnButtonClicked;

  [SerializeField]
  private Button _ButtonScript;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _ButtonLabel;

  private ChallengeData _Challenge;

  private void RaiseButtonClicked(ChallengeData challenge) {
    if (OnButtonClicked != null) { 
      OnButtonClicked(challenge);
    }
  }

  public void Initialize(ChallengeData challenge) {
    _Challenge = challenge;
    _ButtonScript.onClick.AddListener(HandleButtonClicked);
    _ButtonLabel.text = Localization.Get(challenge.ChallengeTitleKey);
  }

  private void HandleButtonClicked() {
    RaiseButtonClicked(_Challenge);
  }

}
