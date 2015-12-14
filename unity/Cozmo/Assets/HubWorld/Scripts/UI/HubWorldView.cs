using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

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

    private readonly Dictionary<string, GameObject> _ChallengeButtons = new Dictionary<string, GameObject>();


    // Using my own tweening because I couldn't figure out how to make 
    // DOTween work properly
    private class Tween {
      public System.Action<float> TweenAction;
      public System.Action CompleteAction;
      public Tween(System.Action<float> tweenAction, 
                    System.Action completeAction = null) {
        TweenAction = tweenAction;
        CompleteAction = completeAction;
      }
    }

    private const float _TweenLength = 3f;
    private float _TweenProgress;
    private readonly List<Tween> _Tweeners = new List<Tween>();

    /// <summary>
    /// Reuse the same seed until the unlocked challenges have changed
    /// </summary>
    private static int sSeed = 0;

    public void Initialize(Dictionary<string, ChallengeStatePacket> _challengeStatesById) {

      _ChallengeButtons.Clear();

      _TweenProgress = 0;

      bool animates = false;
      // For all the challenges
      foreach (ChallengeStatePacket challengeState in _challengeStatesById.Values) {
        // Create the correct button at the correct spot based on current state
        switch (challengeState.currentState) {
        case ChallengeState.FRESHLY_UNLOCKED:
          animates = true;
          goto case ChallengeState.LOCKED;
        case ChallengeState.LOCKED:
          _ChallengeButtons[challengeState.data.ChallengeID] = 
            CreateLockedButton(challengeState.data);
          break;
        case ChallengeState.FRESHLY_COMPLETED:
          animates = true;
          goto case ChallengeState.UNLOCKED;
        case ChallengeState.UNLOCKED:   
          _ChallengeButtons[challengeState.data.ChallengeID] = 
            CreateUnlockedButton(challengeState.data, challengeState.unlockProgress);
          break;
        case ChallengeState.COMPLETED:
          _ChallengeButtons[challengeState.data.ChallengeID] = 
            CreateCompletedButton(challengeState.data);
          break; 
        default:
          DAS.Error("HubWorldView", "ChallengeState view not implemented! " + challengeState);
          break;
        }
      }

      LayoutUnlockedChallenges();
      LayoutCompletedChallenges();

      if (animates) {        

        foreach (ChallengeStatePacket challengeState in _challengeStatesById.Values) {
          GameObject oldButton = null, newButton = null;
          // Create the correct button at the correct spot based on current state
          switch (challengeState.currentState) {
          case ChallengeState.FRESHLY_UNLOCKED:
            
            oldButton = _ChallengeButtons[challengeState.data.ChallengeID]; 
            newButton = CreateUnlockedButton(challengeState.data, challengeState.unlockProgress);
            _ChallengeButtons[challengeState.data.ChallengeID] = newButton;

            newButton.transform.localPosition = oldButton.transform.localPosition;

            newButton.GetComponent<HubWorldButton>().enabled = false;
            oldButton.GetComponent<Asteroid>().enabled = false;

            _Tweeners.Add(new Tween((t) => {

              // Expand to 2x, then shrink to zero
              if( t < 0.5f) {
                oldButton.transform.localScale = Vector3.Lerp(Vector3.one, Vector3.one * 5, t);
              }
              else {
                oldButton.transform.localScale = Vector3.Lerp(Vector3.one * 4, Vector3.zero, t);
              }
              newButton.transform.localScale = Vector3.Lerp(Vector3.zero, Vector3.one, t);
            }, () => { 
              GameObject.Destroy(oldButton); 
              newButton.GetComponent<HubWorldButton>().enabled = true;
            }));

            challengeState.currentState = ChallengeState.UNLOCKED;
            break;
          case ChallengeState.FRESHLY_COMPLETED:   
            oldButton = _ChallengeButtons[challengeState.data.ChallengeID];
            newButton = CreateCompletedButton(challengeState.data);
            _ChallengeButtons[challengeState.data.ChallengeID] = newButton;
            challengeState.currentState = ChallengeState.COMPLETED;
            _UnlockedButtons.Remove(oldButton.GetComponent<RectTransform>());

            newButton.transform.localPosition = oldButton.transform.localPosition;

            var oldGroup = oldButton.GetComponent<CanvasGroup>();
            var newGroup = newButton.GetComponent<CanvasGroup>();

            oldButton.GetComponent<HubWorldButton>().enabled = false;
            newButton.GetComponent<HubWorldButton>().enabled = false;
            const float relativeScale = 2.5f;
            newButton.transform.localScale = Vector3.one * relativeScale;

            oldGroup.alpha = 1f;
            newGroup.alpha = 0f;

            _Tweeners.Add(new Tween((t) => {

              oldGroup.alpha = 1 - t;
              newGroup.alpha = t;

              oldGroup.transform.localScale = Vector3.Lerp(Vector3.one, Vector3.one / relativeScale, t);
              oldButton.transform.localPosition = newButton.transform.localPosition;

            }, () => { 
              GameObject.Destroy(oldButton); 
              newButton.GetComponent<HubWorldButton>().enabled = true;
            }));

            break;
          }

        }

        sSeed = (int)System.DateTime.UtcNow.Ticks;

        LayoutUnlockedChallenges(false);
        LayoutCompletedChallenges(false);
      }

      // Make sure we recieve events from the right camera
      _HubWorldCanvas.worldCamera = HubWorldCamera.Instance.WorldCamera;
    }

    private GameObject CreateLockedButton(ChallengeData challengeData) {

      // Create an asteroid for this challenge
      var result = CreateRandomAsteroid();

      // Add two more in random quadrants for funsies
      for (int i = 0; i < 2; i++) {
        CreateRandomAsteroid();
      }
      return result;
    }

    private GameObject CreateRandomAsteroid() {

      float apogee = Random.Range(500f, 750f);
      float perigee = Random.Range(750f, 1000f);
      float orbitLength = 240f;
      float rotationSpeed = Random.Range(10f, 30f);
      float startingAngle = Random.Range(0f, 360f);
      Vector3 apogeeDirection = Random.insideUnitCircle.normalized;
      return CreateAsteroid(apogee, perigee, orbitLength, rotationSpeed, startingAngle, apogeeDirection);
    }

    private GameObject CreateAsteroid(float apogee, float perigee, float orbitLength, float rotationSpeed, float startingAngle, Vector3 apogeeDirection) {
      GameObject newButton = UIManager.CreateUIElement(_HubWorldLockedAsteroidPrefab.gameObject, _LockedButtonContainer);
      Asteroid asteroid = newButton.GetComponent<Asteroid>();

      asteroid.Initialize(apogee, perigee, orbitLength, rotationSpeed, startingAngle, apogeeDirection);

      return newButton;
    }

    private GameObject CreateUnlockedButton(ChallengeData challengeData, float unlockProgress) {      
      GameObject newButton = UIManager.CreateUIElement(_HubWorldUnlockedButtonPrefab.gameObject, _ChallengeButtonContainer);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeData);
      buttonScript.OnButtonClicked += HandleUnlockedChallengeClicked;
      _UnlockedButtons.Add(newButton.GetComponent<RectTransform>());
      return newButton;
    }

    private GameObject CreateCompletedButton(ChallengeData challengeData) {
      // TODO: The Completed button visuals are going to change, so don't worry about copy pasta
      GameObject newButton = UIManager.CreateUIElement(_HubWorldCompletedButtonPrefab.gameObject, _ChallengeButtonContainer);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeData);
      buttonScript.OnButtonClicked += HandleCompletedChallengeClicked;
      _CompletedButtons.Add(newButton.GetComponent<RectTransform>());
      return newButton;
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

    private void LayoutUnlockedChallenges(bool immediate = true) {
      if (_UnlockedButtons.Count == 0) {
        return;
      }
      // Set the seed so the layout is the same until the layout changes
      Random.seed = sSeed;

      // TODO: grow this?
      const float radius = 400f;

      float anglePerButton = 360 / _UnlockedButtons.Count;

      // add a bit of randomness to the positions to feel more organic
      float wiggleRoom = Mathf.Max(0f, anglePerButton * 0.5f - 15f);

      for (int i = 0; i < _UnlockedButtons.Count; i++) {
        float currentAngle = anglePerButton * i + Random.Range(-wiggleRoom, wiggleRoom);

        var position = new Vector3(
          Mathf.Cos(currentAngle * Mathf.Deg2Rad), 
          Mathf.Sin(currentAngle * Mathf.Deg2Rad), 
          0) * radius;

        if (immediate) {
          _UnlockedButtons[i].localPosition = position;
        }
        else {
          MakeMoveTween(_UnlockedButtons[i], position, Vector3.one);
        }
      }
    }

    private void LayoutCompletedChallenges(bool immediate = true) {
      if (_CompletedButtons.Count == 0) {
        return;
      }

      // Set the seed so the layout is the same until the layout changes
      Random.seed = sSeed;


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
        // add a small amount of variation to button scale
        var scale = Random.Range(0.8f, 1.1f) * Vector3.one;

        if (immediate) {
          _CompletedButtons[i].localPosition = position;
          _CompletedButtons[i].localScale = scale;
        }
        else {          
          MakeMoveTween(_CompletedButtons[i], position, scale);
        }

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
        if (immediate) {          
          _CompletedButtons[i].localPosition -= center;
        }
        else {
          // This is kind of annoying, but it was the easiest way
          // to get what I wanted without rewriting the whole thing
          MakeConstantMoveTween(_CompletedButtons[i], -center);
        }
      }
    }

    private void MakeMoveTween(RectTransform rectTransform, Vector3 position, Vector3 scale) {
      var startPosition = rectTransform.localPosition;
      var startScale = rectTransform.localScale;

      _Tweeners.Add(new Tween((t) => {
        rectTransform.localPosition = Vector3.Lerp(startPosition, position, t);
        rectTransform.localScale = Vector3.Lerp(startScale, scale, t);
      }));
    }

    private void MakeConstantMoveTween(RectTransform rectTransform, Vector3 delta) {
      _Tweeners.Add(new Tween((t) => {
        rectTransform.localPosition += delta;
      }));
    }

    private void Update() {

      if (_TweenProgress < _TweenLength) {
        _TweenProgress += Time.deltaTime;

        bool complete = _TweenProgress >= _TweenLength;

        for (int i = 0; i < _Tweeners.Count; i++) {
          _Tweeners[i].TweenAction(Mathf.Clamp01(_TweenProgress / _TweenLength));

          if (complete && _Tweeners[i].CompleteAction != null) {
            _Tweeners[i].CompleteAction();
          }
        }
        if (complete) {
          _Tweeners.Clear();
        }
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