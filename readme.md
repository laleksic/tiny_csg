# tiny_csg

**tiny_csg** is a library that generates meshes from brush-based level data and supports incremental updates (real-time CSG). It is intended to be used as a backend in 3d level editors and/or generators.

Here's an example of the sort of thing this is for:

[![CSG Level Editor Prototype](http://img.youtube.com/vi/bU-jOUQH5xI/0.jpg)](http://www.youtube.com/watch?v=bU-jOUQH5xI "CSG Level Editor Prototype")

[![CSG Prototype: Transforming Brushes](http://img.youtube.com/vi/DZAa6DT7ITg/0.jpg)](http://www.youtube.com/watch?v=DZAa6DT7ITg "CSG Prototype: Transforming Brushes")

Take note that this is a hobby project and still just a prototype, so it is not tested, not optimized, probably buggy, etc. I can't promise when (or even if) I'll want and be able to keep working on this, but I've still uploaded it (as it is) in case anyone finds it useful. 

If you are interested, please feel free to comment or ask questions [here](https://github.com/laleksic/tiny_csg/discussions). 

The documentation consists of this readme file and a hastily coded under-commented demo (`demo.cpp`).

Building **tiny_csg** requires C++20 and GLM. The demo additionaly needs OpenGL 2.1, SDL2 and GLEW. The `3rdp` directory contains some extra third party libraries used only in the demo. 

## Building

As always with CMake:
```
mkdir build
cd build
cmake .. 
make -j4
```

Tested only on Windows 7 MSYS2.

## Usage

Include `csg.hpp`, everything is in the `csg` namespace.

The `world_t` type is your gateway into the library.

The `volume_t` type is an enumeration of different kinds of volumes that can exist in the world. By default no values of this type are defined, you should define them yourself in your code. For example:

```c++
static constexpr volume_t AIR = 0;
static constexpr volume_t SOLID = 1;
static constexpr volume_t WATER = 2;
// etc.
```

The world has a *void volume* property which is the volume that fills the infinite void of the world by default.

```c++
// create the world
world_t world;

// get/set the void volume
volume_t void_volume = world.get_void_volume();
world.set_void_volume(SOLID);
```

* Pick `SOLID` if you want a Thief-style world-- infinitely solid where you dig out rooms.
* Pick `AIR` if you want a Quake-style world-- infinitely empty where you block out walls/floors/ceilings.

Brushes are the main building blocks of the world. They are convex polyhedrons defined by a set of bounding planes. The polyhedron itself is the intersection of the negative halfspaces of these planes.

```c++
// create a new brush
brush_t *brush = world.add();

// get/set the bounding planes
vector<plane_t> planes = {/*...*/};
planes = brush->get_planes();
brush->set_planes(planes);

// remove a brush
world.remove(brush);

// iterate over brushes in world 
brush = world.first();
while (brush) {
	brush_t *next = world.next(brush);
	// ...
	brush = next;
}
```

Planes are defined by their normal vector and offset, such that the following holds for all points on the plane: `dot(normal, point) + offset = 0`.

```c++
struct plane_t {
    glm::vec3 normal;
    float     offset;
};
```

Each brush also has an associated *volume operation* and *time* property.

The *volume operation* describes how the brush affects the world. Here's some examples of the kind of brushes you can have:

* air brush -- fills its volume unconditionally with air
* solid brush -- fills its volume unconditionally with solid matter
* flood brush -- converts any air intersecting with its volume to water, but leave other volumes (like solid matter) untouched

Volume operations are implemented as functions from volume to volume.

```cpp
brush_t *air_brush, *solid_brush, *flood_brush;
// ...
// create and set volume operations
air_brush->set_volume_operation(make_fill_operation(AIR)); 
solid_brush->set_volume_operation(make_fill_operation(SOLID)); 
flood_brush->set_volume_operation(make_convert_operation(AIR, WATER)); 
```

The *time* describes the order in which volume operations are carried out. For example, digging out some empty space with an air brush and then filling it up with a solid brush is not the same as doing the reverse. The order of operations matters.

```c++
// get/set the time
int time = brush->get_time();
brush->set_time(time);
```

A brush with a lower time 'comes first'. If brushes have the same time, their unique id (order of creation) is used as a tie-breaker.

```c++
// get unique id
uid = brush->get_uid();
```

Once you setup your world the way you want it, call the world's `rebuild` method. Rebuilding the world will generate all the geometry from the brush data, which you can then render, export to your mesh format, or process it further in any way you want.

This library uses a real-time method with incremental updates, so rebuilding will recalculate only what is necessary based on changes since the last rebuild.

The method returns a collection of brushes that were actually rebuilt.

```c++
auto rebuilt = world.rebuild();
for (brush_t *b: rebuilt) {
	// ...
}
```

### Output data

One fact about the final mesh is that all polygons will be fragments of the face polygons of the brushes. When you rebuild the world, the algorithm calculates the face polygon for each plane, and then carves it up into fragments as necessary.

"As necessary" means, until we can uniquely say which volume will be on the fragment's front side and which on the fragment's back side, when all volume operations are completed in their correct order.

Here are the structures that get calculated when you rebuild (the output data):

```c++
struct vertex_t {
    face_t    *faces[3]; 
    glm::vec3 position;
};

struct fragment_t {
    face_t                  *face;
    std::vector<vertex_t>   vertices;
    volume_t                front_volume;
    volume_t                back_volume;
    brush_t                 *front_brush;
    brush_t                 *back_brush;
};

struct face_t {
    const plane_t           *plane;
    std::vector<vertex_t>   vertices;
    std::vector<fragment_t> fragments;
};
```

You access the output data by calling `get_faces` method on a brush.
```c++
world.rebuild(); //rebuild first
auto faces = brush->get_faces();
for (face_t& face: faces) {
	// ...
}
```

The output data is invalidated on world rebuild.

Here is how you would use this information:

When drawing, the only visible polygons should be the boundary ones: the ones on the boundary of two different volumes. So you can just discard all fragments whose back and front volumes are the same.

As for the boundary polygons, you should render the sides that face into an air volume (i.e. if the front volume is air, render with backface culling, and if the back volume is air, render with frontface culling). Here's how that might look in code:

```c++
world_t world;
// ... add brushes to world

world.rebuild(); 

// rendering code
brush_t *brush = world.first();
while (brush) {
	auto faces = brush->get_faces();
	for (face_t& face: faces) {
		for (fragment_t& fragment: face.fragments) {
			volume_t front = fragment.front_volume;
			volume_t back  = fragment.back_volume;
			
			if (front == back) // discard air/air and solid/solid polygons.
				continue;
				
			if (front == AIR)
				// render fragment with backface culling.
			if (back == AIR)
				// render fragment with frontface culling.
		}
	}
	brush = world.next(brush);
}
```

Of course that just goes for the two standard volumes. If you introduce water, you'll probably want to draw an air/water boundary polygon doublesided, and with a different material/shader.

Non-boundary polygons can still be useful to you in contexts other then drawing. If you're doing portal rendering, you can use air/air polygons as portals between two sectors/brushes.

### Triangulating fragments

For the benefit of easier rendering with systems that only support rendering triangles (and not convex polygons), I provide a convenience method to triangulate a fragment.

```cpp
struct triangle_t {
    int i,j,k; // indices into the fragment's vertex array
};

std::vector<triangle_t> triangulate(const fragment_t& fragment);
```

### Intersection queries

Additionaly rebuilding the world allows you to access a brush's axis-aligned bounding box. 

```cpp
struct box_t {
    glm::vec3 min, max;
};

box_t box = brush->get_box();
```

You may also use the following types/methods for various intersection queries.

```cpp
struct ray_t {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct ray_hit_t {
    brush_t    *brush;
    face_t     *face;
    fragment_t *fragment;
    float      parameter; // ray.origin + parameter * ray.direction = position
    glm::vec3  position;
};

std::vector<brush_t*>  brush_t::query_point(const glm::vec3& point);
std::vector<brush_t*>  brush_t::query_box(const box_t& box);
std::vector<ray_hit_t> brush_t::query_ray(const ray_t& ray);
std::vector<brush_t*>  brush_t::query_frustum(const glm::mat4& view_projection);
```

* The point query returns the brushes whose bounding box contains the given point.
* The box query returns the brushes whose bounding box intersects the given box.
* The ray intersections are exact and will be sorted near to far.
* The frustum query call expects an OpenGL style matrix and returns a list of brushes that should be drawn (for frustum culling).

### Userdata

You can attach arbitrary data to `world_t` and `brush_t`.

```c++
std::any world_t::userdata;
std::any brush_t::userdata;
```

### Limitations

* This library only generates vertex positions; it does not generate normals or UV coordinates. Generating these attributes is orthogonal to the CSG process, so I leave it up to you to implement as you wish. The library should provide enough necessary information for you to use in calculations.
* The intersection queries aren't accelerated by a bounding volume hierarchy yet.
* The work done per-brush when rebuilding can be parallelized, but isn't, yet.
* Nothing is done to prevent or remove T-junctions.
* In general this is still just a prototype, not tested enough, not optimized, probably buggy, etc.

## Thanks

I'd like to thank the following folks for their very helpful tutorials and articles that got me started with CSG:

* Gerald Filimonov - [Removing illegal geometry from data imported from quake map files](https://web.archive.org/web/20081019041535/http://www.3dtechdev.com/tutorials/illegalgeometry/illegalgeometrytut.html)
* Stefan Hajnoczi - [.MAP files file format description, algorithms, and code](https://github.com/stefanha/map-files/blob/master/MAPFiles.pdf)
* Dan Royer - [Constructive Solid Geometry tutorial](https://web.archive.org/web/20031207112200/http://www.cfxweb.net/~aggrav8d/tutorials/csg.html)
* Sean Barrett - [The 3D Software Rendering Technology of 1998's Thief: The Dark Project](https://nothings.org/gamedev/thief_rendering.html#csg)
* Sander Van Rossen - [Geometry in Milliseconds: Real-Time Constructive Solid Geometry](https://www.youtube.com/watch?v=Iqmg4gblreo)
* Alan Baylis - [Constructive Solid Geometry Tutorial](http://web.archive.org/web/20201126190349/https://www.alsprogrammingresource.com/csg_tutorial.html)

...and the following folks for explaining geometry algorithms I've used:

* Tavian Barnes - [Fast branchless ray/box intersection algorithm](https://tavianator.com/fast-branchless-raybounding-box-intersections-part-2-nans/)
* Unknown - [Extracting frustum planes from matrix](http://web.archive.org/web/20211128131430/https://gdbooks.gitbooks.io/legacyopengl/content/Chapter8/frustum.html)

...and last, but not least, I'd like to thank the developers of the libraries I'm using:

* Oded Lazar - [Implementing Go's defer keyword in C++](http://web.archive.org/web/20211006173620/https://oded.dev/2017/10/05/go-defer-in-cpp/)
* Nicolas Guillemot - [flythrough_camera](https://github.com/nlguillemot/flythrough_camera)
* Artur Eganyan, David Merfield - [randomColor-cpp](https://github.com/artureganyan/randomColor-cpp)
* Tristan Maat - [shader_loader](https://github.com/TLATER/shader_loader)
* G-Truc Creation - [glm](https://github.com/g-truc/glm)
* SDL Developers - [SDL2](https://www.libsdl.org/index.php)
