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

void reglCppContext::transferStack(contextState& stackState, const Command& command) {
	if (command.mIndices != nullptr) {
		stackState.mIndices = command.mIndices;
	}
	if (command.mCount != -1) {
		stackState.mCount = command.mCount;
	}
	for (Attribute attribute : command.mAttributes) {
		stackState.mAttributes[attribute.mKey] = attribute.mVertexBuffer;
	}

	for (Uniform uniform : command.mUniforms) {
		stackState.mUniforms[uniform.mKey] = uniform.mValue;
	}

	if (command.mPipeline.mDepthTest.second) {
		stackState.mDepthTest = command.mPipeline.mDepthTest.first;
	}

	if (command.mPipeline.mVert != "") {
		stackState.mVert = command.mPipeline.mVert;
	}

	if (command.mPipeline.mFrag != "") {
		stackState.mFrag = command.mPipeline.mFrag;
	}

	if (
		!std::isnan(command.mPass.mClearColor[0]) &&
		!std::isnan(command.mPass.mClearColor[1]) &&
		!std::isnan(command.mPass.mClearColor[2]) &&
		!std::isnan(command.mPass.mClearColor[3])
		) {
		stackState.mClearColor = command.mPass.mClearColor;
	}

	if (
		!std::isnan(command.mViewport[0]) &&
		!std::isnan(command.mViewport[1]) &&
		!std::isnan(command.mViewport[2]) &&
		!std::isnan(command.mViewport[3])
		) {
		stackState.mViewport = command.mViewport;
	}
	
	if (!std::isnan(command.mPass.mClearDepth)) {
		stackState.mClearDepth = command.mPass.mClearDepth;
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

	ProgramInfo programInfo;

	programInfo.mProgram = LoadNormalShader(vert, frag);
	programInfo.mVert = vert;
	programInfo.mFrag = frag;

	// get all attribs:
	{
		int count = 0;
		glGetProgramiv(programInfo.mProgram, GL_ACTIVE_ATTRIBUTES, &count);
		
		constexpr int SIZE = 256;
		char buf[SIZE];
		
		for (int ii = 0; ii < count; ii++)
		{
			int length = 0;
			int size = 0;
			unsigned int type = 0;

			glGetActiveAttrib(programInfo.mProgram, (GLuint)ii, SIZE, &length, &size, &type, buf);

			std::string nameStr(buf, length);

			//printf("Attribute #%d Type: %u Name: %s\n", i, type, name);
			int attribLocation = 0;
			GL_C(attribLocation = glGetAttribLocation(programInfo.mProgram, nameStr.c_str()));

			programInfo.mAttributes[nameStr] = attribLocation;
		}
	}

	// get all uniforms:
	{
		int count = 0;
		glGetProgramiv(programInfo.mProgram, GL_ACTIVE_UNIFORMS, &count);
		
		constexpr int SIZE = 256;
		char buf[SIZE];

		for (int ii = 0; ii < count; ii++)
		{
			int length = 0;
			int size = 0;
			unsigned int type = 0;

			glGetActiveUniform(programInfo.mProgram, (GLuint)ii, SIZE, &length, &size, &type, buf);

			std::string nameStr(buf, length);

			//printf("Attribute #%d Type: %u Name: %s\n", i, type, name);
			int uniformLocation = 0;
			GL_C(uniformLocation = glGetUniformLocation(programInfo.mProgram, nameStr.c_str()));

			programInfo.mUniforms[nameStr] = uniformLocation;
		}
	}
	return programInfo;
}



void reglCppContext::submitWithContextState(contextState state) {

	if (std::isnan(state.mViewport[0]) ||
		std::isnan(state.mViewport[1]) ||
		std::isnan(state.mViewport[2]) ||
		std::isnan(state.mViewport[3])) {
		
		printf("you need to specify a viewport for your command");
	}
	
	GL_C(glViewport(state.mViewport[0], state.mViewport[1], state.mViewport[2], state.mViewport[3]));

	bool doClear = !std::isnan(state.mClearColor[0]) &&
		!std::isnan(state.mClearColor[1]) &&
		!std::isnan(state.mClearColor[2]) &&
		!std::isnan(state.mClearColor[3]);

	doClear = doClear && !std::isnan(state.mClearDepth);
	
	// translate stack state to GL commands. 
	if (doClear) {
		GL_C(glClearDepth(state.mClearDepth));
		
		GL_C(glClearColor(
			state.mClearColor[0],
			state.mClearColor[1],
			state.mClearColor[2],
			state.mClearColor[3]
		));
		
		GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	}

	//set view port  glViewport.

	if (state.mCount != -1) {


		if (state.mVert.size() == 0) {
			printf("please specify a vertex shader\n");
			exit(1);
		}
		if (state.mFrag.size() == 0) {
			printf("please specify a fragment shader\n");
			exit(1);
		}

		ProgramInfo programInfo = fetchProgram(state.mVert, state.mFrag);

		GL_C(glUseProgram(programInfo.mProgram));

		for (const auto& pair : state.mUniforms) {
			std::string uniformName = pair.first;
			UniformValue uniformValue = pair.second;

			if (programInfo.mUniforms.count(uniformName) == 0) {
				printf("the program doesnt have uniform with name %s. vert %s\n frag %s",
					uniformName.c_str(),

					programInfo.mVert,
					programInfo.mFrag
				);
			}
			unsigned int uniformLocation = programInfo.mUniforms[uniformName];

			if (uniformValue.mType == UniformValue::FLOAT_VEC1) {
				GL_C(glUniform1f(uniformLocation, uniformValue.mFloatVec1[0]));
			}
			else if (uniformValue.mType == UniformValue::FLOAT_VEC2) {
				GL_C(glUniform2f(uniformLocation, uniformValue.mFloatVec2[0], uniformValue.mFloatVec2[1]));
			}
			else if (uniformValue.mType == UniformValue::FLOAT_VEC3) {
				GL_C(glUniform3f(uniformLocation,
					uniformValue.mFloatVec3[0],
					uniformValue.mFloatVec3[1],
					uniformValue.mFloatVec3[2]));
			}
			else if (uniformValue.mType == UniformValue::FLOAT_VEC4) {
				GL_C(glUniform4f(uniformLocation,
					uniformValue.mFloatVec4[0],
					uniformValue.mFloatVec3[1],
					uniformValue.mFloatVec4[2],
					uniformValue.mFloatVec4[3]));
			}
			else if (uniformValue.mType == UniformValue::FLOAT_MAT4X4) {
				GL_C(glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, (GLfloat*)& uniformValue.mFloatMat4x4[0]));
			}
		}
	}
}

void reglCppContext::submit(const Command& command) {

	// current state at top of stack.
	contextState stackState = stateStack.top();

	transferStack(stackState, command);

	submitWithContextState(stackState);
	
}



}