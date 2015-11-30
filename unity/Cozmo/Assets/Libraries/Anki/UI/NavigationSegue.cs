using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Anki
{
  namespace UI
  {

    public enum NavigationSegueDirection {
      Forward,
      Reverse,
      Exit
    };


    public class NavigationSegue
    {
      public delegate void SegueTransitionDelegate(Anki.UI.NavigationController navigationController);

      public Anki.UI.ViewController SourceViewController { get; private set; }

      public Anki.UI.ViewController DestinationViewController { get; private set; }

      public string Identifier { get; private set; }

      public NavigationSegueDirection Direction { get; private set; }

      public Anki.UI.NavigationController.OperationType OperationType { get; private set; }

      public Anki.UI.SegueAnimationTransition AnimationTransition;

      public SegueTransitionDelegate TransitionDelegate;

      public NavigationSegue(Anki.UI.ViewController sourceViewController,
                             Anki.UI.ViewController DestinationViewController,
                             string identifier,
                             NavigationSegueDirection direction,
                             Anki.UI.NavigationController.OperationType operationType = Anki.UI.NavigationController.OperationType.Segue)
      {
        this.SourceViewController = sourceViewController;
        this.DestinationViewController = DestinationViewController;
        this.Identifier = identifier;
        this.Direction = direction;
        this.OperationType = operationType;
      }

      public void ApplyAnimationTransition(Anki.UI.SegueAnimationTransition transition)
      {
        if (transition != null) {
        transition.SourceViewController = this.SourceViewController;
        transition.DestinationViewController = this.DestinationViewController;
        transition.Direction = this.Direction;
        this.AnimationTransition = transition;
        }
        else {
          DAS.Error("NavigationSegue", "ApplyAnimationTransition - Transition is NULL");
        }
      }


      public override string ToString()
      {
        return string.Format("[ NavigationSegue: SourceViewController={0}, DestinationViewController={1}, Identifier={2}, OperationType={3} ]", SourceViewController, DestinationViewController, Identifier, OperationType.ToString());
      }
    } // Class StoryboardSegue


  }  // Namespace UI
} // Namespace Anki
