# tantien

Game engine.

## Features

### Node based
Every thing are nodes: shader, render pipeline, material, vfx, gui, edit op...

### Procedural generation
Procedural content generation

### Data driven
Use editors to generate each component

### Fast iteration
Less c&cpp parts, more scripts
Edit assets in runtime directly
Hot update

## Editors

### blueprint
Based for each editors

#### Features
* custom node 
* subgraph
* ...

#### Nodes
* math: number, number2, number3, matrix, add, subtract, multiply, divide, sin, cos, radians, clamp
* camera: camera2d, camera3d, lookat, orthographic, perspective
* transform: translate, scale, rotate
* constant: time
* data: array, fetch, merge, combine, split
* control: for
* tools: custom, print 
* subgraph: subgraph, input, output, property

### rendergraph

![](./screenshots/camera3d.jpg)
    <em>Sample from learnopengl <a href="https://learnopengl.com/Advanced-OpenGL/Cubemaps">Cubemaps</a></em>

![](./screenshots/for.jpg)
    <em>Sample from learnopengl <a href="https://learnopengl.com/PBR/Theory">PBR</a></em>

#### Features
* auto gen shader uniforms
* operable nodes
* ...

#### Nodes
* vertex: model, primitive_shape, vertex_array
* resource: texture, cubemap, shader, render_buffer, render_target
* operate: clear, draw
* control: pass

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