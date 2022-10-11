# cpp-software-rasterizer
This is a scanline 3D rasterizer run on the CPU. It goes through each pixel and calculates whether or not it's taken up by a triangle of the mesh. The shadow mapping is done by rendering a depth map from a different perspective and comparing the current depth of the pixel. I first wrote this in [JavaScript](https://github.com/JentGent/software-rasterizer), then I ported it to C++.
## Demo
![Gameplay](https://i.imgur.com/6WJvcE3.gif)
