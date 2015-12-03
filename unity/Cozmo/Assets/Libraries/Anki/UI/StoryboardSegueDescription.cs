using UnityEngine;
using System.Collections;

namespace Anki
{
  namespace UI
  {
    // Describe a StoryboardSegue with metadata
    // This allows us to lazy load a StoryboardSegue and destination ViewControllers
    
    public class StoryboardSegueDescription
    {
      public string Identifier;
      public string DestinationStoryboardNodeIdentifier;
      public SegueAnimationTransition.AnimationType AnimationType;
      // Set this to change an AnimationTransition's default duration
      public float? AnimationDuration;
      public NavigationSegueDirection Direction;
      
      public StoryboardSegueDescription()
      {
        AnimationType = SegueAnimationTransition.AnimationType.Slide;
        Direction = NavigationSegueDirection.Forward;
      }
     
      public StoryboardSegueDescription(string identifier,
                                        string destinationStoryboardNodeIdentifier,
                                        NavigationSegueDirection direction,
                                        SegueAnimationTransition.AnimationType animationType,
                                        float? animationDuration = null)
      {
        this.Identifier = identifier;
        this.DestinationStoryboardNodeIdentifier = destinationStoryboardNodeIdentifier;
        this.AnimationType = animationType;
        this.AnimationDuration = animationDuration;
        this.Direction = direction;
      }
      
      
      public override string ToString()
      {
        return string.Format("[ StoryboardSegueDescription Identifier = \"{0}\" " +
                             "DestinationStoryboardNodeIdentifier = \"{2}\" AnimationType = \"{3}\" ]", 
                             Identifier, DestinationStoryboardNodeIdentifier, AnimationType.ToString());
      }
      
    } // Class StoryboardSegueDescription
    
    
    // 
    // 
    public class StoryboardExitSegueDescription : StoryboardSegueDescription
    {
      // This identifies the parent storyboard node
      public string ExitSegueIdentifier;


      public StoryboardExitSegueDescription(string identifier,
                                            string exitSegueIdentifier) 
        : base(identifier, null, NavigationSegueDirection.Exit, SegueAnimationTransition.AnimationType.Slide)
      {
        this.ExitSegueIdentifier = exitSegueIdentifier;
      }
      
      public override string ToString()
      {
        return string.Format("[ StoryboardExitSegueDescription Identifier = \"{0}\" " +
                             "ExitSegueIdentifier = \"{2}\" AnimationType = \"{3}\" ]", 
                             Identifier, DestinationStoryboardNodeIdentifier, AnimationType.ToString());
      }
    }
    
    
    // Use this when performing a segue to a node who's view controller is an other storyboard and you want to
    // specify it's inital node.  i.e. jump to the middle of a storyboard flow.
    // FIXME For the love of god change this class name!!!
    public class StoryboardToStoryboardSegueDescription : StoryboardSegueDescription
    {
      // This identifies the sub-storyboards segue to load when loading  
      public string SubStoryboardInitialNodeIdentifier;


      public StoryboardToStoryboardSegueDescription(string identifier,
                                                    string destinationStoryboardNodeIdentifier,
                                                    string subStoryboardInitialNodeIdentifier,
                                                    NavigationSegueDirection direction,
                                                    SegueAnimationTransition.AnimationType animationType,
                                                    float? animationDuration = null)
        : base(identifier, destinationStoryboardNodeIdentifier, direction, animationType, animationDuration)
      {
        this.SubStoryboardInitialNodeIdentifier = subStoryboardInitialNodeIdentifier;
      }
      
      public override string ToString()
      {
        return string.Format("[ StoryboardToStoryboardSegueDescription Identifier = \"{0}\" " +
                             "DestinationStoryboardNodeIdentifier = \"{2}\" " +
                             "StoryboardEntrySegueIdentifier = \"{3}\" " +
                             "AnimationType = \"{4}\" ]", 
                             Identifier, DestinationStoryboardNodeIdentifier,
                             SubStoryboardInitialNodeIdentifier, AnimationType.ToString());
      }
    }

  }
}
