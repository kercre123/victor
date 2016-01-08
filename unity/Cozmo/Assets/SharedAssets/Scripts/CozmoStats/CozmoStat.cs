using UnityEngine;
using System.Collections;

public enum CozmoStat {

  Time, // The length of an interaction with Cozmo
  Bonding, // Interacting with the primary owner
  Social, // Interacting with a variety of people
  Excitement, // Surprises, shocks, high acceleration movement, discovering edges, games that come down to the wire, near wins/loses.  Events that correlate with high anticipation.
  Novelty, // Interactions that are fresh, have not been repeated in a long time.
  Competitiveness, // Playing games against other players
  Cooperation, // Working together with someone towards a shared goal
  Exploration, // Building a large map of Cozmo's surroundings.  Encountering and labeling environmental objects (obstacles, cliffs, undrivable zones)
  SelfDirection, // Being allowed to do his own thing.  Allowing Cozmo to set the level of interaction.
  COUNT
}
