RenderSDK is a C++/Direct3D 12 project I am working in my spare time.

The project implements the following functionality:

- GPU programmable occlussion culling for rendering geometry
- Tiled light culling
- Tiled deferred shading
- Deferred texturing based on Direct3D 12 bindless resources
- Spot light lighting
- Exponential shadow maps

<b>Building the project</b>  
The project is setup for Visual Studio 2017, Debug, x64 mode.  
You might also need to update Target Platform Version (Configuration Properties/General) for the projects in the solution based on your current Windows SDK. Version 10.0.16299.0 is used by default.
