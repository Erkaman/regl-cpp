emcc -Ideps/glad/include  src/main.cpp src/regl-cpp.cpp src/glfw-util.cpp  -O2 -std=c++14 -s TOTAL_MEMORY=33554432 -s USE_GLFW=3 -o htmlout/SingleFileOpenGLTex.html
