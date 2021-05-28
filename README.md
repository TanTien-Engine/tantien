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
* math : Number, Number2, Number3, Number4, Matrix, Add, Subtract, Multiply, Divide, Radians, Sin, Cos, Clamp,
* camera : Camera2d, Camera3d, OrbitCamera, Lookat, Orthographic, Perspective,
* transform : Scale, Rotate, Translate,
* constant : Time, Viewport,
* data : Array, Combine, Fetch, Merge, Split,
* control : For,
* tools : Commentary, Custom, Print, Proxy, Expand, Store, Load,
* subgraph : Input, Output, Property, Subgraph,

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
* vertex : Model, Gltf, PrimitiveShape, VertexArray,
* resource : Shader, Texture, Cubemap, RenderTarget, RenderBuffer, 
* operate : Clear, Compute, Draw,
* control : Pass,

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
* generator : Constant, Gradient, Perlin, Worley,
* combiner : Combiner,
* filter : Clamp, Gaussian, Incline, Inverter, ScaleBias, Step, Transform,
* natural : Erosion, ThermalWeathering,
* selector : SelectHeight, SelectSlope,
* edit : Brush,
* draw : Viewer,
* tools : Shadergraph, 

### shadergraph

#### Features
* use spir-v AST instead of combine shader strings

#### Nodes
* input-geometry : Normal, TexCoord, WorldPos, 
* utility-shader : FragColor, Uniform,
* math : Cross, Dot, Negate, Normalize, Max, Min, Mix, Reflect, RotateX, RotateY, RotateZ, Pow, Sqrt,
* tools : Parameter, SampleTexture, TBN,

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

![](./doc/pbrgraph/screenshots/ibl.gif)

![](./doc/pbrgraph/screenshots/ibl.jpg)
    <em>IBL</em>

![](./doc/pbrgraph/screenshots/sheen.jpg)
    <em>Sheen</em>

![](./doc/pbrgraph/screenshots/clearcoat.jpg)
    <em>Clearcoat</em>

![](./doc/pbrgraph/screenshots/car.jpg)
    <em>Transmission</em>

#### Nodes
* Specular BRDF distribution : D_GGX, D_GGX_Anisotropic, D_Charlie,
* Specular BRDF visibility : V_Kelemen, V_Neubelt, V_Sheen, V_Smith, V_SmithGGXCorrelated, V_SmithGGXCorrelated_Anisotropic,
* Specular BRDF fresnel : F_Schlick, 
* Specular BRDF : Fr_CookTorrance,
* Diffuse BRDF : Fd_Burley, Fd_Lambert, Fd_Wrap,
* Tools : EnergyConservation, Shadergraph, SrgbToLinear, GammaCorrection, IorToRoughness, MirrorLight, IorToF0,

### pixelgraph

#### Nodes
* artistic-adjustment : ChannelMixer, Contrast, Hue, InvertColors, ReplaceColor, Saturation, WhiteBalance,
* uv : Flipbook, PolarCoordinates, RadialShear, Rotate, Spherize, TilingAndOffset, Twirl,
* procedural-shapes : Checkerboard, Ellipse, Polygon, Rectangle, RoundedPolygon, RoundedRectangle,
* tools : Shadergraph,

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