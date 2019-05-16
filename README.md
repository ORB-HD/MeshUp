MeshUp - A Copyright (c) 2012-2018 Martin Felis <martin.felis@iwr.uni-heidelberg.de>

# Introduction

MeshUp is a visualization tool for rigid multibody systems. It renders models in real-time and allows to render motions directly to videos or image sequences.

# Features:

  * Simple definition of models (see directory models/ for examples)
  * Model serialization to and from Lua files
  * Loading of Wavefront OBJ files that can be attached to the meshes
  * Customizable frame definitions order of rotation angles
  * Loading and interpolation of key frame animation (see sampleanimation.txt)
  * Direct rendering of videos or image sequences
	* Lua scripting
  * Rendering of forces and torques arrows

# Usage:

	meshup [model_file(s)] [animation_file(s)] [force_file(s)]

See meshup --help for more options.

# Installation

## Prepackaged versions

  * [for Arch Linux, there is a package available in the AUR](https://aur.archlinux.org/packages/meshup/)

# Building and Installation


## Linux

1. Install RBDL, Lua, and the luamodel addon. See the readme file of [rbdl-orb](https://github.com/ORB-HD/rbdl-orb) for detailed installation instructions.

2. Install the graphics libraries that meshup depends on. Skip this step if you have installed puppeteer which uses the same graphics libraries.
    ```
    sudo apt install ffmpeg libavutil-dev libavcodec-dev libavutil-dev libavformat-dev libswscale-dev libvtk6-qt-dev
    ```
3. Install Boost	
	```
	sudo apt install libboost-all-dev
	```   
4. Clone and build the code:  
	```
	git clone https://github.com/ORB-HD/MeshUp
	```
5. Make the build and install folders in the same parent folder as Meshup 
	```    
	mkdir MeshUp-build
	mkdir MeshUp-install
	```
	Note: the *Meshup-install* folder is unecessary if you want a user-wide installation.
6. Run cmake (or ccmake) as you normally would from the Meshup-build folder
7. If all of the dependencies are found CMake will return without error
8. From the terminal inside of Meshup-build type
	```
	make install
	```
9. *(optional)*: If you have installed Meshup to a local folder (e.g. Meshup-install) add the following lines to your .bashrc file:
	```
	export MESHUP=/home/.../meshup-install/bin
	export PATH=$PATH:$MESHUP
	```
	Note that the (...) should be replaced with the string of folders that is appropriate for your chosen install path.
10. Open a terminal inside of the *Meshup* folder and see if the example model loads. If all goes well the Meshup GUI will be launched and you'll see a colorful humanoid model.

```
meshup models/knubbi.lua
```


# File Format Documentation

The different file formats used in MeshUp are described in doc/FormatDescriptions.md.

# Model Files

MeshUp tries to find the model file by searching for a file called "<model_name>" or "<model_name>.lua". It first checks in the local directory "./" and "./models/". After that MeshUp checks whether the environment variable MESHUP_PATH is set and checks for the file in $MESHUP_PATH/ or $MESHUP_PATH/models.

Please note that as default MeshUp interprets all angular values as degree values, not radians.

# Meshes

Similar as in storing [Model Files](#markdown-header-model-files), MeshUp tries to find meshes in various places. Unlike for models, meshes are required to be in a subfolder called "meshes/".

See [Notes](#markdown-header-notes) further down for information on how to export meshes to OBJ files that can be included directly into Meshup.

# Animation Files

Animation files are designed so that they can be written to a comma or tab
separated file and still be read by MeshUp. There is a small number of
keywords that specify how the data is being interpreted.

There are two sections in the animation file: COLUMNS and DATA.

## COLUMNS section

The COLUMNS section specifies the separate columns of the file and is started with a line that only contains "COLUMNS:". Each following entry that may be separated by commas or whitespaces specifies a single degree of freedom for a frame or the time. The first entry corresponds to the first column, the second for the second and so on.

The first column must always be "time". If a column should be ignored by MeshUp set the entry to "empty".

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

The DATA section has to be specified as multi-column data of the raw values
for which each column is separated by a "," (comma) and at least one whitespace (space or tab).

The section either starts with "DATA:" and directly after it the data or
alternatively, one can use "DATA_FROM: <some_path_to_filename>" to load the actual data from another data file. This can be useful when wanting to keep the actual data as a clear .csv file or if one wants to re-use the header
of a file.

See sampleanimation.txt for an example.

# Forces and Torques Files

Forces and Torques Files are CSV files with the file extention ".ff". The first column specifies the time of the following data. After the time column, each next 9 columns represent one force/torque entry. The first 3 columns of the 9 contain the position of the force and torque. The next 3 columns are the force data and the 3 columns after are the torque data.

# Notes

Wavefront OBJ restrictions:

  * Faces of the model must be triangles
  * Textures are not supported
  * Materials are not supported

Exporting Meshes from Blender:

When exporting meshes as Wavefront OBJ files from Blender, make sure to
apply the following settings:

	  [X] Selection Only
	  [X] Apply Modifiers
	  [X] Include Edges
	  [X] Include Normals
	  [X] Triangulate Faces
	  [X] Objects as OBJ Objects
	  Forward   : -X Forward
	  Up        :  Y Up
	  Path Mode : Auto

You can save these settings as "Operator Presets" to simplify the export
in the future.

# Bugs

Please use the bug tracker at [https://github.com/ORB-HD/MeshUp/issues](https://github.com/ORB-HD/MeshUp/issues) to report or view fixed bugs.

# License

Meshup is published under the MIT license. However Meshup makes use of
other libraries such as Qt, GLEW, jsoncpp, and Lua for which the actual
license may differ.

	Copyright © 2012 Martin Felis <martin.felis@iwr.uni-heidelberg.de>
	
	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:
	
	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE. 

