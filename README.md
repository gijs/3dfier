
# 3dfier

![](https://dl.dropboxusercontent.com/s/vjp0m2rc06z17qe/2016-09-06%20at%2010.53.png)


Takes 2D GIS datasets (eg topographical datasets) and "3dfies" them (as in "making them three-dimensional") by lifting every polygon to 3D.
The elevation is obtained from a point cloud (we support LAS/LAZ at this moment), and the semantics of every polygon is used to perform the lifting.
That is, water polygons are extruded to horizontal polygons, buildings to LOD1 blocks, roads as smooth surfaces, etc.
Every polygon is triangulated (constrained Delaunay triangulation) and the lifted polygons are "stitched" together so that one digital surface model (DSM) is constructed.

The lifting options can be configured in the YAML file (`myconfig.yml`) provided.
Any 2D input (which should be a planar partition) can be used as input, and each class must be mapped to one of the following:

  1. Building
  1. Terrain
  1. Road
  1. Water
  1. Forest
  1. Bridge
  1. Separation (used for concrete slabs used along canals for instance, surely very "Dutch")

It is possible to define new classes, although that would require a bit of programming.

Output is at this moment in either OBJ or CityGML (and CVS for buildings only, ie their ID and height (ground+roof) are output).
The ID of each polygon is preserved, and there is a 1-to-1 mapping between the input and the output. 

Notice that this version is very much a beta version..


## Compiling

To build you'll normally do (from 3dfier root directory):

```
mkdir build && cd build
cmake ..
make
```

The dependencies that are necessary (under Mac we suggest using [Homebrew](http://brew.sh)):

  1. LIBLAS *with* LASzip support (`brew install liblas --with-laszip`)
  1. GDAL (`brew install gdal`)
  1. Boost (`brew install boost`)
  1. CGAL (`brew install cgal`)
  4. yaml-cpp (`brew install yaml-cpp`)


## To run:

`$ ./3dfier myconfig.yml > output.obj`





  
