## FaceWorld / PetWorld

### Quick Summary

* Components of the Robot like [BlockWorld](blockWorld.md), but for storing human (and pet) faces
* Human faces' poses are also tracked, while pets' are not

---

### Details

`FaceWorld` is a component of the Robot for storing faces and their last known location. It is effectively a simpler version of [BlockWorld](blockWorld.md) for storing faces and their 3D poses (and "rejiggering" their poses like BlockWorld does for objects).

It is also is the mechanism for interacting with the face recognizier in the [Vision System](visionSystem.md), e.g. for Face Enrollment (aka Meet Cozmo).

`PetWorld` is the analogous container for pet faces. A key difference for pet faces is that we do not know their 3D pose, only their heading. This is because we do not have any known-size landmark we can identify, unlike human faces where we can use the distance between eyes, which is a relatively consistent dimension, to estimate distance to the face. Since no poses are stored, no rejiggering is required.

