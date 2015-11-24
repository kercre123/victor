using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class DevHubWorldButton : MonoBehaviour {
  
  public delegate void DevButtonClickedHandler(ChallengeData challenge);

  public event DevButtonClickedHandler OnDevButtonClicked;

  private void RaiseButtonClicked(ChallengeData challenge) {
    if (OnDevButtonClicked != null) { 
      OnDevButtonClicked(challenge);
    }
  }

  [SerializeField]
  private Button _ButtonScript;

  [SerializeField]
  private Text _ButtonLabel;

  private ChallengeData _Challenge;

  public void Initialize(ChallengeData challenge) {
    _Challenge = challenge;

    string titleKey = string.Format("#{0}.title", challenge.ChallengeTitleKey);
    _ButtonLabel.text = titleKey;
    gameObject.name = string.Format("{0}: {1}", challenge.ChallengeTitleKey, _ButtonLabel.text);

    _ButtonScript.onClick.AddListener(HandleButtonClicked);
  }

  private void OnDestroy() {
    _ButtonScript.onClick.RemoveListener(HandleButtonClicked);
  }

  private void HandleButtonClicked() {
    RaiseButtonClicked(_Challenge);
  }
}
