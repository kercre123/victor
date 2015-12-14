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
    private RectTransform _ChallengeButtonContainer;

    private List<RectTransform> _UnlockedButtons = new List<RectTransform>();

    [SerializeField]
    private HubWorldButton _HubWorldCompletedButtonPrefab;

    private List<RectTransform> _CompletedButtons = new List<RectTransform>();


    public void Initialize(Dictionary<string, ChallengeStatePacket> _challengeStatesById) {

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
      float orbitLength = 240f;
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
      GameObject newButton = UIManager.CreateUIElement(_HubWorldUnlockedButtonPrefab.gameObject, _ChallengeButtonContainer);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeData);
      buttonScript.OnButtonClicked += HandleUnlockedChallengeClicked;
      _UnlockedButtons.Add(newButton.GetComponent<RectTransform>());
      LayoutUnlockedChallenges();
    }

    private void CreateCompletedButton(ChallengeData challengeData) {
      // TODO: The Completed button visuals are going to change, so don't worry about copy pasta
      GameObject newButton = UIManager.CreateUIElement(_HubWorldCompletedButtonPrefab.gameObject, _ChallengeButtonContainer);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeData);
      buttonScript.OnButtonClicked += HandleCompletedChallengeClicked;
      _CompletedButtons.Add(newButton.GetComponent<RectTransform>());
      LayoutCompletedChallenges();
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

    private void LayoutUnlockedChallenges() {
      if (_UnlockedButtons.Count == 0) {
        return;
      }

      // TODO: grow this?
      const float radius = 400f;

      float anglePerButton = 360 / _UnlockedButtons.Count;

      // add a bit of randomness to the positions to feel more organic
      float wiggleRoom = Mathf.Max(0f, anglePerButton * 0.5f - 15f);

      for (int i = 0; i < _UnlockedButtons.Count; i++) {
        float currentAngle = anglePerButton * i + Random.Range(-wiggleRoom, wiggleRoom);

        _UnlockedButtons[i].localPosition = new Vector3(
                      Mathf.Cos(currentAngle * Mathf.Deg2Rad), 
                      Mathf.Sin(currentAngle * Mathf.Deg2Rad), 
                      0) * radius;
      }
    }

    private void LayoutCompletedChallenges() {
      if (_CompletedButtons.Count == 0) {
        return;
      }

      const float length = 40f;
      const float wiggleRoom = 10f;
      
      // just put them in a hex grid with a little bit of wiggle
      int offset = 0;
      int side = 5;
      int ring = 0;

      const float sqrt3 = 1.73205080757f;

      const float verticalOffset = 1.5f;

      const float horizontalOffset = sqrt3;

      Vector3 totalPositions = Vector3.zero;
      for (int i = 0; i < _CompletedButtons.Count; i++) {

        // this will produce { 2, 1, -1, -2, -1, 1 }
        int v = (side % 3 == 0 ? 2 : 1) * (((side + 1) % 6) < 3 ? 1 : -1);
        // this will produce { 0, 1, 1, 0, -1, -1 }
        int h = (side % 3 == 0 ? 0 : 1) * ((side % 6) < 3 ? 1 : -1);

        // this will produce { -1, -2, -1, 1, 2, 1 }
        int ov = ((side + 2) % 3 == 0 ? 2 : 1) * (((side + 3) % 6) < 3 ? 1 : -1);
        // this will produce { 1, 0, -1, -1, 0, 1 }
        int oh = ((side + 2) % 3 == 0 ? 0 : 1) * (((side + 2) % 6) < 3 ? 1 : -1);

        Vector3 startingPosition = new Vector3(
                                     (v * ring + ov * offset) * verticalOffset,
                                     (h * ring + oh * offset) * horizontalOffset,
                                     0) * length;

        var position = startingPosition + (Vector3)(wiggleRoom * Random.insideUnitCircle);
        _CompletedButtons[i].localPosition = position;
        // add a small amount of variation to button scale
        _CompletedButtons[i].localScale = Random.Range(0.8f, 1.1f) * Vector3.one;

        totalPositions += position;
        offset++;
        if (offset > ring) {
          offset = 1;
          side++;
          if (side == 6) {
            side = 0;
            ring++;
          }
        }
      }

      var center = totalPositions / _CompletedButtons.Count;

      for (int i = 0; i < _CompletedButtons.Count; i++) {
        _CompletedButtons[i].localPosition -= center;
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