using UnityEngine;
using System.Collections;
using Anki.UI;

public class MainStoryboard_Design : Anki.UI.Storyboard {

  protected override void LoadStoryboard()
  {
    // Define Storyboard Node Keys
    string sbNode1Key = "1";
    string sbNode2Key = "2";
    string sbNode3Key = "3";

    // Create Storyboard Nodes
    // Node 1
    StoryboardNode sbNode1 = new StoryboardNode(sbNode1Key, "Root_Design");
    sbNode1.NavigationBarBackButtonHidden = true;
    sbNode1.AddSegueDescription(new StoryboardSegueDescription("Push", sbNode2Key, NavigationSegueDirection.Forward, SegueAnimationTransition.AnimationType.Slide));

    // Node 2
    StoryboardNode sbNode2 = new StoryboardNode(sbNode2Key, "View 4 - PushStoryboard_Design");
    sbNode2.AddSegueDescription(new StoryboardSegueDescription("Push_A", sbNode3Key, NavigationSegueDirection.Forward, SegueAnimationTransition.AnimationType.Slide));
    // Example: Show how to use multiple Unwind Segues and set one as the preferred or default
    sbNode2.AddSegueDescription(new StoryboardSegueDescription("POP_A", sbNode1Key, NavigationSegueDirection.Reverse, SegueAnimationTransition.AnimationType.Slide));
    sbNode2.AddSegueDescription(new StoryboardSegueDescription("POP_B", sbNode1Key, NavigationSegueDirection.Reverse, SegueAnimationTransition.AnimationType.Slide));
    sbNode2.UnwindSegueIdentifier = "POP_A";
  
    // Node 3
    StoryboardNode sbNode3 = new StoryboardNode(sbNode3Key, "Sub-StoryboardNavCntr 1_Design");
    sbNode3.AddSegueDescription(new StoryboardSegueDescription("ExitSegue_Pop", sbNode2Key, NavigationSegueDirection.Reverse, SegueAnimationTransition.AnimationType.Slide));
    // This is presenting another Storyboard & Navigation Controller. Need to hide Navigation Bar
    sbNode3.NavigationBarHidden = true;

    // Store Nodes in Storyboard
    SetInitialNode(sbNode1);

    AddStoryboardNode(sbNode2);
    AddStoryboardNode(sbNode3);
  }

}
