RenderSDK is a C++/Direct3D 12 project I am working in my spare time.

<b>Supported functionality</b>
- GPU programmable occlussion culling for rendering geometry
- Tiled light culling
- Tiled deferred shading
- Deferred texturing based on bindless resources
- Spot light lighting
- Exponential shadow maps
- CPU/GPU profiler based on timestamp queries

<b>Camera navigation</b>
- Keys W, S, A, D, E, Q to move the camera
- Keys UP, DOWN, LEFT, RIGHT to rotate the camera

<b>Building and runnig the project</b>  
The project is setup for Visual Studio 2017, Debug, x64 mode.
You might also need to update Target Platform Version (Configuration Properties/General) for the projects in the solution based on your current Windows SDK.
Version <b>10.0.16299.0</b> is used by default.
Select project <b>DynamicGI</b> as a start-up on. 

![Alt text](/Samples/DynamicGI/Screenshots/Sponza.jpg?raw=true)
