using UnityEngine;
using System;
using Anki.UI;

// Static class that manages all game events, listen to these events in order to find
// exact hooks for certain game and UI moments.
// TODO: Fill out all requested events both here and in the Onboarding Director
public class EventManager : MonoBehaviour {
  
  private static EventManager _Instance;

  public static EventManager Instance {
    get {
      Initialize();
      return _Instance;
    }
  }

  public static void Initialize() {
    if (_Instance != null) {
      return;
    }
    DAS.Info("EventManager.Initialize()", "Event Manager Initialization");
    _Instance = new GameObject("EventManager").AddComponent<EventManager>();
  }

  // Global Events

  // ViewController Events
  public event Action<ViewController> ViewController_WillTransitionIn;
  public void VC_WillTransitionIn(ViewController vc) {
    if (ViewController_WillTransitionIn != null) {
      //DAS.Info("EventManager", "WillTransitionIn");
      ViewController_WillTransitionIn(vc);
    }
  }
  public event Action<ViewController> ViewController_DidTransitionIn;
  public void VC_DidTransitionIn(ViewController vc) {
    if (ViewController_DidTransitionIn != null) {
      //DAS.Info("EventManager", "DidTransitionIn");
      ViewController_DidTransitionIn(vc);
    }
  }
  public event Action<ViewController> ViewController_WillTransitionOut;
  public void VC_WillTransitionOut(ViewController vc) {
    if (ViewController_WillTransitionOut != null) {
      //DAS.Info("EventManager", "WillTransitionOut");
      ViewController_WillTransitionOut(vc);
    }
  }
  public event Action<ViewController> ViewController_DidTransitionOut;
  public void VC_DidTransitionOut(ViewController vc) {
    if (ViewController_DidTransitionOut != null) {
      //DAS.Info("EventManager", "DidTransitionOut");
      ViewController_DidTransitionOut(vc);
    }
  }
  public event Action<ViewController> ViewController_Dismissed;
  public void VC_Dismissed(ViewController vc) {
    if (ViewController_Dismissed != null) {
      //DAS.Info("EventManager", "VC Dismissed");
      ViewController_Dismissed(vc);
    }
  }

  // ListViewCell Events
  public event Action<ListViewCell> ListViewCell_IsSelected;
  public void LVC_IsSelected(ListViewCell lvc) {
    if (ListViewCell_IsSelected != null) {
      DAS.Info("EventManager", "ListViewCell Selected" + lvc.name);
      ListViewCell_IsSelected(lvc);
    }
  }

  // Audio Events
  public event Action<string> Overlay_VO_Ended;
  public void Overlay_VO_Event_End(string source) {
    if (Overlay_VO_Ended != null) {
      DAS.Info("EventManager","VO_FINISH");
      Overlay_VO_Ended(source);
    }
  }

}
