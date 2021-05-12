# tantien

Game engine.

## Features

### Visual programming
Nodes for everyone: shader, render pipeline, material, vfx, gui ...

### Procedural generation
Procedural content generation

### Data driven
Use editors to generate each component

### Fast iteration
* Light c&cpp parts, more heavy on scripts
* Edit assets at runtime directly

## Editors

### blueprint
Based for each editors

#### Features
* custom node 
* subgraph
* ...

#### Nodes
* math: add, clamp, cos, divide, matrix, multiply, number, number2, number3, number4, radians, sin, subtract
* camera: camera2d, camera3d, lookat, orthographic, perspective
* transform: rotate, scale, translate
* constant: time, viewport
* data: array, combine, fetch, merge, split
* control: for
* tools: commentary, custom, print, proxy
* subgraph: input, output, property, subgraph

### rendergraph

![](./doc/rendergraph/screenshots/terrain.jpg)
    <em>Terrain</em>

![](./doc/rendergraph/screenshots/cel_shading.jpg)
    <em>Cel Shading</em>

![](./doc/rendergraph/screenshots/ssss.jpg)
    <em>Sample from Jorge Jimenez's separable-sss <a href="https://github.com/iryoku/separable-sss">SSSS</a></em>

![](./doc/rendergraph/screenshots/ssao.jpg)
    <em>Sample from learnopengl <a href="https://learnopengl.com/Advanced-Lighting/SSAO">SSAO</a></em>

#### Features
* auto gen shader uniforms
* operable nodes
<img src="./doc/rendergraph/screenshots/camera3d.gif">
* ...

#### Nodes
* vertex: model, primitive_shape, vertex_array
* resource: cubemap, render_buffer, render_target, shader, texture
* operate: clear, compute, draw
* control: pass

### terraingraph

![](./doc/terraingraph/screenshots/erosion.jpg)
    <em>Noise and erosion</em>

![](./doc/terraingraph/screenshots/brush.gif)
    <em>Brush</em>

![](./doc/terraingraph/screenshots/noise_brush.gif)
    <em>Noise brush</em>

#### Features
* multiple brushes

#### Nodes
* generator : constant, gradient, perlin, worley
* combiner : combiner
* filter: clamp, gaussian, incline, inverter, scale_bias, step, transform
* natural : erosion
* selector ： select_height, select_slope
* edit : brush
* draw : viewer

### shadergraph

![](./doc/shadergraph/screenshots/combine.jpg)
    <em>Test</em>

#### Features
* use spir-v AST instead of combine shader strings

#### Nodes
* artistic-adjustment : ChannelMixer, Contrast, Hue, InvertColors, ReplaceColor, Saturation, WhiteBalance,
* input-geometry : Normal, TexCoord, WorldPos, 
* procedural-shapes : Checkerboard, Ellipse, Polygon, Rectangle, RoundedPolygon, RoundedRectangle,
* utility-shader : FragColor, Uniform,
* uv : Flipbook, PolarCoordinates, RadialShear, Rotate, Spherize, TilingAndOffset, Twirl,
* math : Dot, Negate, Normalize, Max, Min, Mix, RotateX, RotateY, RotateZ, 
* tools : Parameter, SampleTexture,

### sdfgraph

![](./doc/sdfgraph/screenshots/combine.gif)

![](./doc/sdfgraph/screenshots/combine.jpg)
    <em>Sample from Jamie Wong's tutorial <a href="http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions/">Ray Marching and Signed Distance Functions</a></em>

#### Nodes
* primitive : Sphere, Box, RoundBox, BoxFrame, Torus, CappedTorus, Link, InfiniteCylinder, Cone, InfiniteCone, Plane, HexagonalPrism, TriangularPrism, Capsule, VerticalCapsule, VertCappedCylinder, CappedCylinder, RoundedCylinder, VertCappedCone, CappedCone, SolidAngle, VertRoundCone, RoundCone, Ellipsoid, Rhombus, Octahedron, Pyramid, Triangle, Quad,
* operation-alterations : Elongate, Rounding, Onion,
* operation-combinations : Union, Subtraction, Intersection, SmoothUnion, SmoothSubtraction, SmoothIntersection,
* operation-positioning : Symmetry, InfiniteRepetition, FiniteRepetition,
* operation-deformations and distortions : Displace, Twist, Bend,

### noisegraph

![](./doc/noisegraph/screenshots/ridged_turbulence.jpg)

#### Nodes
* noise : PerlinNoise, BillowedNoise, RidgedNoise,
* turbulence : Turbulence, IqTurbulence, SwissTurbulence, JordanTurbulence, Erosion1Turbulence, Erosion2Turbulence, DistortTurbulence,
* deriv : PerlinNoisePseudoDeriv, PerlinNoiseDeriv, NoiseDeriv,

### pbrgraph

![](./doc/pbrgraph/screenshots/standard_model.jpg)

#### Nodes
* framework : MaterialTextures, Shadergraph,
* tools : SrgbToLinear, GammaCorrection, NormalFromMap, LightingDirs,
* brdf : DistributionGgx, FresnelSchlick, GeometrySmith, 
* components : Brdf, Ibl,

## Building
First clone the project & submodules:
```
git clone git@github.com:TanTien-Engine/tantien.git
cd tantien
git submodule update --init --recursive
```

### Windows
open platform/windows/tantien.sln
copy dlls (from platform/windows/lib)

Run: editor.exe rendergraph