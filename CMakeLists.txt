# CMake entry point
cmake_minimum_required (VERSION 3.1)
project (regl-cpp)

find_package(OpenGL REQUIRED)

# get rid of annoying MSVC warnings.
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Compile external dependencies
add_subdirectory (deps)

set (CMAKE_CXX_STANDARD 11)


include_directories(
	deps/glfw-3.2.1/include/
	deps/glad/include
	deps/tiny_obj_loader
	src/
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	-lGL -lGLU -lXrandr -lXext -lX11 -lrt
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")


add_library(regl-cpp-lib src/regl-cpp.cpp src/glfw-util.cpp deps/glad/src/glad.c)


set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
	regl-cpp-lib
)

add_executable(textured-cube samples/textured-cube/main.cpp)
target_link_libraries(textured-cube  ${ALL_LIBS} )

add_executable(animated samples/animated/main.cpp)
target_link_libraries(animated ${ALL_LIBS} )



