using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Anki
{
  namespace UI
  {

    public struct StoryboardNode
    {
      public string Identifier;  // Must be unique
      public string ViewControllerPrefabName;
      // View Navigation Controller Properties
      public bool NavigationBackgroundViewHidden;
      public bool NavigationForegroundViewHidden;
      public bool NavigationBarHidden;
      public bool NavigationBarBackButtonHidden;
      public string UnwindSegueIdentifier;
      // Segues
      private Dictionary<string, StoryboardSegueDescription> _Segues;
      
      public StoryboardNode(string identifier, string viewControllerPrefabName)
      {
        this.Identifier = identifier;
        this.ViewControllerPrefabName = viewControllerPrefabName;
        NavigationBackgroundViewHidden = false;
        NavigationForegroundViewHidden = false;
        NavigationBarHidden = false;
        NavigationBarBackButtonHidden = false;
        UnwindSegueIdentifier = null;
        this._Segues = new Dictionary<string, StoryboardSegueDescription>();
      }
      
      // Segue 
      public void AddSegueDescription(StoryboardSegueDescription segue)
      {
        _Segues.Add(segue.Identifier, segue);
      }
      
      public StoryboardSegueDescription SegueWithIdentifier(string identifier)
      {
        StoryboardSegueDescription aSegue = null;
        _Segues.TryGetValue(identifier, out aSegue);
        return aSegue;
      }
      
      public StoryboardSegueDescription DefaultUnwindSegue()
      {
        StoryboardSegueDescription segueDescription = null;
        // First check if Unwind Segue Identifier exist
        if (UnwindSegueIdentifier != null) {
          _Segues.TryGetValue(UnwindSegueIdentifier, out segueDescription);
        }
        else if (segueDescription == null) {
          // Search for unwind segue
          List<string> segueKeys = new List<string>(_Segues.Keys);
          for (int idx = 0; idx < segueKeys.Count; ++idx) {
            // Check if it's an Unwind Segue
            StoryboardSegueDescription aSegue = _Segues[segueKeys[idx]];
            if ( aSegue.Direction == NavigationSegueDirection.Reverse || aSegue.Direction == NavigationSegueDirection.Exit ) {
              // Check if the unwind segue has already been set or is set to an exit segue and this segue is a Reverse
              // Prioritise Reverse segues if none exist use a Exit segue if only one exist
              if ( segueDescription == null ||
                  (aSegue.Direction == NavigationSegueDirection.Reverse &&
               segueDescription.Direction == NavigationSegueDirection.Exit) ) {
                // First Unwind segue
                segueDescription = aSegue;
              } else {
                // More then one Unwind segue, therefore no default unwind segue
                return null;
              }
            }
          }
        }
        
        return segueDescription;
      }

    }
  }
}
