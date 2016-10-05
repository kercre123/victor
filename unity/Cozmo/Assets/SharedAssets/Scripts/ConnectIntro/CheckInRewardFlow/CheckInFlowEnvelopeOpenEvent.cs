using UnityEngine;
using System.Collections;

public class CheckInFlowEnvelopeOpenEvent : MonoBehaviour {
  public event System.Action OnEnvelopeOpen;
  public void RaiseOpenAnimationEvent() {
    if (OnEnvelopeOpen != null) {
      OnEnvelopeOpen();
    }
  }

  public event System.Action OnEnvelopeStartExit;
  public void RaiseStartExitAnimationEvent() {
    if (OnEnvelopeStartExit != null) {
      OnEnvelopeStartExit();
    }
  }
}
