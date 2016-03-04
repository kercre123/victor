using UnityEngine;
using System;
using System.Collections;

namespace Anki
{
  namespace UI
  {
    public class SegueAnimationTransition : MonoBehaviour
    {
      // Define Animation Types
      public enum AnimationType
      {
        None,
        PushPop,
        Fade,
        Slide
      }

      public delegate void Callback();

      // Properties
      public Anki.UI.ViewController SourceViewController;  
      public Anki.UI.ViewController DestinationViewController;
      public NavigationSegueDirection Direction;
      public float AnimationDuration = 0.0f;

      public Callback AnimationCompletedCallback;


      // Factroy method to add AnimationTransition component to gameObject and apply to segue.
      public static SegueAnimationTransition ApplyAnimationTransitionToSegue(GameObject gameObject,
                                                                             AnimationType animationType,
                                                                             float? animationDuration,
                                                                             NavigationSegue segue)
      {
        SegueAnimationTransition transition = CreateAnimationTransitionComponent(gameObject, animationType);
        if (animationDuration.HasValue) {
          transition.AnimationDuration = animationDuration.Value;
        }

        segue.ApplyAnimationTransition(transition);
        
        return transition;
      }


      // Factory method to add AnimationTransition component to gameObject
      private static SegueAnimationTransition CreateAnimationTransitionComponent(GameObject gameObject, AnimationType animationType)
      {
        SegueAnimationTransition transition = null;
        switch(animationType) {

          case AnimationType.None:
            transition = gameObject.AddComponent<SegueAnimationTransition_None>();
            break;

          case AnimationType.PushPop:
            transition = gameObject.AddComponent<SegueAnimationTransition_Push_Pop>();
            break;
            
          case AnimationType.Fade:
            transition = gameObject.AddComponent<SegueAnimationTransition_Fade>();
            break;
            
          case AnimationType.Slide:
            transition = gameObject.AddComponent<SegueAnimationTransition_Slide>();
            break;
            
        }
        
        return transition;
      }


      // Start Animation
      public void PerformAnimation()
      {
        PrepareForAnimation();
        AnimateTransition();
      }


      // ------------------------------------------------------------------------------------------
      // Animation sub-class hooks to customize animation transition
      // ------------------------------------------------------------------------------------------

      // Animation Engine must call this methods when it's completed 
      protected void AnimationCompleted()
      {
        if (AnimationCompletedCallback != null) {
          AnimationCompletedCallback();
        }
      }

      // Animation Hooks
      protected virtual void PrepareForAnimation()
      {
      }

      protected virtual void AnimateTransition()
      {
      }


    }
  }
}
