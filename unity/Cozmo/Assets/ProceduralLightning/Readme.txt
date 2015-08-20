Procedural Lightning for Unity 3D
© 2015 Digital Ruby, LLC – Created by Jeff Johnson – jjxtra@gmail.com
http://www.digitalruby.com

Version 1.3

Changelog:
1.0    -  Initial release
1.0.1  -  Added web player link
1.0.2  -  Fixed a display glitch with multiple bolts and very short duration
1.1    -  Added an option to render lightning with a mesh that cuts down on the number of draw calls and CPU usage significantly
1.2    -  Bug fixes
1.3    -  Added a bunch of prefabs for shape lightning, chain / path lightning and more with demo scenes
    -  Improved lightning bolt batching. Use CreateLightningBolts (plural) on LightningBoltScript to batch all bolts into one draw call
    -  Scene view shows gizmos for building lightning paths for better visualization - see DemoScenePath
    -  Disabled legacy line renderer. It is still there, but #if'd out. I would suggest avoiding using this as I will probably delete
      it in a future version. The CPU usage and draw calls are terrible for this mode.

Procedural Lightning for Unity contains code to create 3D procedural lightning for use in your Unity game or app.

Assets provided:
- Demo Scenes:
  - Night time (DemoScene)
  - Day time (DemoScene2)
  - Script Configuration Scene (DemoSceneConfigureScript)
  - Prefab Configuration Scene (DemoSceneConfigurePrefab)
  - Path demo (DemoScenePath)
  - Shape lightning (sphere, etc.) (DemoSceneShape)
  - Lightning strike demo (DemoSceneStrike)
  - Jitter / Lightning Field demo (DemoSceneLightningField)
- Prefab
  - LightningBoltPathPrafab.prefab is a component to create lightning that follows a path
  - LightningBoltPrefab.prefab is a component to create lightning that goes from a start point to an end point
  - LightningBoltShapeConePrefab.prefab is an example of creating lightning in the shape of a cone
  - LightningBoltShapeSpherePrefab.prefab is an example of creating lightning in the shape of a sphere
  - ThunderAndLightningPrefab.prefab is a drop in component for random lightning or your own custom lightning. If you don't want the random lightning, simply disable the ThunderAndLightning script or delete it.
  - Dark cloud particle system
  - High level thunder and lightning script (ThunderAndLightningScript.cs)
  - Low level lightning bolt script (LightningBoltScript.cs)
  - Lightning glow (defaults to a blue tint, but this can be changed in the material using the inspector, or by changing the _TintColor variable in a script).
    - Texture (LightningGlowTexture.png)
    - Material (LightningGlowMaterial.mat)
    - Shader (LightningGlowShader.shader)
  - Lightning bolt
    - Texture (LightningBoltTexture.png)
    - Material (LightningBoltMaterial.mat)
    - Shader (LightningBoltShader.shader)
  - Lightning origin (for when it comes from a cloud)
    - Texture (Default particle)
    - Material (LightningOriginMaterial.mat)
  - 17 thunder sounds (thunder*.mp3)

- Configuration scene overview (DemoSceneConfigure)
This scene allows tweaking of many of the parameters for your lightning. Press SPACE BAR to create a lightning bolt, and drag the circle and anchor to change where your bolt goes.

The goal here is to get a style of bolt that will work for your game or app. Once you have a style you like, you can click "copy" in the bottom right. This will put a C# script on your clipboard which you can paste into your app. It will also put the current "seed" for the lightning into the text box in the bottom right which means the structure of your bolt will be the same each time it is emitted. This is great for cut-scenes or other scenarios where you have an exact style bolt you want. Simply click "clear" to get a random seed again.

The possible parameters are:
- Generations: The number of splits, or details in the lightning. Higher numbers yield higher quality looking lightning, at the cost of additional CPU time to create the lightning bolt. Usually 5-6 generations is good enough, unless you want really nice looking bolts, in which case 8 works great - but remember to only do this on capable hardware if you are generating lots of bolts. This value can be 0 for point light only lightning, otherwise it's clamped between 4-8.
- Bolt count: The number of lightning bolts to shoot out at once. There is a slight delay before successive bolts are sent out. This delay can be changed and will be a variable when you copy the script for your lightning bolt.
- Duration: The total time in seconds that the bolts will take to emite and dissipate. When sending out multiple bolts, each bolt will appear for a percentage of this time (about duration divided by count seconds).
- Start Distance: This moves the source of the bolt closer or further away from the camera. For best results, keep this close to the end distance.
- End Distance: This moves the end of the bolt closer or further away from the camera. For best results, keep this close to the start distance.
- Chaos Factor: As this value is increased, the lightning bolts spread out more and become more chaotic and cover more distance. In my testing, 0.1 to 0.25 are good ranges.
- Trunk Width: How wide the main trunk of the lightning bolt will be int Unity units.
- Forkedness: How many forks or splits will your lightning bolt have? If this value is 0, none. If it is 1, lots!
- Glow Intensity: How bright the glow of the lightning will be. Set to 0 to remove the glow.
- Glow Width Multiplier: Spreads the glow out more. Set to 0 to remove the glow.
- Fade Percent: 0.0 to 0.5, how long the lightning takes to fade in and out during it's lifetime (percent).
- Growth multiplier: How slowly the lightning should grow, 0 for instant, 1 for slow (0 - 1).

*Scene view*
Here is how to rapidly prototype and design lightning in your scene:

- Drag one of the prefabs into your scene
- Press play
- Tweak the values to your liking
- Copy the root game object of the prefab
- Stop play
- Paste in the new object
- *Important* The parent object and object with the lightning script attached should have position and rotation values of 0, and scale of 1. See the DemoScenePath for an example.

*Rendering*
The lightning can be rendererd via two modes, configurable via a render mode property on the Lightning object in the prefab or on the LightningScript.cs code.
- Mesh renderer with line glow. The glow will follow the shape of the line. This gives a consistant glow size no matter the generation setting and follows the shape of each line segment.
- Mesh renderer with billboard glow. The glow will be a square with size of the length of the line. The glow will be much larger on smaller generations, and smaller for higher generations.

A few special cases are worth pointing out:
- LightningBoltParameters contains a Camera property. This property ensures that the glow looks correct. If you are rendering the lightning in a camera that is not the main camera (Camera.main), you will need to assign this if your lightning has glow.
- LightningBoltParameters also has a Random property. You can use this if you want to create a bolt that will look the same every time. Use the System.Random constructor that takes a seed property. The default is a new System.Random.
- LightningBoltParameters has an EndWidth property. This will set the ends of your lightning bolt (and branches) to a percent of the start width. This can be useful if you want pointier lightning, in which case lower the value towards 0. The default is 0.5f.
- LightningBoltParameters has a PointLightIntensity property. Set this to non-zero to create a point light at the source of the lightning. This point light will fade in and out with the lightning bolt. For point light only lightning (i.e. cloud lightning) set the generations property to 0.
- Lightning glow can be expensive, so you may want to disable this on lower end mobile devices by setting the glow intensity or the glow width to 0.
- When parenting under a prefab object, please ensure that both the parent prefab and the object that the lightning script is on have a position and rotation of 0 and scale of 1.

*Orthographic Cameras*
When generating lightning for an orthographic camera, the algorithm changes slightly and the lightning will look a little more spread out and varied because there will be no z direction.

This asset contains a lot of options and code, so if you are confused or need guidance, please contact me. I'm always happy to answer questions. Please email me at jjxtra@gmail.com and I'll be happy to assist you.

Thanks for purchasing Procedural Lightning for Unity.

- Jeff Johnson, CEO Digital Ruby, LLC
jjxtra@gmail.com
http://www.digitalruby.com