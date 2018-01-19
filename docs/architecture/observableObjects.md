## Observable Objects

### Quick Summary

* Objects are things the robot can observe and interact with, stored in [BlockWorld](blockWorld.md)
* Their identity and pose can be estimated using VisionMarkers of known size on them
* Active objects have lights and accelerometers and a radio connection to the Robot
* Passive objects just have markers
* Custom Objects are definable at runtime for use by the SDK

---

### Base Observable Objects

`ObservableObject` is the base class from Coretech-Vision. It referes to objects possessing some number of `VisionMarkers` on them at known locations. Thus, by seeing one or more VisionMarkers (and knowing their physical size), the vision system can instantiate an object and know where it is in 3D space, relative to the camera. The Cozmo project actually utilizes an extended version of the `Vision::ObservableObject` class found in `Cozmo::ObservableObject` which adds support for Cozmo-specific attributes like object families and types.

### Actionable Objects
`ActionableObject` derives from `ObservableObject` and is Cozmo/Victor-specific (i.e., not in CoreTech). It adds the notion of being able to interact with (not just observe) an object. These objects have `PreActionPoses`, which are pre-defined relative positions for "docking", "rolling", or "entry", for example.

### Active Objects
`ActiveObject` derives from `ObservableObject` and is Cozmo/Victor-specific. It adds methods for controlling the LEDs on Light Cubes and querying or maintaining their state of motion. (LightCubes are both ActionableObjects and ActiveObjects.) Importantly, because ActiveObjects can detect if they've been moved, they can potentially be used for localization by the robot.

### Custom Objects
**CustomObject** is generally only used by the SDK and can be defined at runtime via messages. There are different types:

* **Walls** with a height and width and the same marker on front and back
* **Cubes** with a single dimension in all direction and the same marker on all sides
* **Boxes** with with specified size in x,y,z and individual markers on each side (generalizations of the above)
* **FixedObstacles** which have no markers but are simply static obstacles to be avoided

Additionally, objects can be defined to be "unique", meaning the user is assuring the system that only one object of this type exists and thus can be immmediatley updated based on its type (vs. reasoning about its pose to decide if it is a new or existing instance).

### Markerless Objects [Deprecated]
`MarkerlessObject` also derives from ObservableObject, but (as the name suggests) does not have any Markers. They inherit from ObservableObject just so they work within BlockWorld in the same way, but they must be "observed" via some other means than VisionMarkers, such as a proximity sensor. 

NOTE: MarkerlessObjects are somewhat deprecated and we are moving towards maintaining these "objects" directly in the [Nav Map](navMap.md) instead.

