using UnityEngine;
using System.Collections;
using Anki.UI;

public class SubStoryboard2_Design : Anki.UI.Storyboard {


  protected override void LoadStoryboard()
  {
    // Define Storyboard Node Keys
    string sbNode1Key = "1";
    string sbNode2Key = "2";
    string sbNode3Key = "3";
    string sbNode4Key = "4";
    
   
    // Create Storyboard Nodes
    StoryboardNode sbNode1 = new StoryboardNode(sbNode1Key, "Sub-Root_Design");
    sbNode1.AddSegueDescription(new StoryboardSegueDescription("Push", sbNode2Key, NavigationSegueDirection.Forward, SegueAnimationTransition.AnimationType.Slide));
    sbNode1.AddSegueDescription(new StoryboardExitSegueDescription("Pop", "Exit_Pop"));
    
    StoryboardNode sbNode2 = new StoryboardNode(sbNode2Key, "View 1_Design");
    sbNode2.AddSegueDescription(new StoryboardSegueDescription("Push_A", sbNode3Key, NavigationSegueDirection.Forward, SegueAnimationTransition.AnimationType.Slide));
    sbNode2.AddSegueDescription(new StoryboardSegueDescription("Push_B", sbNode4Key, NavigationSegueDirection.Forward, SegueAnimationTransition.AnimationType.Slide));
    sbNode2.AddSegueDescription(new StoryboardSegueDescription("POP_A", sbNode1Key, NavigationSegueDirection.Reverse, SegueAnimationTransition.AnimationType.Slide));
    sbNode2.AddSegueDescription(new StoryboardSegueDescription("POP_B", sbNode1Key, NavigationSegueDirection.Reverse, SegueAnimationTransition.AnimationType.Slide));
    sbNode2.UnwindSegueIdentifier = "POP_A";
    
    StoryboardNode sbNode3 = new StoryboardNode(sbNode3Key, "View 2_Design");
    sbNode3.AddSegueDescription(new StoryboardSegueDescription("Pop", sbNode2Key, NavigationSegueDirection.Reverse, SegueAnimationTransition.AnimationType.Slide));
    
    StoryboardNode sbNode4 = new StoryboardNode(sbNode4Key, "View 3_Design");
    sbNode4.AddSegueDescription(new StoryboardSegueDescription("Pop", sbNode2Key, NavigationSegueDirection.Reverse, SegueAnimationTransition.AnimationType.Slide));
    
    // Store Nodes in Storyboard
    SetInitialNode(sbNode1);
    
    AddStoryboardNode(sbNode2);
    AddStoryboardNode(sbNode3);
    AddStoryboardNode(sbNode4);
  }

}
