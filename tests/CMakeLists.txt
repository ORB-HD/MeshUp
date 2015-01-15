PROJECT ( TESTS )

CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

# Needed for UnitTest++
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../CMake )

SET ( TESTS_SRCS
	main.cc
	AnimationTests.cc
	FrameTests.cc
	QuaternionTests.cc
	StringUtilsTests.cc

	../src/Animation.cc
	../src/Model.cc
	../src/MeshVBO.cc
	../src/Curve.cc
	../src/luatables/luatables.cc
	)

FIND_PACKAGE (UnitTest++)

INCLUDE_DIRECTORIES ( ../src/ )

SET_TARGET_PROPERTIES ( ${PROJECT_EXECUTABLES} PROPERTIES
  LINKER_LANGUAGE CXX
)

IF ( UNITTEST++_FOUND )

	ADD_EXECUTABLE ( meshuptests ${TESTS_SRCS} )

  INCLUDE_DIRECTORIES ( ${UNITTEST++_INCLUDE_DIR} )

	SET_TARGET_PROPERTIES ( meshuptests PROPERTIES
		LINKER_LANGUAGE CXX
		OUTPUT_NAME runtests
		)

	TARGET_LINK_LIBRARIES ( meshuptests
			${UNITTEST++_LIBRARY}
			${OPENGL_LIBRARIES}
			${Boost_LIBRARIES}
			lua-static
			glew
		)
		
ENDIF ( UNITTEST++_FOUND )
