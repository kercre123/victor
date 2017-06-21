using UnityEngine;
using System.Collections;

public abstract class HubWorldBase : MonoBehaviour {

  protected static HubWorldBase _Instance = null;

  public static HubWorldBase Instance { get { return _Instance; } }

  public abstract void LoadHubWorld();
  public abstract void DestroyHubWorld();

  public abstract GameBase GetChallengeInstance();

  public abstract bool CloseChallengeImmediately();

  public abstract void StartFreeplay();
  public abstract void StopFreeplay();
  public abstract void StartLoadNeedsHubView();
}
