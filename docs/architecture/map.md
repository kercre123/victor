# Map component

* Holds the robots navigation map
* Tracks memory of things seen and places been
* BlockWorld is separate
* Nav Map is 2D (planar), Block/Face/Pet World is 3D
* Underlying QuadTree

## Nav Map aka. Memory Map

The [map component](/engine/navMap/mapComponent.cpp) owns the "nav(igation) map", which holds information about the "memory" of the things the robot has seen: including cliffs, prox obstacles (from the distance sensor) and ground that the robot has successfully traversed in the past.

The map consists of regions of `MemoryMapData`. The data has a content type, `EContentType` defined in [memoryMapTypes.h](/engine/navMap/memoryMap/memoryMapTypes.h). This includes, for example, `Unknown`, `ObstacleCharger`, `Cliff`. Additional data can also be stored along with the content type (e.g. a cube ID). Data can be added to the memory map using functions like `AddQuad()`, `AddLine()`, etc. The data in the map can also be modified with the `TransformContent()` functions.

The map can be queried in multiple ways. Geometric queries can be done such as `HasCollisionRayWithTypes()` which checks if a ray collided with certain types. "Borders" between types can also be computed with the various border functions (e.g. the border between `Unknown` and `ClearOfCliff` might be the frontier you want to explore). There are also queries like `FindContentIf()` or `HasContentType()` that search the entire map, regardless of position.

The resolution of the memory map can be queried `INavMap::GetContentPrecisionMM()`, and as of this writing is 1cm.

## Storage

As an implementation detail, the memory map is stored as a QuadTree. These details are entirely internal to the map and shouldn't affect the usage, but do provide efficient storage for large maps, including automatically splitting and merging cells in the QuadTree as data gets added or updated. This is easily visible in the Webots viz.

There is a limit to the amount of memory we want to use to store this data, so if the quad tree becomes "too large" (defined in [quadTree.cpp](/engine/navMap/quadTree/quadTree.cpp)) it will be truncated. I.e. if you drove forever in one direction, you'd start to lose memory of the things far away from the robot as it drove.

## Navigation

Some things in the nav map are obstacles, and will be added as obstacles for the [path planner](planner.md).
