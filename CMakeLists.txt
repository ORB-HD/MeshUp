PROJECT ( MESHUP )

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

# Needed for UnitTest++
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake )

# Perform the proper linking
SET (CMAKE_SKIP_BUILD_RPATH FALSE)
SET (CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# The QT Interface
SET( QT_USE_QTXML TRUE )
SET( QT_USE_QTOPENGL TRUE )
SET( QT_WRAP_CPP TRUE )
FIND_PACKAGE (Qt4 REQUIRED)
FIND_PACKAGE (OpenGL)
FIND_PACKAGE (Boost COMPONENTS filesystem system REQUIRED)
FIND_PACKAGE (Lua51 REQUIRED)

INCLUDE_DIRECTORIES ( 
	glew/include 
	${LUA_INCLUDE_DIR}
	src/luatables )

SUBDIRS ( 
	tests/ 
	jsoncpp/
	)

ADD_LIBRARY ( MeshUp
	src/MeshupModel.cc
	src/Animation.cc
	src/MeshVBO.cc
	src/Curve.cc
	src/objloader.cc
	src/luatables/luatables.cc
	)

ADD_LIBRARY ( glew
	glew/src/glew.c
	glew/src/glewinfo.c
	glew/src/visualinfo.c
	)

ADD_DEFINITIONS (${QT_DEFINITIONS})

INCLUDE (${QT_USE_FILE})

SET ( MainWindow_UIS
	ui/MainWindow.ui
	ui/RenderImageDialog.ui
	ui/RenderImageSeriesDialog.ui
	)

QT4_WRAP_UI (MainWindow_UIS_H ${MainWindow_UIS})

SET ( MeshupApp_SRCS
	src/main.cc
	src/glwidget.cc
	src/MeshupApp.cc
	src/AnimationEditModel.cc
	)

QT4_WRAP_CPP ( MeshupApp_MOC_SRCS
	src/MeshupApp.h
	src/RenderImageDialog.h
	src/RenderImageSeriesDialog.h
	src/glwidget.h
	src/AnimationEditModel.h
	)

ADD_EXECUTABLE ( meshup
	${MeshupApp_SRCS}
	${MeshupApp_MOC_SRCS}
	${MainWindow_UIS_H}
)

ADD_EXECUTABLE (meshup_model_converter
	converter/converter.cc
)

INCLUDE_DIRECTORIES (${QT_INCLUDE_DIR})
INCLUDE_DIRECTORIES (${QT_INCLUDE_DIR}/QtOpenGL)
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES ( include )
INCLUDE_DIRECTORIES ( jsoncpp/include )

SET_TARGET_PROPERTIES ( ${PROJECT_EXECUTABLES} PROPERTIES
	LINKER_LANGUAGE CXX
	INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" 
)

TARGET_LINK_LIBRARIES ( MeshUp
	${OPENGL_LIBRARIES}
	${Boost_LIBRARIES}
	${LUA_LIBRARIES}
	glew
	json
	)

TARGET_LINK_LIBRARIES ( meshup
	${QT_LIBRARIES}
	${OPENGL_LIBRARIES}
	glew
	MeshUp
)

TARGET_LINK_LIBRARIES ( meshup_model_converter
	${OPENGL_LIBRARIES}
	MeshUp
)

# Installation
INSTALL (TARGETS meshup
RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
)

FILE ( GLOB model_files
	models/*.json
	)

FILE ( GLOB mesh_files
	meshes/*.obj
	)

INSTALL ( FILES ${model_files} DESTINATION share/meshup/models )
INSTALL ( FILES ${mesh_files} DESTINATION share/meshup/meshes )
