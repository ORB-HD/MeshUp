# Introduction

MeshUp is a 3d visualization tool for rigid-body motions. It is based on
skeletal animation. The visualization needs three things: a model file,
meshes and an animation file. This document describes the file format for
the models for MeshUp.

## Models

Models define a hierarchy of Frames to which geometric objects such as
spheres, boxes or a generic mesh can be attached. There is one special
Frame called BASE which is the global reference frame.

Between a parent and a joint frame there can be a transformation
(translation and rotation) which is called the joint frame. If the joint
frame contains no transformation the parent and child frame coincide.

For details see [Model Format](#model-format).

## Meshes

Meshes are geometric objects thar can be created with 3d content creation
tools such as Blender, MilkShape, etc. So far MeshUp only supports the OBJ
file format to load meshes. Furthermore there are some limitations (see
README for more details).

## Animations

Animation files contain the motion information independent of the model.
The actual data is stored in a comma separated format which allows easy
export from a simulation program and also plotting using Octave or even a
Spreadsheet program.

For details see [Animation Files](#animation-files).

<a id="model-format"></a>
# Model Format

Models have to be specified as a specially formatted Lua table which must
be returned by the script, i.e. if the model is specified in the table
"model = { ... }" the script has to return this when loaded. Within the
returned table, MeshUp goes through the table "frames" and builds
the model from the individual Frame Information Tables (see further down
for more information about those).

A valid file could look like this:

    model = {
      configuration = { ... }
      frames = {
        {
          <frame 1 information table>
        },
        {
          <frame 2 information table>
        }
      }
    }

    return model

## Configuration

The configuration table describes your coordinate system. Note that in
general the coordinate system is assumed to be right-handed. The
configuration is specified using the principal axes of your global
coordinate system:

Example:

    model = {
    	configuration = {
        axis_front = { 1, 0, 0 },
        axis_up    = { 0, 0, 1 },
        axis_right = { 0, -1, 0 },
      },
    
      frames = {
      ...
      }
    }

This defines a right-handed coordinate system for which X poits forward, Y
to the left and Z up (as seen from the model).

**Note:** The table frames must contain all Frame Information Tables as a list
and individual tables must *not* be specified with a key, i.e.

    frames = {
      some_frame = {
        ...
      },
      {
        ..
      }
    }

is not possible as Lua does not retain the order of the individual
frames when an explicit key is specified.

## Frame Information Table

The Frame Information Table is searched for values to build the skeletal
hierarchy. The following fields are used by MeshUp (everything else is ignored):

*name* (required, type: string):
>  Name of the body that is being added. This name must be unique.

*parent* (required, type: string):
>  If the value is "ROOT" the parent frame of this body is assumed to be
>  the base coordinate system, otherwise it must be the exact same string
>  as was used for the "name"-field of the parent frame.
  
*joint_frame* (optional, type: table):
>  Specifies the origin of the joint in the frame of the parent. It uses
>  the values (if existing):
>
>      r (3-d vector, default: (0., 0., 0.))
>      E (3x3 matrix, default: identity matrix)
>   
>  for which r is the translation and E the rotation of the joint frame.

*visuals* (optional, type: array of Mesh Information Tables):
>   Specification of all meshes that are attached to the current frame.

*points* (optional, type: array of Point Information Tables):
>   Additional points that are specified relative to the frame and move with
>	 it.

## Mesh Information Table

The Mesh Information Table describes the geometric mesh including its
properties such as color and dimensions and where it origin should be
placed in the current frame.

An example Mesh Information Table looks like this:

    HipMesh1 = {
  		name = "HipMesh1",
  		dimensions = { 0.25, 0.4, 0.25 },
  		color = { 0.8, 0.8, 0.2},
  		mesh_center = { 0, 0, 0.125 },
  		src = "meshes/unit_cube.obj",
  	},

The Mesh Information Table can use the following attributes:

*scale* (3-d vector, default:  {1., 1., 1.})
>	Scales the model relative to its default size. A scale of {2., 2., 2. }
>	will draw the mesh twice as big as its default size. Note that dimensions
>	takes precedence over scale.
	
*dimensions* (3-d vector, default:  {0., 0., 0.})
>	Scales the mesh so that its absolute size (more precisely its bounding
>	box) is that of dimensions. Note that dimensions takes precedence over
>	scale. Once it is unequal to {0., 0., 0.} it will be used instead of the
>	definition of scale.

*color* (3-d vector, default:  {1., 1., 1.})
> The color of the mesh as red-green-blue values. Black is (0., 0., 0.)
>	and white is (1., 1., 1.).

*translate* (3-d vector, default:  {0., 0., 0.})
> Shifts the whole mesh by a given vector.

*mesh_center* (3-d vector, default:  {0., 0., 0.})
>	Shifts the whole mesh so that the center of its bounding box is at the
>	position defined by mesh_center.	

*rotate* (table, default: { axis = {1., 0., 0.}, angle = 0.})
> Rotates the visual around the specified axis by the given angle.

*geometry* (table, default: none)
> A geometric object which can be specified instead of a mesh file (see
> src/). Here are examples for all supported objects along with their
> default properties:
>
> * Box Geometry:
>
>         geometry = {
>           box = { dimensions = {1., 1., 1.} }
>         }
>
> * Sphere Geometry:
>
>  	  geometry = {
>           sphere = { radius=1., rows=16, segments=32 }
>         }
>
> * Capsule Geometry:
>
>  	  geometry = {
>           capsule = { radius=1., length=2., rows=16, segments=32 }
>         }
>   The capsule geometry is aligned along the Z-axis. ```length```
>   specifies the total length of the capsule including the rounded caps.
>
> * Cylinder Geometry:
>
>  	  geometry = {
>           capsule = { radius=1., length=2., rows=16, segments=32 }
>         }
>   The cylinder geometry is aligned along the Z-axis.
>
> Please note that the attributes *geometry* and *src* are exclusive!
> Furthermore the sizes and radii specified in the geometry table will be
> overridden when ```scale``` or ```dimensions``` are present in the Mesh
> Information Table.

*src* (string)
>  The path to the mesh file, i.e. a path to an OBJ file. If the src string
>  is "mymesh.obj" it will search in the following paths from top to
>  bottom:
>
> * ```./mymesh.obj```
> * ```/mymesh.obj```
> * ```${MESHUP_PATH}/mymesh.obj```
> * ```${INSTALL_PREFIX}/mymesh.obj```
> * ```${INSTALL_PREFIX}/meshes/mymesh.obj```
> * ```/usr/local/share/meshup/meshes/mymesh.obj```
> * ```/usr/share/meshup/meshes/mymesh.obj```
>
> ```${INSTALL_PREFIX}``` is the install prefix specified via the CMake
> variables ```CMAKE_INSTALL_PREFIX```.
>
> Please note that the attributes *geometry* and *src* are exclusive!

## Point Information Table

The Point Information Table describes a single point relative to a frame.
This point then moves together with the frame. The point is optionally
drawn with a line from the frame's origin to the location of the point.

An example Point Information Table looks like this:

    ToolCenterPoint = {
  		coordinates = { 0.25, 0.1, 0.5 },
  		color = { 0.8, 0.8, 0.2},
  		draw_line = true,
  	},

The Point Information Table has the following attributes:

*coordinates* (3-d vector)
>	These are the coordinates of the point in the frame in which the point is
>	defined.
	
*color* (3-d vector, default:  {1., 1., 1.})
> The color of the mesh as red-green-blue values. Black is (0., 0., 0.)
>	and white is (1., 1., 1.).

*draw_line* (boolean, default:  false)
>	Specifies whether a line from the frame origin to the point should be
>	drawn.

*line_width* (number, default:  1.)
>	Specifies the width of the line in pixels. Note that the width range
>	may be limited by the graphics hardware and/or driver. (Added in v0.3.14)

<a id="animation-files"></a>
# Animation Files

Animation files are designed so that they can be written to a comma or tab
separated file and still be read by MeshUp. There is a small number of
keywords that specify how the data is being interpreted.

There are two sections in the animation file: COLUMNS and DATA.

## COLUMNS section

The COLUMNS section specifies the separate columns of the file and is
started with a line that only contains "COLUMNS:". Each following entry
that may be separated by commas or whitespaces specifies a single degree of
freedom for a frame or the time. The first entry corresponds to the first
column, the second for the second and so on.

The first column must always be "time". If a column should be
ignored by MeshUp set the entry to "empty".

The mapping for joints is specified in the following syntax:

    <frame name>:<joint type>:<axis>[:<unit>]

where

    <frame name> is the name of the frame used in the model.
    <joint type> can be 
      t,translation for translational motions
      r,rotation    for rotational motions
      s,scale       for scaling motions
    <axis> can be either of x,y,z for the respective axes. Negative axes can
      be specified by prepending a '-' to the axis name.
    <unit> (optional) can be r,rad, or radian to specify that the columns
      should be interpreted as radians instead of the default degrees.

For a single frame all column specifications must be consecutive in the
COLUMN section.

## DATA section

The ```DATA``` section has to be specified as multi-column data of the raw values
for which each column is separated by a "," (comma) and at least one
whitespace (space or tab).

### DATA_FROM

The section either starts with ```DATA:``` and directly after it the data or
alternatively, one can use ```DATA_FROM: <some/path/to/filename.cs>``` to load the
actual data from another data file. This can be useful when wanting to keep
the actual data as a clear .csv file or if one wants to re-use the header
of a file.

By default DATA_FROM will load the data file from the same
directory as the original file. However if the file should be loaded from
n absolute path you must start the path with a "/", e.g. ```DATA_FROM:
/some/absolute/path/data.csv```.

## Examples

Meshup comes with a set of example files, e.g. see sampleanimation.txt for
an example.
