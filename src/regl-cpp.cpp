#include "regl-cpp.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define  LOGI(...)  printf(__VA_ARGS__)
#define  LOGE(...)  printf(__VA_ARGS__)

inline void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("OpenGL error %08x, at %s:%i - for %s.\n", err, fname, line, stmt);
	}
}

#ifdef NDEBUG
// helper macro that checks for GL errors.
#define GL_C(stmt) do {					\
	stmt;						\
    } while (0)
#else
// helper macro that checks for GL errors.
#define GL_C(stmt) do {					\
	stmt;						\
	CheckOpenGLError(#stmt, __FILE__, __LINE__);	\
    } while (0)
#endif


namespace reglCpp {

reglCppContext context;

void reglCppContext::frame(const std::function<void()>& fn) {
	contextState initialState;
	stateStack.push(initialState);
	fn();
}

void reglCppContext::submit(const pass& cmd) {
	// TODO: crash if executed outside frame.

	GL_C(glClearColor(
		cmd.mClearColor[0], 
		cmd.mClearColor[1],
		cmd.mClearColor[2],
		cmd.mClearColor[3]));

	GL_C(glClearDepth(cmd.mClearDepth));

	GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void reglCppContext::transferStack(contextState& stackState, const command& cmd) {
	if (cmd.mIndices != nullptr) {
		stackState.mIndices = cmd.mIndices;
	}
	if (cmd.mCount != -1) {
		stackState.mCount = cmd.mCount;
	}
	for (attribute attrib : cmd.mAttributes) {
		stackState.mAttributes[attrib.mKey] = attrib.mVertexBuffer;
	}

	for (uniform u : cmd.mUniforms) {
		stackState.mUniforms[u.mKey] = u.mValue;
	}

	if (cmd.mPipeline.mDepthTest.second) {
		stackState.mDepthTest = cmd.mPipeline.mDepthTest.first;
	}

	if (cmd.mPipeline.mVert != "") {
		stackState.mVert = cmd.mPipeline.mVert;
	}

	if (cmd.mPipeline.mFrag != "") {
		stackState.mFrag = cmd.mPipeline.mFrag;
	}

	if (
		!std::isnan(cmd.mPass.mClearColor[0]) &&
		!std::isnan(cmd.mPass.mClearColor[1]) &&
		!std::isnan(cmd.mPass.mClearColor[2]) &&
		!std::isnan(cmd.mPass.mClearColor[3])
		) {

		stackState.mClearColor = cmd.mPass.mClearColor;
	}

	if (!std::isnan(cmd.mPass.mClearDepth)) {
		stackState.mClearDepth = cmd.mPass.mClearDepth;
	}

}

inline char* GetShaderLogInfo(GLuint shader) {
	GLint len;
	GLsizei actualLen;
	GL_C(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len));
	char* infoLog = new char[len];
	GL_C(glGetShaderInfoLog(shader, len, &actualLen, infoLog));
	return infoLog;
}

inline GLuint CreateShaderFromString(const std::string& shaderSource, const GLenum shaderType) {
	GLuint shader;

	GL_C(shader = glCreateShader(shaderType));
	const char* c_str = shaderSource.c_str();
	GL_C(glShaderSource(shader, 1, &c_str, NULL));
	GL_C(glCompileShader(shader));

	GLint compileStatus;
	GL_C(glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus));
	if (compileStatus != GL_TRUE) {
		LOGI("Could not compile shader\n\n%s \n\n%s\n", shaderSource.c_str(),
			GetShaderLogInfo(shader));
		exit(1);
	}

	return shader;
}

inline GLuint LoadNormalShader(const std::string& vsSource, const std::string& fsShader) {

	bool useGl3 = true;// just hardcode this for now.

	std::string prefix = "";

	prefix += "#version 150\n";
	prefix += "#define GL3\n";

	prefix += std::string(R"(

)");

	GLuint vs = CreateShaderFromString(prefix + vsSource, GL_VERTEX_SHADER);
	GLuint fs = CreateShaderFromString(prefix + fsShader, GL_FRAGMENT_SHADER);

	GLuint shader = glCreateProgram();
	glAttachShader(shader, vs);
	glAttachShader(shader, fs);
	glLinkProgram(shader);

	GLint Result;
	glGetProgramiv(shader, GL_LINK_STATUS, &Result);
	if (Result == GL_FALSE) {
		LOGI("Could not link shader \n\n%s\n", GetShaderLogInfo(shader));
		exit(1);
	}

	glDetachShader(shader, vs);
	glDetachShader(shader, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return shader;
}

reglCppContext::ProgramInfo reglCppContext::fetchProgram(const std::string& vert, const std::string& frag){
	std::string key = vert + frag;
	
	if (programCache.count(key) != 0) {
		return programCache[key];
	}

	ProgramInfo program;

	program.mProgram = LoadNormalShader(vert, frag);

	// get all attribs:
	{
		int count = 0;
		glGetProgramiv(program.mProgram, GL_ACTIVE_ATTRIBUTES, &count);
		
		constexpr int SIZE = 256;
		char buf[SIZE];
		
		for (int ii = 0; ii < count; ii++)
		{
			int length = 0;
			int size = 0;
			unsigned int type = 0;

			glGetActiveAttrib(program.mProgram, (GLuint)ii, SIZE, &length, &size, &type, buf);

			std::string nameStr(buf, length);

			//printf("Attribute #%d Type: %u Name: %s\n", i, type, name);
			int attribLocation = 0;
			GL_C(attribLocation = glGetAttribLocation(program.mProgram, nameStr.c_str()));

			program.mAttributes[nameStr] = attribLocation;
		}
	}

	// get all uniforms:
	{
		int count = 0;
		glGetProgramiv(program.mProgram, GL_ACTIVE_UNIFORMS, &count);
		
		constexpr int SIZE = 256;
		char buf[SIZE];

		for (int ii = 0; ii < count; ii++)
		{
			int length = 0;
			int size = 0;
			unsigned int type = 0;

			glGetActiveUniform(program.mProgram, (GLuint)ii, SIZE, &length, &size, &type, buf);

			std::string nameStr(buf, length);

			//printf("Attribute #%d Type: %u Name: %s\n", i, type, name);
			int uniformLocation = 0;
			GL_C(uniformLocation = glGetUniformLocation(program.mProgram, nameStr.c_str()));

			program.mUniforms[nameStr] = uniformLocation;
		}
	}
	return program;
}

void reglCppContext::submit(const command& cmd) {

	// current state at top of stack.
	contextState stackState = stateStack.top();


	transferStack(stackState, cmd);

	// translate stack state to GL commands. 
	{
		if (
			!std::isnan(stackState.mClearColor[0]) &&
			!std::isnan(stackState.mClearColor[1]) &&
			!std::isnan(stackState.mClearColor[2]) &&
			!std::isnan(stackState.mClearColor[3])) {

			if (!std::isnan(stackState.mClearDepth)) {
				GL_C(glClearDepth(stackState.mClearDepth));
			}
			
			GL_C(glClearColor(
				stackState.mClearColor[0],
				stackState.mClearColor[1],
				stackState.mClearColor[2],
				stackState.mClearColor[3]
			));
			
			GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		}

		if (cmd.mPipeline.mVert.size() == 0) {
			printf("please specify a vertex shader\n");
			exit(1);
		}
		if (cmd.mPipeline.mFrag.size() == 0) {
			printf("please specify a fragment shader\n");
			exit(1);
		}

		ProgramInfo program = fetchProgram(cmd.mPipeline.mVert, cmd.mPipeline.mFrag);


		printf("lol");

	}
	
}



}