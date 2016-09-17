RenderSDK is a C++/Direct3D 12 project I am working in my spare time.
I started it around August last year when Direct12 API became publicly available with release of Windows 10.
My objective with the project was to gain practical experience with Direct3D 12 API.
I wanted to have a set of ready and easy to use utilities to implement and experiment with graphics effects, taking benefit of the new API features.

In the current development state, it provides the following packages:

1.D3DWrapper

Thin Direct3D 12 wrappers around the native API.

2.Math

Essential mathematical things to deal with
- vectors
- matrices
- quaternions
- transformations
- spherical harmonics
- intersection tests, etc.

3.RenderPasses

In particular, it is responsible for setting up pipeline state object associated with the render pass and recording the draw/compute commands.

4.Samples

HelloRectangle. Demonstrates core functionality of Direct3D 12 API in action, such as using
- index and vertex buffers
- command queue and command lists
- descriptor table and descriptor heap
- uploading data to the GPU and handling resource transitions.

SceneVoxelization.
My initial intention was to play around with scene voxelization taking advantage of hardware support for conservative rasterization and rasterizer ordered views to generate voxel grid data. The sample pretty quickly evolved into,  
