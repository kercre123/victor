# Path planning

* Loads obstacles from the [map component](map.md)
* Automatic replanning around new obstacles
* A* Lattice planner, runs in thread
* Simpler planners can run in special cases

Path planning refers to the problem of navigating the robot from point A to B without collisions and, for
Victor, in a way that fits with his character.

The following things happen, roughly in order, and skipping a bunch of details:

1. A [behavior](behaviors.md) (or behavior helper) executes a [DriveToPoseAction](actions.md) (or an action
   that contains one). This action has a goal pose (or set of goal poses)
   
2. The robot selects an appropriate planner based on the robot pose and goal. For long distances, this is
   always the `LatticePlanner`, but for short distances or specific use cases (like aligning to dock with a
   cube) another simpler planner may be selected.

3. The `LatticePlanner` lives in the `Cozmo` namespace and incorporates all of the obstacles from the
   [nav map](map.md). The lattice planner owns a "worker" thread where the planning will be done
   
4. Lattice planner hands off to `xythetaPlanner` in it's thread, which is part of coretech and doesn't know
   anything about, e.g. the `Robot` class. Planning takes some time to run, and runs in it's own thread

5. During each tick of engine, the planning status is checked. When the plan is complete, it is processed into
   a path the robot can follow
   
6. The `SpeedChooser` uses robot parameters + randomness to pick the speed that the robot should follow the path

7. The action, together with the `DrivingAnimationHandler` runs the animations which get layered onto the
   driving (e.g. sounds, eyes, head motion)
   
8. The path following happens in the robot process.

9. While following the path, safety checks are made to determine if _replanning_ is needed. If there is a new
   obstacle along the path, the robot will replan a new one. Ideally this will happen in the background and be
   non-noticeable to the user, but if the robot gets too close to the obstacle, it'll have to stop and wait
   for the new plan to be computed

## A* on a lattice

Some docs on planning on a lattice can be found in the [ROS SBPL docs](http://wiki.ros.org/sbpl). A brief
overview is here.

Instead of planning on a grid, we plan with _motion primitives_. These are sections of arcs and straight lines
that fit within the constraints of what the robot can execute. The space is discretized into 16 (or some other
number) of angles, and set up so that each primitive starts and ends _exactly_ on a grid cell center aligned
with one of the discrete angles.

In this way, once a plan is complete, it is ready to be executed as-is without any smoothing or trajectory
optimization on top.

By varying relative cost of different actions, you can produce qualitatively different paths -- for example
preferring backing up vs. turning around, and smooth arc turns vs. sharp "in place" turns.

Tools associated with the creation and visualization of the motion primitives can be found in the
[planning](../../coretech/planning) directory (look in `/tools` and `/matlab`).

For historical reasons (Cozmo having limited number of obstacles), the planner internally uses a "list of
polygons" as it's obstacle, rather than more traditional cost maps. To convert through the Nav Map, a convex
hull algorithm is run on (pieces of) the obstacles from the quad tree.

## Soft obstacles

Most obstacles (all, at the moment) are "soft" obstacles, which means that the planner _is allowed_ to collide
with them, it's just heavily penalized. This prevents the robot from "Getting stuck", e.g. if it is already in
collision with an object, or if it has no way to move without a collision.
