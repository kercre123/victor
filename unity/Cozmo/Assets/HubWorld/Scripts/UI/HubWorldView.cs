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
    private Asteroid _HubWorldLockedAsteroidPrefab;

    [SerializeField]
    private Transform _LockedButtonContainer;

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

      // Create an asteroid for this challenge
      CreateRandomAsteroid();

      // Add two more in random quadrants for funsies
      for (int i = 0; i < 2; i++) {
        CreateRandomAsteroid();
      }
    }

    private void CreateRandomAsteroid() {

      float apogee = Random.Range(500f, 750f);
      float perigee = Random.Range(750f, 1000f);
      float orbitLength = 120f;
      float rotationSpeed = Random.Range(10f, 30f);
      float startingAngle = Random.Range(0f, 360f);
      Vector3 apogeeDirection = Random.insideUnitCircle.normalized;
      CreateAsteroid(apogee, perigee, orbitLength, rotationSpeed, startingAngle, apogeeDirection);
    }

    private void CreateAsteroid(float apogee, float perigee, float orbitLength, float rotationSpeed, float startingAngle, Vector3 apogeeDirection) {
      GameObject newButton = UIManager.CreateUIElement(_HubWorldLockedAsteroidPrefab.gameObject, _LockedButtonContainer);
      Asteroid asteroid = newButton.GetComponent<Asteroid>();

      asteroid.Initialize(apogee, perigee, orbitLength, rotationSpeed, startingAngle, apogeeDirection);
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