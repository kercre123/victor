using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class HubWorldView : MonoBehaviour {

  public delegate void ButtonClickedHandler(string challengeClicked);

  public event ButtonClickedHandler OnLockedChallengeClicked;
  public event ButtonClickedHandler OnUnlockedChallengeClicked;
  public event ButtonClickedHandler OnCompletedChallengeClicked;

  [SerializeField]
  private HubWorldButton _HubWorldLockedButtonPrefab;

  [SerializeField]
  private RectTransform[] _LockedButtonNodes;

  private int _LockedButtonCounter;

  [SerializeField]
  private HubWorldButton _HubWorldUnlockedButtonPrefab;

  [SerializeField]
  private RectTransform[] _UnlockedButtonNodes;

  private int _UnlockedButtonCounter;

  [SerializeField]
  private HubWorldButton _HubWorldCompletedButtonPrefab;

  [SerializeField]
  private RectTransform[] _CompletedButtonNodes;

  private int _CompletedButtonCounter;

  public void Initialize(Dictionary<string, ChallengeStatePacket> _challengeStatesById) {
    _LockedButtonCounter = 0;
    _UnlockedButtonCounter = 0;
    _CompletedButtonCounter = 0;

    // For all the challenges
    foreach (ChallengeStatePacket challengeState in _challengeStatesById.Values) {
      // Create the correct button at the correct spot based on current state
      switch (challengeState.currentState) {
      case ChallengeState.LOCKED:
        CreateLockedButton(challengeState.data);
        break;
      case ChallengeState.UNLOCKED:
        CreateUnlockedButton(challengeState.data, challengeState.unlockProgress);
        break;
      case ChallengeState.COMPLETED:
        CreateCompletedButton(challengeState.data);
        break;
      default:
        DAS.Error("HubWorldView", "ChallengeState view not implemented! " + challengeState);
        break;
      }
    }
  }

  private void CreateLockedButton(ChallengeData challengeData) {
    // TODO: The Unlocked button visuals are going to change, so don't worry about copy pasta
    if (_LockedButtonCounter >= 0 && _LockedButtonCounter < _LockedButtonNodes.Length) {
      GameObject newButton = UIManager.CreateUIElement(_HubWorldLockedButtonPrefab.gameObject, _LockedButtonNodes[_LockedButtonCounter]);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeData);
      buttonScript.OnButtonClicked += HandleLockedChallengeClicked;
      _LockedButtonCounter++;
    }
  }

  private void CreateUnlockedButton(ChallengeData challengeData, float unlockProgress) {
    if (_UnlockedButtonCounter >= 0 && _UnlockedButtonCounter < _UnlockedButtonNodes.Length) {
      GameObject newButton = UIManager.CreateUIElement(_HubWorldUnlockedButtonPrefab.gameObject, _UnlockedButtonNodes[_UnlockedButtonCounter]);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeData);
      buttonScript.OnButtonClicked += HandleUnlockedChallengeClicked;
      _UnlockedButtonCounter++;
    }
  }

  private void CreateCompletedButton(ChallengeData challengeData) {
    // TODO: The Completed button visuals are going to change, so don't worry about copy pasta
    if (_CompletedButtonCounter >= 0 && _CompletedButtonCounter < _CompletedButtonNodes.Length) {
      GameObject newButton = UIManager.CreateUIElement(_HubWorldCompletedButtonPrefab.gameObject, _CompletedButtonNodes[_CompletedButtonCounter]);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeData);
      buttonScript.OnButtonClicked += HandleCompletedChallengeClicked;
      _CompletedButtonCounter++;
    }
  }

  private void HandleLockedChallengeClicked(string challengeClicked) {
    if (OnLockedChallengeClicked != null) {
      OnLockedChallengeClicked(challengeClicked);
    }
  }

  private void HandleUnlockedChallengeClicked(string challengeClicked) {
    if (OnUnlockedChallengeClicked != null) {
      OnUnlockedChallengeClicked(challengeClicked);
    }
  }

  private void HandleCompletedChallengeClicked(string challengeClicked) {
    if (OnCompletedChallengeClicked != null) {
      OnCompletedChallengeClicked(challengeClicked);
    }
  }

  public void CloseView() {
    // TODO: Play some close animations before destroying view
    GameObject.Destroy(gameObject);
  }

  public void CloseViewImmediately() {
    GameObject.Destroy(gameObject);
  }
}
