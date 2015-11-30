using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using DG.Tweening;

namespace Cozmo.HubWorld {
  public class HubWorldView : MonoBehaviour {

    public delegate void ButtonClickedHandler(string challengeClicked,Transform buttonTransform);

    public event ButtonClickedHandler OnLockedChallengeClicked;
    public event ButtonClickedHandler OnUnlockedChallengeClicked;
    public event ButtonClickedHandler OnCompletedChallengeClicked;

    [SerializeField]
    private Canvas _HubWorldCanvas;

    [SerializeField]
    private HubWorldButton _HubWorldLockedButtonPrefab;

    [SerializeField]
    private RectTransform _LockedButtonContainer;

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

      // Make sure we recieve events from the right camera
      _HubWorldCanvas.worldCamera = HubWorldCamera.Instance.WorldCamera;
    }

    private void CreateLockedButton(ChallengeData challengeData) {
      // For each locked challenge, create 1 asteroids in each quadrant
      // Right now we're not doing anything on locked challenge tap so this is okay.
      int numAsteroids = 4;

      for (int i = 0; i < numAsteroids; i++) {
        CreateAsteroidInQuadrant(i % 4);
      }

      // Add two more in random quadrants for funsies
      for (int i = 0; i < 2; i++) {
        CreateAsteroidInQuadrant(Random.Range(0, 4));
      }
    }

    private void CreateAsteroidInQuadrant(int quadrant) {
      switch (quadrant) {
      case 0: 
        CreateAsteroid(-950, -500, -460, 0);
        break;
      case 1:
        CreateAsteroid(-950, -500, 0, 510);
        break;
      case 2:
        CreateAsteroid(500, 950, -460, 0);
        break;
      case 3:
        CreateAsteroid(500, 950, 0, 510);
        break;
      default:
        break;
      }
    }

    private void CreateAsteroid(float minLocalX, float maxLocalX, float minLocalY, float maxLocalY) {
      GameObject newButton = UIManager.CreateUIElement(_HubWorldLockedButtonPrefab.gameObject, _LockedButtonContainer);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      newButton.transform.localPosition = new Vector3(Random.Range(minLocalX, maxLocalX),
        Random.Range(minLocalY, maxLocalY), Random.Range(-75, 75));

      buttonScript.Initialize(null);
      //buttonScript.OnButtonClicked += HandleLockedChallengeClicked;
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

    private void HandleLockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnLockedChallengeClicked != null) {
        OnLockedChallengeClicked(challengeClicked, buttonTransform);
      }
    }

    private void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnUnlockedChallengeClicked != null) {
        OnUnlockedChallengeClicked(challengeClicked, buttonTransform);
      }
    }

    private void HandleCompletedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnCompletedChallengeClicked != null) {
        OnCompletedChallengeClicked(challengeClicked, buttonTransform);
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
}